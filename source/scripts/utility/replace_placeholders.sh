#!/bin/bash
###------------------------------------------------------------------------------------------------
# alias:   aws_builder.utility.replace_placeholders
# script:  replace_placeholders.sh
# purpose: Replace Placeholder Variables in all files in specified directory
# version: 1.0.0
###------------------------------------------------------------------------------------------------

###------------------------------------------------------------------------------------------------
## Config
declare -r SELF_IDENTITY="replace_placeholders"
declare -r SELF_IDENTITY_H="Replace Placeholders"
declare -a ARGUMENTS_DESCRIPTION=(
    '-d <directory> : Directory to Scan'
    '-p <file> : Placeholder Var file'
    '-i <file> : Placeholder Info file'
)

###------------------------------------------------------------------------------------------------
## Load Defaults
declare -r GLOBAL_CONFIG_FILE="${DIRECTORY_AWS_BUILDER:-/opt/aws_builder}/config/global.config"
source "${GLOBAL_CONFIG_FILE}" || exit 3

###------------------------------------------------------------------------------------------------
## Declare dependencies
REQUIRED_SOURCE_FILES+=()

###------------------------------------------------------------------------------------------------
## Script specific settings (non-configurable)
# Variables
VARIABLES_REQUIRED=(
    'directory_scan'
    'file_placeholder_var'
    'file_placeholder_info'
)
ARRAY_FILES=()
KEY_MAX_LENGTH=0
TMP_FILE=""
TMP_KEY=""
TMP_VAR=""

###------------------------------------------------------------------------------------------------
## Main
# Process Arguments
while getopts "hVd:p:i:" OPTION; do
    case $OPTION in
        d) DIRECTORY_SCAN=$OPTARG;;
        p) FILE_PLACEHOLDER_VAR=$OPTARG;;
        i) FILE_PLACEHOLDER_INFO=$OPTARG;;
        h) usage; exit 0;;
        V) echo "$(return_script_version "${0}")"; exit 0;;
        *) echo "ERROR: There is an error with one or more of the arguments"; usage; exit $E_BAD_ARGS;;
        ?) echo "ERROR: There is an error with one or more of the arguments"; usage; exit $E_BAD_ARGS;;
    esac
done

start_logic
log "${SELF_IDENTITY_H}: Started"

DIRECTORY_SCAN="$(echo "${DIRECTORY_SCAN}" | sed 's:/*$::')"

verify_array_key_values "VARIABLES_REQUIRED[@]"
RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then exit_logic $E_BAD_ARGS; fi

if [ ! -d "${DIRECTORY_SCAN}" ]; then
    TMP_ERROR_MSG="Scan directory does not exist [${DIRECTORY_SCAN}]"
    log_error "- ${TMP_ERROR_MSG}"
    exit_logic $E_OBJECT_NOT_FOUND "${TMP_ERROR_MSG}"
fi
if [ ! -f "${FILE_PLACEHOLDER_VAR}" ]; then
    TMP_ERROR_MSG="Placeholder Var file does not exist [${FILE_PLACEHOLDER_VAR}]"
    log_error "- ${TMP_ERROR_MSG}"
    exit_logic $E_OBJECT_NOT_FOUND "${TMP_ERROR_MSG}"
fi
if [ ! -f "${FILE_PLACEHOLDER_INFO}" ]; then
    TMP_ERROR_MSG="Placeholder Info file does not exist [${FILE_PLACEHOLDER_INFO}]"
    log_error "- ${TMP_ERROR_MSG}"
    exit_logic $E_OBJECT_NOT_FOUND "${TMP_ERROR_MSG}"
fi

line_break
log "- Scan Directory:        [${DIRECTORY_SCAN}]"
log "- Placeholder Var File:  [${FILE_PLACEHOLDER_VAR}]"
log "- Placeholder Info File: [${FILE_PLACEHOLDER_INFO}]"
line_break

log_notice "${SELF_IDENTITY_H}: Loading Variables"
source "${FILE_PLACEHOLDER_VAR}"

load_array_properties_from_file "ARRAY_PLACEHOLDER[@]" "${FILE_PLACEHOLDER_INFO}" "PLACEHOLDER"

line_break
for TMP_KEY in "${ARRAY_PLACEHOLDER[@]}"; do
    if [ ${#TMP_KEY} -gt $KEY_MAX_LENGTH ]; then
        KEY_MAX_LENGTH=${#TMP_KEY}
    fi
done
KEY_MAX_LENGTH=$((${KEY_MAX_LENGTH}+1))
for TMP_KEY in "${ARRAY_PLACEHOLDER[@]}"; do
    TMP_VAR="$(to_upper "PLACEHOLDER_${TMP_KEY}")"
    log "$(printf "%-1s %-${KEY_MAX_LENGTH}s %s" "-" "${TMP_KEY}:" "[${!TMP_VAR}]")"
done
line_break

log_notice "${SELF_IDENTITY_H}: Replacing Placeholder Targets:"
line_break
export -f log
for TMP_KEY in "${ARRAY_PLACEHOLDER[@]}"; do
    log "[${TMP_KEY}]"
    TMP_VAR="$(to_upper "PLACEHOLDER_${TMP_KEY}")"
    TMP_PLACEHOLDER="$(to_upper "ZZ_${TMP_KEY}_ZZ")"
    OLD_IFS="${IFS}"
    IFS=$'\n'
    ARRAY_FILES=($(grep -r -l "${TMP_PLACEHOLDER}" "${DIRECTORY_SCAN}/" 2>/dev/null))
    IFS="${OLD_IFS}"
    for TMP_FILE in "${ARRAY_FILES[@]}"; do
        log "- ${TMP_FILE}"
        sed -i "s|${TMP_PLACEHOLDER}|${!TMP_VAR}|g" "${TMP_FILE}"
    done
done
line_break

log_success "${SELF_IDENTITY_H}: Finished"
exit_logic 0
