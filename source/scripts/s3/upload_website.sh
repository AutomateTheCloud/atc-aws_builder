#!/bin/bash
###------------------------------------------------------------------------------------------------
# alias:   aws_builder.s3.upload_website
# script:  upload_website.sh
# purpose: S3 (Upload Website)
# version: 1.0.0
###------------------------------------------------------------------------------------------------

###------------------------------------------------------------------------------------------------
## Config
declare -r SELF_IDENTITY="upload_website"
declare -r SELF_IDENTITY_H="S3 (Upload Website)"
declare -a ARGUMENTS_DESCRIPTION=(
    '-r <aws_region> : AWS Region'
    '-s <stack_name> : Cloudformation Stack Name'
    '-f <file> : s3_website.yml Template'
    '-d <source_directory> : Static site files directory'
    '-F : Force Sync (optional, defaults to no)'
    '-p <file>: Placeholder Var file (optional, if specified, will replace placeholders in files located in source directory)'
    '-i <invalidation_path>: Invalidation Path (optional, for use when specifying S3 Prefix and forcing an invalidation)'
    '-P <profile> : AWS Profile (optional)'
)

###------------------------------------------------------------------------------------------------
## Load Defaults
declare -r GLOBAL_CONFIG_FILE="${DIRECTORY_AWS_BUILDER:-/opt/aws_builder}/config/global.config"
source "${GLOBAL_CONFIG_FILE}" || exit 3

###------------------------------------------------------------------------------------------------
## Declare dependencies
REQUIRED_SOURCE_FILES+=(
    "${LIB_FUNCTIONS_AWS_INITIALIZE}"
    "${LIB_FUNCTIONS_AWS_CLOUDFORMATION}"
    "${LIB_FUNCTIONS_AWS_S3_WEBSITE}"
    "${LIB_FUNCTIONS_AWS_SSM}"
)

###------------------------------------------------------------------------------------------------
## Script specific settings (non-configurable)
# Variables
VARIABLES_REQUIRED=(
    'aws_region'
    'aws_stack_name'
    'file_s3_website_yml_template'
    'directory_site_source'
    'force_sync'
)
STACK_VARIABLES_REQUIRED=(
    'ProjectAbbr'
    'FunctionAbbr'
    'Environment'
)
FORCE_SYNC=no
TMP_PARAMETER_PATH_GLOBAL=""
TMP_PARAMETER_PATH_FUNCTION=""
FILE_CLOUDFORMATION_OUTPUT=""
FILE_SECRETS=""

TMP_FILE_APPLICATION_VARS_GLOBAL=""
TMP_FILE_APPLICATION_VARS_FUNCTION=""

FILE_PLACEHOLDER_VAR=""

###------------------------------------------------------------------------------------------------
## Main
# Process Arguments
while getopts "hVr:s:f:d:Fp:P:" OPTION; do
    case $OPTION in
        r) AWS_REGION=$OPTARG;;
        s) AWS_STACK_NAME=$OPTARG;;
        f) FILE_S3_WEBSITE_YML_TEMPLATE=$OPTARG;;
        d) DIRECTORY_SITE_SOURCE=$OPTARG;;
        F) FORCE_SYNC=yes;;
        p) FILE_PLACEHOLDER_VAR=$OPTARG;;
        P) AWS_PROFILE_TARGET=$OPTARG;;
        h) usage; exit 0;;
        V) echo "$(return_script_version "${0}")"; exit 0;;
        *) echo "ERROR: There is an error with one or more of the arguments"; usage; exit $E_BAD_ARGS;;
        ?) echo "ERROR: There is an error with one or more of the arguments"; usage; exit $E_BAD_ARGS;;
    esac
done

start_logic
log "${SELF_IDENTITY_H}: Started"

###------------------------------------------------------------------------------------------------
## START Initialize
line_break
log_highlight "Initialize"

DIRECTORY_SITE_SOURCE="$(echo "${DIRECTORY_SITE_SOURCE}" | sed 's:/*$::')"

verify_array_key_values "VARIABLES_REQUIRED[@]"
RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then exit_logic $E_BAD_ARGS; fi

if [ ! -f "${FILE_S3_WEBSITE_YML_TEMPLATE}" ]; then
    TMP_ERROR_MSG="s3_website.yml.template file does not exist [${FILE_S3_WEBSITE_YML_TEMPLATE}]"
    log_error "- ${TMP_ERROR_MSG}"
    exit_logic $E_OBJECT_NOT_FOUND "${TMP_ERROR_MSG}"
