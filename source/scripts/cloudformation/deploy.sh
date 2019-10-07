#!/bin/bash
###------------------------------------------------------------------------------------------------
# alias:   aws_builder.cloudformation.deploy
# script:  deploy.sh
# purpose: Deploys CloudFormation Template
# version: 1.0.0
###------------------------------------------------------------------------------------------------

###------------------------------------------------------------------------------------------------
## Config
declare -r SELF_IDENTITY="cloudformation_deploy"
declare -r SELF_IDENTITY_H="CloudFormation (Deploy)"
declare -a ARGUMENTS_DESCRIPTION=(
    '-f <file> : CloudFormation Template File'
    '-s <s3_bucket_temp> : S3 Bucket to Place Temporary files'
    '-D <dynamodb_table> : CloudFormation Manifest DynamoDB Table (optional)'
    '-R <dynamodb_region> : CloudFormation Manifest DynamoDB Region (optional)'
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
   "${LIB_FUNCTIONS_AWS_S3}"
   "${LIB_FUNCTIONS_AWS_CLOUDFORMATION}"
)

###------------------------------------------------------------------------------------------------
## Variables
VARIABLES_REQUIRED=(
    'file_template'
    's3_bucket_temp'
)

FILE_TEMPLATE=""
FILE_PARAMETERS=""
FILE_TAGS=""
FILE_TARGETS=""
FILE_DETAILS=""
REF_STACK_ID=""
DYNAMODB_MANIFEST=""
DYNAMODB_REGION=""

TMP_CHECKSUM_FILE=""
TMP_FILE_CHECKSUM_FINGERPRINT=""
CHECKSUM_CLOUDFORMATION=""

TMP_ERROR_MSG=""

###------------------------------------------------------------------------------------------------
## Main
# Process Arguments
while getopts "hVf:s:D:R:P:" OPTION; do
    case $OPTION in
        f) FILE_TEMPLATE=$OPTARG;;
        s) S3_BUCKET_TEMP=$OPTARG;;
        D) DYNAMODB_MANIFEST=$OPTARG;;
        R) DYNAMODB_REGION=$OPTARG;;
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

FILE_PARAMETERS="$(echo "${FILE_TEMPLATE}" | sed "s/.${EXTENSION_TEMPLATE}\$//g").${EXTENSION_PARAMETERS}"
FILE_TAGS="$(echo "${FILE_TEMPLATE}" | sed "s/.${EXTENSION_TEMPLATE}\$//g").${EXTENSION_TAGS}"
FILE_DETAILS="$(echo "${FILE_TEMPLATE}" | sed "s/.${EXTENSION_TEMPLATE}\$//g").${EXTENSION_DETAILS}"
FILE_TARGETS="$(echo "${FILE_TEMPLATE}" | sed "s/.${EXTENSION_TEMPLATE}\$//g").${EXTENSION_TARGETS}"

if [ ! -f "${FILE_TEMPLATE}" ]; then
    TMP_ERROR_MSG="CloudFormation Template file does not exist [${FILE_TEMPLATE}]"
    log_error "- ${TMP_ERROR_MSG}"
    exit_logic $E_OBJECT_NOT_FOUND "${TMP_ERROR_MSG}"
fi
if [ ! -f "${FILE_PARAMETERS}" ]; then
    TMP_ERROR_MSG="Parameters file does not exist. All CloudFormation Template files must have an accompanying Parameters file within the same directory [${FILE_PARAMETERS}]"
    log_error "- ${TMP_ERROR_MSG}"
    exit_logic $E_OBJECT_NOT_FOUND "${TMP_ERROR_MSG}"
fi
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
log "- CloudFormation Template:          [${FILE_TEMPLATE}]"
log "- DynamoDB Manifest:                [${DYNAMODB_MANIFEST}]"
log "- DynamoDB Region:                  [${DYNAMODB_REGION}]"

load_variables_details_from_file_keyvalue "${FILE_DETAILS}"
RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then exit_logic ${RETURNVAL};fi
display_variables_details
## END Initialize
###------------------------------------------------------------------------------------------------

###------------------------------------------------------------------------------------------------
## START Execution
line_break
log_highlight "Execution"

cloudformation_deploy REF_STACK_ID "${DETAILS_STACK_NAME}" "${FILE_TEMPLATE}" "${FILE_PARAMETERS}" "${FILE_TAGS}" "${S3_BUCKET_TEMP}" "${DETAILS_REGION}"
RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then exit_logic ${RETURNVAL};fi
line_break

if(! is_empty "${REF_STACK_ID}"); then
    log "- Stack ID:    [${REF_STACK_ID}]"
    CONSOLE_URL="https://console.aws.amazon.com/cloudformation/home?region=${DETAILS_REGION}#/stack/detail?stackId=$(echo "${REF_STACK_ID}" | sed -e 's;/;%2F;g')"
    log "- Console URL: [${CONSOLE_URL}]"
    line_break

    log_notice "${SELF_IDENTITY_H}: Monitoring status of CloudFormation Deployment"
    call_sleep 15 "allowing initial stack deployment time to start"
    cloudformation_poll_status "${DETAILS_STACK_NAME}" yes "${DETAILS_DEPLOYMENT_TIMEOUT}" 15 "${DETAILS_REGION}"
    RETURNVAL="$?"
    line_break
    log "- Console URL: [${CONSOLE_URL}]"
    line_break
    if [ ${RETURNVAL} -ne 0 ]; then exit_logic ${RETURNVAL};fi
fi

if [ -f "${FILE_TARGETS}" ]; then
    log_notice "${SELF_IDENTITY_H}: Targets File detected. Deploying Targets"
    log "- Targets File: [${FILE_TARGETS}]"
    exec_script "${SCRIPTS_CLOUDFORMATION_TARGETS}" "-f ${FILE_TARGETS}"
    RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then exit_logic ${RETURNVAL};fi
fi

if(! is_empty "${DYNAMODB_MANIFEST}"); then
    generate_temp_file TMP_FILE_CHECKSUM_FINGERPRINT "checksum fingerprint file"
    > ${TMP_FILE_CHECKSUM_FINGERPRINT}
    return_file_sha256sum "${FILE_TEMPLATE}" TMP_CHECKSUM_FILE
    echo "${TMP_CHECKSUM_FILE}" >> ${TMP_FILE_CHECKSUM_FINGERPRINT}
    return_file_sha256sum "${FILE_PARAMETERS}" TMP_CHECKSUM_FILE
    echo "${TMP_CHECKSUM_FILE}" >> ${TMP_FILE_CHECKSUM_FINGERPRINT}
    return_file_sha256sum "${FILE_TAGS}" TMP_CHECKSUM_FILE
    echo "${TMP_CHECKSUM_FILE}" >> ${TMP_FILE_CHECKSUM_FINGERPRINT}
    return_file_sha256sum "${FILE_DETAILS}" TMP_CHECKSUM_FILE
    echo "${TMP_CHECKSUM_FILE}" >> ${TMP_FILE_CHECKSUM_FINGERPRINT}
    return_file_sha256sum "${TMP_FILE_CHECKSUM_FINGERPRINT}" CHECKSUM_CLOUDFORMATION
    manifest_put_checksum "${DETAILS_STACK_NAME}" "${CHECKSUM_CLOUDFORMATION}" "${DYNAMODB_MANIFEST}" "${DYNAMODB_REGION}"
fi
## END Execution
###------------------------------------------------------------------------------------------------

log_success "${SELF_IDENTITY_H}: Finished"
exit_logic 0
