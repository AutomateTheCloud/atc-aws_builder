#!/bin/bash
###------------------------------------------------------------------------------------------------
# alias:   aws_builder.cloudformation.targets
# script:  targets.sh
# purpose: Allows for additional processing which cannot occur during CloudFormation deployment
# version: 1.0.0
###------------------------------------------------------------------------------------------------

###------------------------------------------------------------------------------------------------
## Config
declare -r SELF_IDENTITY="cloudformation_targets"
declare -r SELF_IDENTITY_H="CloudFormation (Targets)"
declare -a ARGUMENTS_DESCRIPTION=(
    '-f <file> : CloudFormation Targets File'
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
   "${LIB_FUNCTIONS_AWS_EC2}"
   "${LIB_FUNCTIONS_AWS_VPC}"
   "${LIB_FUNCTIONS_AWS_CLOUDFORMATION}"
)

###------------------------------------------------------------------------------------------------
## Variables
VARIABLES_REQUIRED=(
    'file_targets'
)

FILE_TAGS=""
FILE_TARGETS=""
FILE_DETAILS=""

TMP_RESOURCE_NAME=""
TMP_RESOURCE_TYPE=""
TMP_RESOURCE_ID=""

TMP_ERROR_MSG=""
TMP_FILE_CLOUDFORMATION_OUTPUT=""

###------------------------------------------------------------------------------------------------
## Main
# Process Arguments
while getopts "hVf:P:" OPTION; do
    case $OPTION in
        f) FILE_TARGETS=$OPTARG;;
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

verify_array_key_values "VARIABLES_REQUIRED[@]"
RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then exit_logic $E_BAD_ARGS; fi

if [ ! -f "${FILE_TARGETS}" ]; then
    TMP_ERROR_MSG="CloudFormation Target file does not exist [${FILE_TARGETS}]"
    log_error "- ${TMP_ERROR_MSG}"
    exit_logic $E_OBJECT_NOT_FOUND "${TMP_ERROR_MSG}"
fi
if ! $(echo "${FILE_TARGETS}" | grep "${EXTENSION_TARGETS}\$" >/dev/null 2>&1); then
    TMP_ERROR_MSG="Specified file does not appear to be a CloudFormation Target file, does not end with '.${EXTENSION_TARGETS}' [${FILE_TARGETS}]"
    log_error "- ${TMP_ERROR_MSG}"
    exit_logic $E_OBJECT_NOT_FOUND "${TMP_ERROR_MSG}"
fi

FILE_TAGS="$(echo "${FILE_TARGETS}" | sed "s/.${EXTENSION_TARGETS}\$//g").${EXTENSION_TAGS}"
FILE_DETAILS="$(echo "${FILE_TARGETS}" | sed "s/.${EXTENSION_TARGETS}\$//g").${EXTENSION_DETAILS}"

if [ ! -f "${FILE_TAGS}" ]; then
    TMP_ERROR_MSG="Tags file does not exist. All CloudFormation Template files must have an accompanying Tags file within the same directory [${FILE_TAGS}]"
    log_error "- ${TMP_ERROR_MSG}"
    exit_logic $E_OBJECT_NOT_FOUND "${TMP_ERROR_MSG}"
fi
if [ ! -f "${FILE_DETAILS}" ]; then
    TMP_ERROR_MSG="Details file does not exist. All CloudFormation Template files must have an accompanying Details file within the same directory [${FILE_DETAILS}]"
    log_error "- ${TMP_ERROR_MSG}"
    exit_logic $E_OBJECT_NOT_FOUND "${TMP_ERROR_MSG}"
fi

line_break
log "- CloudFormation Targets: [${FILE_TARGETS}]"

load_variables_details_from_file_keyvalue "${FILE_DETAILS}"
RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then exit_logic ${RETURNVAL};fi
display_variables_details

generate_temp_file TMP_FILE_CLOUDFORMATION_OUTPUT "CloudFormation Stack Output"
cloudformation_get_outputs "${TMP_FILE_CLOUDFORMATION_OUTPUT}" "${DETAILS_STACK_NAME}" "${DETAILS_REGION}"
RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then exit_logic ${RETURNVAL};fi

log_notice "${SELF_IDENTITY_H}: Expanding Target variables"
load_targets "${FILE_TARGETS}"
## END Initialize
###------------------------------------------------------------------------------------------------

###------------------------------------------------------------------------------------------------
## START Execution
line_break
log_highlight "Execution"

log_notice "${SELF_IDENTITY_H}: Processing Targets"
line_break
for TMP_OBJECT in "${ARRAY_TARGETS_RESOURCES[@]}"; do
    TMP_TARGET_ACTION="$(echo "${TMP_OBJECT}" | awk -F '|' '{print $1}')"
    TMP_RESOURCE_NAME="$(echo "${TMP_OBJECT}" | awk -F '|' '{print $2}')"
    load_property_from_file TMP_RESOURCE_ID "${TMP_RESOURCE_NAME}" "${TMP_FILE_CLOUDFORMATION_OUTPUT}"
    if(is_empty "${TMP_RESOURCE_ID}"); then
        log "  - Resource ID not present, skipping [${TMP_RESOURCE_NAME} / ${TMP_TARGET_ACTION}]"
    else
        case "${TMP_TARGET_ACTION}" in
            tag_ec2)
                TMP_TAG_KEY="$(echo "${TMP_OBJECT}" | awk -F '|' '{print $3}')"
                TMP_TAG_VALUE="$(echo "${TMP_OBJECT}" | awk -F '|' '{print $4}')"

                ec2_tag_resource "${TMP_RESOURCE_ID}" "${TMP_TAG_KEY}" "${TMP_TAG_VALUE}" "${DETAILS_REGION}" "${DETAILS_ACCOUNT_ABBR}"
                RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then log_warning "  - Failed to tag resource [${TMP_RESOURCE_NAME} / ${TMP_RESOURCE_TYPE} / ${TMP_RESOURCE_ID}]";fi

                ec2_tag_resource_from_file "${TMP_RESOURCE_ID}" "${FILE_TAGS}" "${DETAILS_REGION}" "${DETAILS_ACCOUNT_ABBR}"
                RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then log_warning "  - Failed to tag resource [${TMP_RESOURCE_NAME} / ${TMP_RESOURCE_TYPE} / ${TMP_RESOURCE_ID}]";fi
                ;;
            harden_default_security_group)
                ec2_harden_default_security_group "${TMP_RESOURCE_ID}" "${DETAILS_REGION}"
                ;;
            modify_vpc_peering_connection)
                vpc_peering_connection_enable_dns "${TMP_RESOURCE_ID}" "${DETAILS_REGION}" "${DETAILS_ACCOUNT_ABBR}"
                RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then log_warning "  - Failed to modify resource [${TMP_RESOURCE_NAME} / ${TMP_RESOURCE_TYPE} / ${TMP_RESOURCE_ID}]";fi
                ;;
            *)  log_warning "  - Unsupported Resource Type [${TMP_RESOURCE_NAME} / ${TMP_TARGET_ACTION}]";;
            ?)  log_warning "  - Unsupported Resource Type [${TMP_RESOURCE_NAME} / ${TMP_TARGET_ACTION}]";;
        esac
    fi
    line_break
done
## END Execution
###------------------------------------------------------------------------------------------------

log_success "${SELF_IDENTITY_H}: Finished"
exit_logic 0
