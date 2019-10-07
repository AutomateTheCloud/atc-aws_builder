#!/bin/bash
###------------------------------------------------------------------------------------------------
# alias:   aws_builder.ssl.upload_certificate
# script:  upload_certificate.sh
# purpose: SSL - Upload Certificate
# version: 1.0.0
###------------------------------------------------------------------------------------------------

###------------------------------------------------------------------------------------------------
## Config
declare -r SELF_IDENTITY="upload_certificate"
declare -r SELF_IDENTITY_H="SSL - Upload Certificate"
declare -a ARGUMENTS_DESCRIPTION=(
    '-n <name> : SSL Certificate Name'
    '-d <directory> : SSL Certificate Directory'
    '-r <region> : Region'
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
    "${LIB_FUNCTIONS_AWS_METADATA}"
    "${LIB_FUNCTIONS_AWS_SSL}"
)

###------------------------------------------------------------------------------------------------
## Variables
VARIABLES_REQUIRED=(
    'ssl_name'
    'directory_ssl'
    'aws_account_number'
    'aws_region'
)

FILE_SSL_CRT=""
FILE_SSL_CHAIN=""
FILE_SSL_KEY=""
TMP_ERROR_MSG=""

###------------------------------------------------------------------------------------------------
## Main
# Process Arguments
while getopts "hVn:d:r:P:" OPTION; do
    case $OPTION in
        n) SSL_NAME=$OPTARG;;
        d) DIRECTORY_SSL=$OPTARG;;
        r) AWS_REGION=$OPTARG;;
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

DIRECTORY_SSL="$(echo "${DIRECTORY_SSL}" | sed 's:/*$::')"

aws_metadata_account_id_from_cli AWS_ACCOUNT_NUMBER

verify_array_key_values "VARIABLES_REQUIRED[@]"
RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then exit_logic $E_BAD_ARGS; fi

if [ ! -d "${DIRECTORY_SSL}" ]; then
    TMP_ERROR_MSG="SSL Directory does not exist [${DIRECTORY_SSL}]"
    log_error "- ${TMP_ERROR_MSG}"
    exit_logic $E_OBJECT_NOT_FOUND "${TMP_ERROR_MSG}"
fi

FILE_SSL_CRT="${DIRECTORY_SSL}/${SSL_NAME}.crt"
FILE_SSL_CHAIN="${DIRECTORY_SSL}/${SSL_NAME}.chain"
FILE_SSL_KEY="${DIRECTORY_SSL}/${SSL_NAME}.key"

if [ ! -f "${FILE_SSL_CRT}" ]; then
    TMP_ERROR_MSG="SSL Certificate file not found [expected: ${FILE_SSL_CRT}]"
    log_error "- ${TMP_ERROR_MSG}"
    exit_logic $E_OBJECT_NOT_FOUND "${TMP_ERROR_MSG}"
fi
if [ ! -f "${FILE_SSL_CHAIN}" ]; then
    TMP_ERROR_MSG="SSL Chain file not found [expected: ${FILE_SSL_CHAIN}]"
    log_error "- ${TMP_ERROR_MSG}"
    exit_logic $E_OBJECT_NOT_FOUND "${TMP_ERROR_MSG}"
fi
if [ ! -f "${FILE_SSL_KEY}" ]; then
    TMP_ERROR_MSG="SSL Certificate file not found [expected: ${FILE_SSL_KEY}]"
    log_error "- ${TMP_ERROR_MSG}"
    exit_logic $E_OBJECT_NOT_FOUND "${TMP_ERROR_MSG}"
fi

PARAMETER_DESCRIPTION="SSL Certificate: ${SSL_NAME} - [${AWS_REGION}]"

line_break
log "- SSL Certificate Name:   [${SSL_NAME}]"
log "- SSL Directory:          [${DIRECTORY_SSL}]"
log "- AWS Account Number:     [${AWS_ACCOUNT_NUMBER}]"
log "- Region:                 [${AWS_REGION}]"
## END Initialize
###------------------------------------------------------------------------------------------------

###------------------------------------------------------------------------------------------------
## START Execution
line_break
log_highlight "Execution"

iam_import_certificate "${SSL_NAME}" "${FILE_SSL_CRT}" "${FILE_SSL_CHAIN}" "${FILE_SSL_KEY}" "${AWS_REGION}"
RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then exit_logic $RETURNVAL; fi

line_break
acm_import_certificate "${SSL_NAME}" "${FILE_SSL_CRT}" "${FILE_SSL_CHAIN}" "${FILE_SSL_KEY}" "${AWS_REGION}"
RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then exit_logic $RETURNVAL; fi
## END Execution
###------------------------------------------------------------------------------------------------

log_success "${SELF_IDENTITY_H}: Finished"
exit_logic 0
