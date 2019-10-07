#!/bin/bash
###------------------------------------------------------------------------------------------------
# alias:   aws_builder.cloudformation.process
# script:  process.sh
# purpose: Processes directory of CloudFormation files
# version: 1.0.0
###------------------------------------------------------------------------------------------------

###------------------------------------------------------------------------------------------------
## Config
declare -r SELF_IDENTITY="cloudformation_process"
declare -r SELF_IDENTITY_H="CloudFormation (Process)"
declare -a ARGUMENTS_DESCRIPTION=(
    '-d <directory> : CloudFormation Source Directory'
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
   "${LIB_FUNCTIONS_AWS_CLOUDFORMATION}"
)

###------------------------------------------------------------------------------------------------
## Variables
VARIABLES_REQUIRED=(
    'directory_source_cloudformation'
    's3_bucket_temp'
)

DIRECTORY_SOURCE_CLOUDFORMATION=""
DYNAMODB_MANIFEST=""
DYNAMODB_REGION=""

ARRAY_FILE_CLOUDFORMATION=()

TMP_FILE_CLOUDFORMATION_INFO=""
TMP_OBJECT=""
TMP_FILENAME=""
TMP_CLOUDFORMATION=""
TMP_FILE_PARAMETERS=""
TMP_FILE_TAGS=""
TMP_FILE_DETAILS=""
FILE_CHECKSUM_FINGERPRINT=""
TMP_ACCOUNT_ABBR=""
TMP_PROJECT_ABBR=""
TMP_CHECKSUM_FILE=""
TMP_CHECKSUM_CLOUDFORMATION=""

###------------------------------------------------------------------------------------------------
## Main
# Process Arguments
while getopts "hVd:s:D:R:P:" OPTION; do
    case $OPTION in
        d) DIRECTORY_SOURCE_CLOUDFORMATION=$OPTARG;;
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

DIRECTORY_SOURCE_CLOUDFORMATION="$(echo "${DIRECTORY_SOURCE_CLOUDFORMATION}" | sed 's:/*$::')"

verify_array_key_values "VARIABLES_REQUIRED[@]"
RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then exit_logic $E_BAD_ARGS; fi

if [ ! -d "${DIRECTORY_SOURCE_CLOUDFORMATION}" ]; then
    TMP_ERROR_MSG="Source Directory not found [${DIRECTORY_SOURCE_CLOUDFORMATION}]"
    log_error "- ${TMP_ERROR_MSG}"
    exit_logic $E_OBJECT_NOT_FOUND "${TMP_ERROR_MSG}"
fi

line_break
log "- CloudFormation Source Directory:  [${DIRECTORY_SOURCE_CLOUDFORMATION}]"
log "- DynamoDB Manifest:                [${DYNAMODB_MANIFEST}]"
log "- DynamoDB Region:                  [${DYNAMODB_REGION}]"
## END Initialize
###------------------------------------------------------------------------------------------------

###------------------------------------------------------------------------------------------------
## START Execution
line_break
log_highlight "Execution"

log_notice "${SELF_IDENTITY_H}: Scanning for CloudFormation files"
for TMP_OBJECT in $(eval "find ${DIRECTORY_SOURCE_CLOUDFORMATION} -name '*.template.yaml' -type f 2>/dev/null"); do
    TMP_FILENAME="$(basename "${TMP_OBJECT}")"
    TMP_CLOUDFORMATION="$(trim "${TMP_FILENAME%.*}")"
    ARRAY_FILE_CLOUDFORMATION+=("${TMP_OBJECT}")
    log "- [${TMP_CLOUDFORMATION}]"
done