fi
if [ ! -d "${DIRECTORY_SITE_SOURCE}" ]; then
    TMP_ERROR_MSG="Site Source directory does not exist [${DIRECTORY_SITE_SOURCE}]"
    log_error "- ${TMP_ERROR_MSG}"
    exit_logic $E_OBJECT_NOT_FOUND "${TMP_ERROR_MSG}"
fi

line_break
log "- AWS Region:               [${AWS_REGION}]"
log "- CloudFormation Stack:     [${AWS_STACK_NAME}]"
log "- S3 Website YAML Template: [${FILE_S3_WEBSITE_YML_TEMPLATE}]"
log "- Site Source Directory:    [${DIRECTORY_SITE_SOURCE}]"
log "- Force Sync:               [${FORCE_SYNC}]"
if(! is_empty "${FILE_PLACEHOLDER_VAR}"); then
    log "- Placeholder Var File:     [${FILE_PLACEHOLDER_VAR}]"
fi
line_break

generate_temp_file FILE_CLOUDFORMATION_OUTPUT "Cloudformation Stack Output"
generate_temp_file TMP_FILE_APPLICATION_VARS_GLOBAL "Application Vars (Global) Output"
generate_temp_file TMP_FILE_APPLICATION_VARS_FUNCTION "Application Vars (Function) Output"
generate_temp_file TMP_FILE_APPLICATION_VARS "Application Vars"

cloudformation_get_outputs "${FILE_CLOUDFORMATION_OUTPUT}" "${AWS_STACK_NAME}" "${AWS_REGION}"
RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then exit_logic ${RETURNVAL}; fi

load_info_s3_website "${FILE_CLOUDFORMATION_OUTPUT}"
RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then exit_logic ${RETURNVAL}; fi

if(! is_empty "${FILE_PLACEHOLDER_VAR}"); then
    load_array_properties_from_file "STACK_VARIABLES_REQUIRED[@]" "${FILE_CLOUDFORMATION_OUTPUT}" "STACK"
    verify_array_key_values "STACK_VARIABLES_REQUIRED[@]" "STACK"
    RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then exit_logic $RETURNVAL; fi

    TMP_PARAMETER_PATH_GLOBAL="/secrets/${STACK_PROJECTABBR}/global/${STACK_ENVIRONMENT}/vars/"
    TMP_PARAMETER_PATH_FUNCTION="/secrets/${STACK_PROJECTABBR}/${STACK_FUNCTIONABBR}/${STACK_ENVIRONMENT}/vars/"
        
    log "${FUNCTION_DESCRIPTION}: retrieving parameters (Global)"
    parameters_to_properties_file "${TMP_PARAMETER_PATH_GLOBAL}" "${TMP_FILE_APPLICATION_VARS_GLOBAL}" yes "${AWS_REGION}"
    RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then exit_logic $RETURNVAL; fi

    log "${FUNCTION_DESCRIPTION}: retrieving parameters (Function)"
    parameters_to_properties_file "${TMP_PARAMETER_PATH_FUNCTION}" "${TMP_FILE_APPLICATION_VARS_FUNCTION}" yes "${AWS_REGION}"
    RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then exit_logic $RETURNVAL; fi
    cat ${FILE_CLOUDFORMATION_OUTPUT} ${TMP_FILE_APPLICATION_VARS_GLOBAL} ${TMP_FILE_APPLICATION_VARS_FUNCTION} | grep -v "^#" | sort | uniq > ${TMP_FILE_APPLICATION_VARS}

    exec_script "${SCRIPTS_UTILITY_REPLACE_PLACEHOLDERS}" -d "${DIRECTORY_SITE_SOURCE}" -p "${FILE_PLACEHOLDER_VAR}" -i "${TMP_FILE_APPLICATION_VARS}"
    RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then exit_logic $RETURNVAL; fi
fi
## END Initialize
###------------------------------------------------------------------------------------------------

###------------------------------------------------------------------------------------------------
## START Execution
line_break
log_highlight "Execution"

process_s3_website "${FILE_S3_WEBSITE_YML_TEMPLATE}" "${DIRECTORY_SITE_SOURCE}" "${FORCE_SYNC}" "${AWS_REGION}"
RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then exit_logic ${RETURNVAL}; fi
## END Execution
###------------------------------------------------------------------------------------------------

log_success "${SELF_IDENTITY_H}: Finished"
exit_logic 0
