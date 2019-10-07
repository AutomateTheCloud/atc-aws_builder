#!/bin/bash
###------------------------------------------------------------------------------------------------
# alias:   aws_builder.s3.upload_file
# script:  upload_file.sh
# purpose: S3 (Upload File)
# version: 1.0.0
###------------------------------------------------------------------------------------------------

###------------------------------------------------------------------------------------------------
## Config
declare -r SELF_IDENTITY="s3_upload_file"
declare -r SELF_IDENTITY_H="S3 (Upload File)"
declare -a ARGUMENTS_DESCRIPTION=(
    '-b <bucket_name> : S3 Bucket Name'
    '-f <destination_file> : File in S3 Bucket'
    '-F <local_file> : Local File'
    '-s : Enable server side encryption'
    '-r : Enable Reduced Redundancy'
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
    "${LIB_FUNCTIONS_AWS_S3}"
)

###------------------------------------------------------------------------------------------------
## Variables
VARIABLES_REQUIRED=(
    's3_bucket'
    's3_file'
    'local_file'
    's3_server_side_encryption'
    's3_reduced_redundancy'
)
S3_BUCKET=""
S3_FILE=""
LOCAL_FILE=""
S3_SERVER_SIDE_ENCRYPTION=no
S3_REDUCED_REDUNDANCY=no

###------------------------------------------------------------------------------------------------
## Main
# Process Arguments
while getopts "hVb:f:F:srP:" OPTION; do
    case $OPTION in
        b) S3_BUCKET=$OPTARG;;
        f) S3_FILE=$OPTARG;;
        F) LOCAL_FILE=$OPTARG;;
        s) S3_SERVER_SIDE_ENCRYPTION=yes;;
        r) S3_REDUCED_REDUNDANCY=yes;;
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

line_break
log "- S3 Bucket:          [${S3_BUCKET}]"
log "- S3 File:            [${S3_FILE}]"
log "- Local File:         [${LOCAL_FILE}]"
log "- Encryption:         [${S3_SERVER_SIDE_ENCRYPTION}]"
log "- Reduced Redundancy: [${S3_REDUCED_REDUNDANCY}]"
## END Initialize
###------------------------------------------------------------------------------------------------

###------------------------------------------------------------------------------------------------
## START Execution
line_break
log_highlight "Execution"

s3_cp_upload "${S3_BUCKET}" "${S3_FILE}" "${LOCAL_FILE}" "${S3_SERVER_SIDE_ENCRYPTION}" "${S3_REDUCED_REDUNDANCY}"
RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then exit_logic $E_BAD_ARGS; fi
## END Execution
###------------------------------------------------------------------------------------------------

log_success "${SELF_IDENTITY_H}: Finished"
exit_logic 0
