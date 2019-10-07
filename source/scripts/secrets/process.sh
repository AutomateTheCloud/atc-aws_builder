#!/bin/bash
###------------------------------------------------------------------------------------------------
# alias:   aws_builder.secrets.process
# script:  process.sh
# purpose: Batch processes directory of secrets files
# version: 1.0.0
###------------------------------------------------------------------------------------------------

###------------------------------------------------------------------------------------------------
## Config
declare -r SELF_IDENTITY="secrets_process"
declare -r SELF_IDENTITY_H="Secrets (Process)"
declare -a ARGUMENTS_DESCRIPTION=(
    '-d <directory> : Secrets Source Directory'
    '-D <dynamodb_table> : Secrets Manifest DynamoDB Table (optional)'
    '-R <dynamodb_region> : Secrets Manifest DynamoDB Region (optional)'
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
    'directory_source_secrets'
)

DIRECTORY_SOURCE_SECRETS=""

ARRAY_FILE_SECRETS=()

DYNAMODB_MANIFEST=""
DYNAMODB_REGION=""

TMP_FILE_SECRETS_INFO=""
TMP_OBJECT=""
TMP_FILENAME=""
TMP_SECRET=""
TMP_PROJECT_ABBR=""
TMP_CHECKSUM_FILE=""
TMP_CHECKSUM_SECRET=""

###------------------------------------------------------------------------------------------------
## Main
# Process Arguments
while getopts "hVd:D:R:P:" OPTION; do
    case $OPTION in
        d) DIRECTORY_SOURCE_SECRETS=$OPTARG;;
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

DIRECTORY_SOURCE_SECRETS="$(echo "${DIRECTORY_SOURCE_SECRETS}" | sed 's:/*$::')"

verify_array_key_values "VARIABLES_REQUIRED[@]"
RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then exit_logic $E_BAD_ARGS; fi

if [ ! -d "${DIRECTORY_SOURCE_SECRETS}" ]; then
    TMP_ERROR_MSG="Source Directory not found [${DIRECTORY_SOURCE_SECRETS}]"
    log_error "- ${TMP_ERROR_MSG}"
    exit_logic $E_OBJECT_NOT_FOUND "${TMP_ERROR_MSG}"
fi

line_break
log "- Secrets Source Directory: [${DIRECTORY_SOURCE_SECRETS}]"
log "- DynamoDB Manifest:        [${DYNAMODB_MANIFEST}]"
log "- DynamoDB Region:          [${DYNAMODB_REGION}]"
## END Initialize
###------------------------------------------------------------------------------------------------

###------------------------------------------------------------------------------------------------
## START Execution
line_break
log_highlight "Execution"

log_notice "${SELF_IDENTITY_H}: Scanning for Secrets"
for TMP_OBJECT in $(eval "find ${DIRECTORY_SOURCE_SECRETS} -name '*.secrets' -type f 2>/dev/null"); do
    TMP_FILENAME="$(basename "${TMP_OBJECT}")"
    TMP_SECRET="$(trim "${TMP_FILENAME%.*}")"
    ARRAY_FILE_SECRETS+=("${TMP_OBJECT}")
    log "- [${TMP_SECRET}]"
done

log_notice "${SELF_IDENTITY_H}: Processing Secrets"
for TMP_OBJECT in "${ARRAY_FILE_SECRETS[@]}"; do
    TMP_CHECKSUM_FILE=""
    TMP_CHECKSUM_SECRET=""
    TMP_FILENAME="$(basename "${TMP_OBJECT}")"
    TMP_SECRET="$(trim "${TMP_FILENAME%.*}")"
    TMP_ACCOUNT_ABBR="$(cat "${TMP_OBJECT}" 2>/dev/null | sed -e 's/^[ \t]*//' | grep "^Account:" | awk -F"Account:" '{print $2}' | awk -F\| '{print $2}' | sed -e 's/^ *//g' -e 's/ *$//g')"
    if(is_empty "${TMP_ACCOUNT_ABBR}"); then
        log_error "- Failed to load Account Abbreviation, aborting operation"
        exit_logic $E_OBJECT_NOT_FOUND
    fi
    TMP_PROJECT_ABBR="$(cat "${TMP_OBJECT}" 2>/dev/null | sed -e 's/^[ \t]*//' | grep "^Project:" | awk -F"Project:" '{print $2}' | awk -F\| '{print $2}' | sed -e 's/^ *//g' -e 's/ *$//g')"
    if(is_empty "${TMP_PROJECT_ABBR}"); then
        log_error "- Failed to load Project Abbreviation, aborting operation"
        exit_logic $E_OBJECT_NOT_FOUND
    fi

    log "- [${TMP_SECRET}]"
    return_file_sha256sum "${TMP_OBJECT}" TMP_CHECKSUM_FILE

    if(! is_empty "${DYNAMODB_MANIFEST}"); then
        manifest_get_checksum TMP_CHECKSUM_SECRET "${TMP_SECRET}" "${DYNAMODB_MANIFEST}" "${DYNAMODB_REGION}"
        log "  - [${TMP_CHECKSUM_SECRET}]"
    else
        TMP_CHECKSUM_SECRET="n/a"
    fi
    
    if [[ "ZZ_${TMP_CHECKSUM_FILE}" != "ZZ_${TMP_CHECKSUM_SECRET}" ]]; then
        exec_script "${SCRIPTS_SECRETS_DEPLOY}" -f "${TMP_OBJECT}" -D "${DYNAMODB_MANIFEST}" -R "${DYNAMODB_REGION}"
        RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then exit_logic $RETURNVAL; fi
    else
        log "   - Checksum Match, skipping update to Secrets"
    fi
done
## END Execution
###------------------------------------------------------------------------------------------------

log_success "${SELF_IDENTITY_H}: Finished"
exit_logic 0
