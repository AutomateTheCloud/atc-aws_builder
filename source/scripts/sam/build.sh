#!/bin/bash
###------------------------------------------------------------------------------------------------
# alias:   aws_builder.sam.build
# script:  build.sh
# purpose: Builds SAM template and dependencies from AWS Builder SAM project
# version: 1.0.0
###------------------------------------------------------------------------------------------------

###------------------------------------------------------------------------------------------------
## Config
declare -r SELF_IDENTITY="sam_build"
declare -r SELF_IDENTITY_H="SAM (Build)"
declare -a ARGUMENTS_DESCRIPTION=(
    '-f <file> : AWS Builder SAM Project File'
    '-s <s3_bucket_temp> : S3 Bucket to Place Temporary files'
    '-a <s3_bucket_artifacts> : S3 Bucket to Artifacts'
    '-d <directory> : Output Directory'
    '-P <profile> : AWS Profile (optional)'
)

###------------------------------------------------------------------------------------------------
## Load Defaults
declare -r GLOBAL_CONFIG_FILE="${DIRECTORY_AWS_BUILDER:-/opt/aws_builder}/config/global.config"
source "${GLOBAL_CONFIG_FILE}" || exit 3

###------------------------------------------------------------------------------------------------
## Declare dependencies
REQUIRED_SOURCE_FILES+=(
   "${LIB_FUNCTIONS_CORE_BUILDER}"
   "${LIB_FUNCTIONS_AWS_INITIALIZE}"
   "${LIB_FUNCTIONS_AWS_CLOUDFORMATION}"
)

###------------------------------------------------------------------------------------------------
## Variables
VARIABLES_REQUIRED=(
    'file_project'
    's3_bucket_temp'
    's3_bucket_artifacts'
    'directory_output'
)

FILE_PROJECT=""
DIRECTORY_OUTPUT=""

SAM_S3_PREFIX=""

FILE_SAM_YAML=""
FILE_CLOUDFORMATION_TEMPLATE=""
FILE_CLOUDFORMATION_PARAMETERS=""
FILE_CLOUDFORMATION_TAGS=""
FILE_CLOUDFORMATION_DETAILS=""
TMP_FILE_CLOUDFORMATION_TEMPLATE=""
TMP_FILE_CLOUDFORMATION_PARAMETERS=""
TMP_FILE_CLOUDFORMATION_TAGS=""
TMP_FILE_CLOUDFORMATION_DETAILS=""

###------------------------------------------------------------------------------------------------
## Main
# Process Arguments
while getopts "hVf:s:a:d:P:" OPTION; do
    case $OPTION in
        f) FILE_PROJECT=$OPTARG;;
        s) S3_BUCKET_TEMP=$OPTARG;;
        a) S3_BUCKET_ARTIFACTS=$OPTARG;;
        d) DIRECTORY_OUTPUT=$OPTARG;;
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

DIRECTORY_OUTPUT="$(echo "${DIRECTORY_OUTPUT}" | sed 's:/*$::')"

verify_array_key_values "VARIABLES_REQUIRED[@]"
RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then exit_logic $E_BAD_ARGS; fi

if [ ! -f "${FILE_PROJECT}" ]; then
    TMP_ERROR_MSG="Project file does not exist [${FILE_PROJECT}]"
    log_error "- ${TMP_ERROR_MSG}"
    exit_logic $E_OBJECT_NOT_FOUND "${TMP_ERROR_MSG}"
fi

FILE_SAM_YAML="$($(which dirname) "${FILE_PROJECT}")/sam.yaml"
if [ ! -f "${FILE_SAM_YAML}" ]; then
    TMP_ERROR_MSG="SAM YAML file does not exist [${FILE_SAM_YAML}]"
    log_error "- ${TMP_ERROR_MSG}"
    exit_logic $E_OBJECT_NOT_FOUND "${TMP_ERROR_MSG}"
fi

line_break
log "- AWS Builder SAM Project:   [${FILE_PROJECT}]"
log "- Output Directory (base): [${DIRECTORY_OUTPUT}]"

load_project_sam "${FILE_PROJECT}" "${FILE_SAM_YAML}"
RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then exit_logic ${RETURNVAL}; fi

SAM_S3_PREFIX="sam/${DETAILS_PROJECT_ABBR}/${DETAILS_FUNCTION_ABBR}/${DETAILS_STACK_NAME}"

generate_temp_file TMP_FILE_CLOUDFORMATION_TEMPLATE "template file"
generate_temp_file TMP_FILE_CLOUDFORMATION_PARAMETERS "parameters file"
generate_temp_file TMP_FILE_CLOUDFORMATION_TAGS "tags file"
generate_temp_file TMP_FILE_CLOUDFORMATION_DETAILS "details file"

FILE_CLOUDFORMATION_TEMPLATE="${DIRECTORY_OUTPUT}/${DETAILS_STACK_NAME}.${EXTENSION_TEMPLATE}"
FILE_CLOUDFORMATION_PARAMETERS="${DIRECTORY_OUTPUT}/${DETAILS_STACK_NAME}.${EXTENSION_PARAMETERS}"
FILE_CLOUDFORMATION_TAGS="${DIRECTORY_OUTPUT}/${DETAILS_STACK_NAME}.${EXTENSION_TAGS}"
FILE_CLOUDFORMATION_DETAILS="${DIRECTORY_OUTPUT}/${DETAILS_STACK_NAME}.${EXTENSION_DETAILS}"
## END Initialize
###------------------------------------------------------------------------------------------------

###------------------------------------------------------------------------------------------------
## START Execution
line_break
log_highlight "Execution"

sam_package "${FILE_SAM_YAML}" "${TMP_FILE_CLOUDFORMATION_TEMPLATE}" "${S3_BUCKET_ARTIFACTS}" "${SAM_S3_PREFIX}" "${DETAILS_REGION}"
RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then exit_logic ${RETURNVAL};fi

exec_script "${SCRIPTS_CLOUDFORMATION_VALIDATE}" -f "${TMP_FILE_CLOUDFORMATION_TEMPLATE}" -s "${S3_BUCKET_TEMP}" -r "${DETAILS_REGION}"
RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then exit_logic ${RETURNVAL};fi
cat "${TMP_FILE_CLOUDFORMATION_TEMPLATE}" > "${FILE_CLOUDFORMATION_TEMPLATE}"

generate_file_parameters "${TMP_FILE_CLOUDFORMATION_PARAMETERS}"
cat "${TMP_FILE_CLOUDFORMATION_PARAMETERS}" > "${FILE_CLOUDFORMATION_PARAMETERS}"

generate_file_tags "${TMP_FILE_CLOUDFORMATION_TAGS}"
cat "${TMP_FILE_CLOUDFORMATION_TAGS}" > "${FILE_CLOUDFORMATION_TAGS}"

generate_file_details "${TMP_FILE_CLOUDFORMATION_DETAILS}"
cat "${TMP_FILE_CLOUDFORMATION_DETAILS}" > "${FILE_CLOUDFORMATION_DETAILS}"

line_break
log_highlight "Generated Files"
log "File (Template):   [${FILE_CLOUDFORMATION_TEMPLATE}]"
log "File (Parameters): [${FILE_CLOUDFORMATION_PARAMETERS}]"
log "File (Tags):       [${FILE_CLOUDFORMATION_TAGS}]"
log "File (Details):    [${FILE_CLOUDFORMATION_DETAILS}]"
line_break
## END Execution
###------------------------------------------------------------------------------------------------

log_success "${SELF_IDENTITY_H}: Finished"
exit_logic 0
