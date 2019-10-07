#!/bin/bash
###------------------------------------------------------------------------------------------------
# alias:   aws_builder.secrets.deploy
# script:  deploy.sh
# purpose: Deploys secrets file and uploads data to AWS as Encrypted SSM Parameters
# version: 1.0.0
###------------------------------------------------------------------------------------------------

###------------------------------------------------------------------------------------------------
## Config
declare -r SELF_IDENTITY="secrets_deploy"
declare -r SELF_IDENTITY_H="Secrets (Deploy)"
declare -a ARGUMENTS_DESCRIPTION=(
    '-f <file> : Secrets File'
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
   "${LIB_FUNCTIONS_AWS_SSM}"
   "${LIB_FUNCTIONS_AWS_CLOUDFORMATION}"
)

###------------------------------------------------------------------------------------------------
## Variables
VARIABLES_REQUIRED=(
    'file_secrets'
)

DIRECTORY_SECRETS_FILES=""

FILE_SECRETS=""

CHECKSUM_SECRETS=""
DYNAMODB_MANIFEST=""
DYNAMODB_REGION=""

ARRAY_SECRETS_VARS=()
ARRAY_SECRETS_FILES=()

ARN_KMS_KEY=""

PARAMETER_PATH=""
PARAMETER_PATH_VARS=""
PARAMETER_PATH_FILES=""
PARAMETER_DESCRIPTION=""

TMP_STRING=""
TMP_KEY=""
TMP_VALUE=""
KEY_MAX_LENGTH=0
TMP_ERROR_MSG=""

###------------------------------------------------------------------------------------------------
## Main
# Process Arguments
while getopts "hVf:D:R:P:" OPTION; do
    case $OPTION in
        f) FILE_SECRETS=$OPTARG;;
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

if [ ! -f "${FILE_SECRETS}" ]; then
    TMP_ERROR_MSG="Secrets file does not exist [${FILE_SECRETS}]"
    log_error "- ${TMP_ERROR_MSG}"
    exit_logic $E_OBJECT_NOT_FOUND "${TMP_ERROR_MSG}"
fi
if [ "${FILE_SECRETS##*.}" != "${EXTENSION_SECRETS}" ]; then
    TMP_ERROR_MSG="Specified file does not appear to be a secrets file, does not end with '.${EXTENSION_SECRETS}' [${FILE_SECRETS}]"
    log_error "- ${TMP_ERROR_MSG}"
    exit_logic $E_OBJECT_NOT_FOUND "${TMP_ERROR_MSG}"
fi
DIRECTORY_SECRETS_FILES="$($(which dirname) "${FILE_SECRETS}")/files"

line_break
log "- Secrets:                  [${FILE_SECRETS}]"
log "- DynamoDB Manifest:        [${DYNAMODB_MANIFEST}]"
log "- DynamoDB Region:          [${DYNAMODB_REGION}]"
line_break

load_variables_details_from_file "${FILE_SECRETS}"
RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then exit_logic ${RETURNVAL}; fi
display_variables_details

ARN_KMS_KEY="arn:aws:kms:${DETAILS_REGION}:${DETAILS_ACCOUNT_NUMBER}:alias/data-${DETAILS_ORGANIZATION_ABBR}-${DETAILS_REGION}"
PARAMETER_PATH="/secrets/${DETAILS_PROJECT_ABBR}/${DETAILS_FUNCTION_ABBR}/${DETAILS_ENVIRONMENT}"
PARAMETER_DESCRIPTION="${DETAILS_PROJECT_NAME}: ${DETAILS_FUNCTION_NAME} (${DETAILS_ENVIRONMENT}) - [${DETAILS_ORGANIZATION_ABBR} / ${DETAILS_REGION}]"

log_highlight "Variables: KMS"
log "- KMS Key (Data):      [${ARN_KMS_KEY}]"
line_break

log_highlight "Variables: Parameter Config"
PARAMETER_PATH_VARS="${PARAMETER_PATH}/vars"
PARAMETER_PATH_FILES="${PARAMETER_PATH}/files"

log "- Parameter Path:  [${PARAMETER_PATH}]"
log "- Parameter Desc:  [${PARAMETER_DESCRIPTION}]"
line_break

