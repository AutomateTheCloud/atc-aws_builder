#!/bin/bash
###------------------------------------------------------------------------------------------------
# alias:   aws_builder.ec2.generate_ssh_keypair
# script:  generate_ssh_keypair.sh
# purpose: EC2 (Generate SSH Keypair)
# version: 1.0.0
###------------------------------------------------------------------------------------------------

###------------------------------------------------------------------------------------------------
## Config
declare -r SELF_IDENTITY="generate_ssh_keypair"
declare -r SELF_IDENTITY_H="EC2 (Generate SSH Keypair)"
declare -a ARGUMENTS_DESCRIPTION=(
    '-p <purpose_identifier> : Purpose Identifier (ec2-user, app_name_abbr, etc)'
    '-o <organization_abbr> : Organization Abbreviation'
    '-d <directory> : Output Directory'
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
    "${LIB_FUNCTIONS_AWS_EC2}"
)

###------------------------------------------------------------------------------------------------
## Variables
VARIABLES_REQUIRED=(
    'purpose_identifier'
    'organization_abbr'
    'directory_output'
    'aws_region'
)

TMP_UUID=""
KEYPAIR_NAME=""
FILE_KEYPAIR_PEM=""
FILE_KEYPAIR_PUB=""
FILE_KEYPAIR_PPK=""

OPERATION_FILE_LOG=""

###------------------------------------------------------------------------------------------------
## Main
# Process Arguments
while getopts "hVp:o:d:r:P:" OPTION; do
    case $OPTION in
        p) PURPOSE_IDENTIFIER=$OPTARG;;
        o) ORGANIZATION_ABBR=$OPTARG;;
        d) DIRECTORY_OUTPUT=$OPTARG;;
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

generate_uuid TMP_UUID 4
KEYPAIR_NAME="${PURPOSE_IDENTIFIER}_${ORGANIZATION_ABBR}_${AWS_REGION}_$($(which date) +%Y-%m-%d)_${TMP_UUID}"
FILE_KEYPAIR_PEM="${DIRECTORY_OUTPUT}/${KEYPAIR_NAME}.pem"
FILE_KEYPAIR_PUB="${DIRECTORY_OUTPUT}/${KEYPAIR_NAME}.pem.pub"
FILE_KEYPAIR_PPK="${DIRECTORY_OUTPUT}/${KEYPAIR_NAME}.ppk"

line_break
log "- Purpose Identifier:     [${PURPOSE_IDENTIFIER}]"
log "- Organization Abbr:      [${ORGANIZATION_ABBR}]"
log "- Directory (Output):     [${DIRECTORY_OUTPUT}]"
log "- Region:                 [${AWS_REGION}]"
line_break
log "- Keypair Name:           [${KEYPAIR_NAME}]"
log "- File (Keypair Private): [${FILE_KEYPAIR_PEM}]"
log "- File (Keypair Public):  [${FILE_KEYPAIR_PUB}]"
if which puttygen > /dev/null 2>&1; then
    log "- File (Keypair PPK):     [${FILE_KEYPAIR_PPK}]"
fi
## END Initialize
###------------------------------------------------------------------------------------------------

###------------------------------------------------------------------------------------------------
## START Execution
line_break
log_highlight "Execution"

generate_temp_file OPERATION_FILE_LOG "operation output log"

log_notice "${SELF_IDENTITY_H}: Generating Keypair"
> ${OPERATION_FILE_LOG}
$(which ssh-keygen) -t rsa -b 4096 -N '' -C '' -f "${FILE_KEYPAIR_PEM}" > ${OPERATION_FILE_LOG} 2>&1
RETURNVAL="$?"
if [ ${RETURNVAL} -ne 0 ]; then
    log_add_from_file "${OPERATION_FILE_LOG}" "${SELF_IDENTITY_H}: Data containing error" 200000
    TMP_ERROR_MSG="Failed to generate Keypair [ssh-keygen_returned: ${RETURNVAL}]"
    log_error "- ${TMP_ERROR_MSG}"
    exit_logic $E_OBJECT_FAILED_TO_CREATE "${TMP_ERROR_MSG}"
fi
if [ ! -f "${FILE_KEYPAIR_PEM}" ]; then
    log_add_from_file "${OPERATION_FILE_LOG}" "${SELF_IDENTITY_H}: Data containing error" 200000
    TMP_ERROR_MSG="Private Keypair file does not exist [${FILE_KEYPAIR_PEM}]"
    log_error "- ${TMP_ERROR_MSG}"
    exit_logic $E_OBJECT_FAILED_TO_CREATE "${TMP_ERROR_MSG}"
fi
if [ ! -f "${FILE_KEYPAIR_PUB}" ]; then
    log_add_from_file "${OPERATION_FILE_LOG}" "${SELF_IDENTITY_H}: Data containing error" 200000
    TMP_ERROR_MSG="Public Keypair file does not exist [${FILE_KEYPAIR_PUB}]"
    log_error "- ${TMP_ERROR_MSG}"
    exit_logic $E_OBJECT_FAILED_TO_CREATE "${TMP_ERROR_MSG}"
fi

if which puttygen > /dev/null 2>&1; then
    log_notice "${SELF_IDENTITY_H}: Generating PPK file"
    > ${OPERATION_FILE_LOG}
    $(which puttygen) ${FILE_KEYPAIR_PEM} -o ${FILE_KEYPAIR_PPK} -O private > ${OPERATION_FILE_LOG} 2>&1
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_add_from_file "${OPERATION_FILE_LOG}" "${SELF_IDENTITY_H}: Data containing error" 200000
        TMP_ERROR_MSG="Failed to generate PPK [puttygen_returned: ${RETURNVAL}]"
        log_error "- ${TMP_ERROR_MSG}"
        exit_logic $E_OBJECT_FAILED_TO_CREATE "${TMP_ERROR_MSG}"
    fi
else
    log_warning "${SELF_IDENTITY_H}: Puttygen not installed, skipping PPK generation"
fi

ec2_import_keypair "${KEYPAIR_NAME}" "${FILE_KEYPAIR_PUB}" "${AWS_REGION}"
RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then exit_logic $RETURNVAL; fi
## END Execution
###------------------------------------------------------------------------------------------------

log_success "${SELF_IDENTITY_H}: Finished"
exit_logic 0
