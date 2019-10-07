#!/bin/bash
###------------------------------------------------------------------------------------------------
# alias:   aws_builder.kms.encrypt_file
# script:  encrypt_file.sh
# purpose: KMS - Encrypt File
# version: 1.0.0
###------------------------------------------------------------------------------------------------

###------------------------------------------------------------------------------------------------
## Config
declare -r SELF_IDENTITY="encrypt_file"
declare -r SELF_IDENTITY_H="KMS - Encrypt File"
declare -a ARGUMENTS_DESCRIPTION=(
    '-f <file> : File to encrypt'
    '-d <directory> : Output Directory'
    '-k <kms_key_id> : KMS Key ID'
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
    "${LIB_FUNCTIONS_AWS_KMS}"
)

###------------------------------------------------------------------------------------------------
## Variables
VARIABLES_REQUIRED=(
    'file_target'
    'directory_output'
    'kms_key_id'
    'aws_region'
)

TMP_ERROR_MSG=""
KMS_ENVELOPE_KEY=""

###------------------------------------------------------------------------------------------------
## Main
# Process Arguments
while getopts "hVf:d:k:r:P:" OPTION; do
    case $OPTION in
        f) FILE_TARGET=$OPTARG;;
        d) DIRECTORY_OUTPUT=$OPTARG;;
        k) KMS_KEY_ID=$OPTARG;;
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

DIRECTORY_OUTPUT="$(echo "${DIRECTORY_OUTPUT}" | sed 's:/*$::')"

verify_array_key_values "VARIABLES_REQUIRED[@]"
RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then exit_logic $E_BAD_ARGS; fi

if [ ! -d "${DIRECTORY_OUTPUT}" ]; then
    TMP_ERROR_MSG="Output Directory does not exist [${DIRECTORY_OUTPUT}]"
    log_error "- ${TMP_ERROR_MSG}"
    exit_logic $E_OBJECT_NOT_FOUND "${TMP_ERROR_MSG}"
fi

line_break
log "- File:                   [${FILE_TARGET}]"
log "- Output Directory:       [${DIRECTORY_OUTPUT}]"
log "- KMS Key ID:             [${KMS_KEY_ID}]"
log "- Region:                 [${AWS_REGION}]"
## END Initialize
###------------------------------------------------------------------------------------------------

###------------------------------------------------------------------------------------------------
## START Execution
line_break
log_highlight "Execution"

kms_encrypt_file "${FILE_TARGET}" "${DIRECTORY_OUTPUT}" "${KMS_KEY_ID}" "${AWS_REGION}"
RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then exit_logic $RETURNVAL; fi
## END Execution
###------------------------------------------------------------------------------------------------

log_success "${SELF_IDENTITY_H}: Finished"
exit_logic 0