log_notice "${SELF_IDENTITY_H}: Loading required variables [Secrets]"
OLD_IFS="${IFS}"
IFS=$'\n'
for CURRENT_LINE in $(cat "${FILE_SECRETS}" | grep -v "^#\|^;" | grep "^\[" | sed -e 's/[ \t]*$//' -e '/^$/d'); do
    IFS="${OLD_IFS}"
    TMP_STRING="$(trim "${CURRENT_LINE}")"
    if(! is_empty "${TMP_STRING}"); then
        if [[ "${TMP_STRING}" =~ ^\[FILE_.* ]]; then
            ARRAY_SECRETS_FILES+=("${TMP_STRING}")
        else
            ARRAY_SECRETS_VARS+=("${TMP_STRING}")
        fi

    fi
    IFS=$'\n'
done
IFS="${OLD_IFS}"

line_break
log_highlight "Parameter List - Vars"
for TMP_STRING in "${ARRAY_SECRETS_VARS[@]}"; do
    TMP_KEY="$(trim "$(echo "${TMP_STRING}" | awk -F']=' '{print $1}' | sed -e 's/\[//g')")"
    if [ ${#TMP_KEY} -gt $KEY_MAX_LENGTH ]; then
        KEY_MAX_LENGTH=${#TMP_KEY}
    fi
done
KEY_MAX_LENGTH=$((${KEY_MAX_LENGTH}+1))
for TMP_STRING in "${ARRAY_SECRETS_VARS[@]}"; do
    TMP_KEY="$(trim "$(echo "${TMP_STRING}" | awk -F']=' '{print $1}' | sed -e 's/\[//g')")"
    TMP_VALUE="$(trim "$(echo "${TMP_STRING}" | awk -F']=' '{print $2}')")"
    if $(echo "${TMP_VALUE}" | grep '*$' >/dev/null 2>&1); then
        log "$(printf "%-1s %-${KEY_MAX_LENGTH}s %s" "-" "${TMP_KEY}:" "[${TMP_VALUE//?/*}]")"
    else
        log "$(printf "%-1s %-${KEY_MAX_LENGTH}s %s" "-" "${TMP_KEY}:" "[${TMP_VALUE}]")"
    fi
done

if [ ${#ARRAY_SECRETS_FILES[@]} -gt 0 ]; then
    line_break
    log_highlight "Parameter List - Files"
    for TMP_STRING in "${ARRAY_SECRETS_FILES[@]}"; do
        TMP_KEY="$(trim "$(echo "${TMP_STRING}" | awk -F']=' '{print $1}' | sed -e 's/\[//g')")"
        if [ ${#TMP_KEY} -gt $KEY_MAX_LENGTH ]; then
            KEY_MAX_LENGTH=${#TMP_KEY}
        fi
    done
    KEY_MAX_LENGTH=$((${KEY_MAX_LENGTH}+1))
    for TMP_STRING in "${ARRAY_SECRETS_FILES[@]}"; do
        TMP_KEY="$(trim "$(echo "${TMP_STRING}" | awk -F']=' '{print $1}' | sed -e 's/\[//g')")"
        TMP_VALUE="$(trim "$(echo "${TMP_STRING}" | awk -F']=' '{print $2}')")"
        if [ ! -f "${DIRECTORY_SECRETS_FILES}/${TMP_VALUE}" ]; then
            log_error "- File not found (Key: [${TMP_KEY##FILE_}] / Value: [${DIRECTORY_SECRETS_FILES}/${TMP_VALUE}]"
            exit_logic $E_OBJECT_NOT_FOUND
        fi
        if $(echo "${TMP_VALUE}" | grep '*$' >/dev/null 2>&1); then
            log "$(printf "%-1s %-${KEY_MAX_LENGTH}s %s" "-" "${TMP_KEY##FILE_}:" "[${TMP_VALUE//?/*}]")"
        else
            log "$(printf "%-1s %-${KEY_MAX_LENGTH}s %s" "-" "${TMP_KEY##FILE_}:" "[${TMP_VALUE}]")"
        fi
    done
fi
## END Initialize
###------------------------------------------------------------------------------------------------

###------------------------------------------------------------------------------------------------
## START Execution
line_break
log_highlight "Execution"

log_notice "${SELF_IDENTITY_H}: Performing cleanup of parameter path"
parameter_path_delete "${PARAMETER_PATH}" "${DETAILS_REGION}"
RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then exit_logic ${RETURNVAL}; fi

log_notice "${SELF_IDENTITY_H}: Processing Parameters - Vars"
for TMP_STRING in "${ARRAY_SECRETS_VARS[@]}"; do
    TMP_KEY="$(trim "$(echo "${TMP_STRING}" | awk -F']=' '{print $1}' | sed -e 's/\[//g')")"
    TMP_VALUE="$(trim "$(echo "${TMP_STRING}" | awk -F']=' '{print $2}')")"
    if(! is_empty "${TMP_VALUE}"); then
        parameter_put "${PARAMETER_PATH_VARS}/${TMP_KEY}" "${TMP_VALUE}" "${ARN_KMS_KEY}" "${PARAMETER_DESCRIPTION}" no "${DETAILS_REGION}"
        RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then exit_logic ${RETURNVAL}; fi
    fi
done

if [ ${#ARRAY_SECRETS_FILES[@]} -gt 0 ]; then
    line_break
    log_notice "${SELF_IDENTITY_H}: Processing Parameters - Files"
    for TMP_STRING in "${ARRAY_SECRETS_FILES[@]}"; do
        TMP_KEY="$(trim "$(echo "${TMP_STRING}" | awk -F']=' '{print $1}' | sed -e 's/\[//g')")"
        TMP_VALUE="$(trim "$(echo "${TMP_STRING}" | awk -F']=' '{print $2}')")"
        log "- ${TMP_KEY##FILE_} / ${TMP_VALUE}"
        if(! is_empty "${TMP_VALUE}"); then
            parameter_put_file_multi_part "${PARAMETER_PATH_FILES}/${TMP_KEY##FILE_}" "${DIRECTORY_SECRETS_FILES}/${TMP_VALUE}" "${ARN_KMS_KEY}" "${PARAMETER_DESCRIPTION}" no "${DETAILS_REGION}"
            RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then exit_logic ${RETURNVAL}; fi
        fi
    done
fi

if(! is_empty "${DYNAMODB_MANIFEST}"); then
    return_file_sha256sum "${FILE_SECRETS}" CHECKSUM_SECRETS
    manifest_put_checksum "${DETAILS_STACK_NAME}" "${CHECKSUM_SECRETS}" "${DYNAMODB_MANIFEST}" "${DYNAMODB_REGION}"
fi

parameter_tag_path "${PARAMETER_PATH}" "Organization:${DETAILS_ORGANIZATION_NAME};Project:${DETAILS_PROJECT_NAME};Function:${DETAILS_FUNCTION_NAME};Environment:${DETAILS_ENVIRONMENT};Owner:${DETAILS_OWNER};Contact:${DETAILS_CONTACT}" "${DETAILS_REGION}"
## END Execution
###------------------------------------------------------------------------------------------------

log_success "${SELF_IDENTITY_H}: Finished"
exit_logic 0