log_notice "${SELF_IDENTITY_H}: Processing CloudFormation files"
generate_temp_file FILE_CHECKSUM_FINGERPRINT "checksum fingerprint file"
for TMP_OBJECT in "${ARRAY_FILE_CLOUDFORMATION[@]}"; do
    TMP_CHECKSUM_FILE=""
    TMP_CHECKSUM_CLOUDFORMATION=""
    TMP_FILENAME="$(basename "${TMP_OBJECT}")"
    TMP_CLOUDFORMATION="$(trim "${TMP_FILENAME%%.*}")"
    TMP_FILE_DETAILS="$(dirname "${TMP_OBJECT}")/${TMP_CLOUDFORMATION}.details"
    load_key_value_from_file TMP_ACCOUNT_ABBR "account_abbr" "${TMP_FILE_DETAILS}"
    if(is_empty "${TMP_ACCOUNT_ABBR}"); then
        log_error "- Failed to load Account Abbreviation, aborting operation"
        exit_logic $E_OBJECT_NOT_FOUND
    fi
    load_key_value_from_file TMP_PROJECT_ABBR "project_abbr" "${TMP_FILE_DETAILS}"
    if(is_empty "${TMP_PROJECT_ABBR}"); then
        log_error "- Failed to load Project Abbreviation, aborting operation"
        exit_logic $E_OBJECT_NOT_FOUND
    fi

    log "- [${TMP_CLOUDFORMATION}]"

    TMP_FILE_PARAMETERS="$(echo "${TMP_OBJECT}" | sed "s/.${EXTENSION_TEMPLATE}\$//g").${EXTENSION_PARAMETERS}"
    TMP_FILE_TAGS="$(echo "${TMP_OBJECT}" | sed "s/.${EXTENSION_TEMPLATE}\$//g").${EXTENSION_TAGS}"
    TMP_FILE_DETAILS="$(echo "${TMP_OBJECT}" | sed "s/.${EXTENSION_TEMPLATE}\$//g").${EXTENSION_DETAILS}"

    > ${FILE_CHECKSUM_FINGERPRINT}
    return_file_sha256sum "${TMP_OBJECT}" TMP_CHECKSUM_FILE
    echo "${TMP_CHECKSUM_FILE}" >> ${FILE_CHECKSUM_FINGERPRINT}
    return_file_sha256sum "${TMP_FILE_PARAMETERS}" TMP_CHECKSUM_FILE
    echo "${TMP_CHECKSUM_FILE}" >> ${FILE_CHECKSUM_FINGERPRINT}
    return_file_sha256sum "${TMP_FILE_TAGS}" TMP_CHECKSUM_FILE
    echo "${TMP_CHECKSUM_FILE}" >> ${FILE_CHECKSUM_FINGERPRINT}
    return_file_sha256sum "${TMP_FILE_DETAILS}" TMP_CHECKSUM_FILE
    echo "${TMP_CHECKSUM_FILE}" >> ${FILE_CHECKSUM_FINGERPRINT}

    return_file_sha256sum "${FILE_CHECKSUM_FINGERPRINT}" TMP_CHECKSUM_FILE

    if(! is_empty "${DYNAMODB_MANIFEST}"); then
        manifest_get_checksum TMP_CHECKSUM_CLOUDFORMATION "${TMP_CLOUDFORMATION}" "${DYNAMODB_MANIFEST}" "${DYNAMODB_REGION}"
        log "  - [${TMP_CHECKSUM_CLOUDFORMATION}]"
    else
        TMP_CHECKSUM_CLOUDFORMATION="n/a"
    fi
    if [[ "ZZ_${TMP_CHECKSUM_FILE}" != "ZZ_${TMP_CHECKSUM_CLOUDFORMATION}" ]]; then
        exec_script "${SCRIPTS_CLOUDFORMATION_DEPLOY}" -f "${TMP_OBJECT}" -s "${S3_BUCKET_TEMP}" -D "${DYNAMODB_MANIFEST}" -R "${DYNAMODB_REGION}"
        RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then exit_logic $RETURNVAL; fi
    else
        log "   - Checksum Match, skipping update to CloudFormation"
    fi
done
## END Execution
###------------------------------------------------------------------------------------------------

log_success "${SELF_IDENTITY_H}: Finished"
exit_logic 0
