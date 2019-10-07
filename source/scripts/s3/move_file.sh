#!/bin/bash
###------------------------------------------------------------------------------------------------
# alias:   aws_builder.s3.move_file
# script:  move_file.sh
# purpose: S3 (Move File)
# version: 1.0.0
###------------------------------------------------------------------------------------------------

###------------------------------------------------------------------------------------------------
## Config
declare -r SELF_IDENTITY="s3_move_file"
declare -r SELF_IDENTITY_H="S3 (Move File)"
declare -a ARGUMENTS_DESCRIPTION=(
    '-b <bucket_name> : S3 Bucket Name'
    '-f <source_file> : File in S3 Bucket (Source)'
    '-F <target_file> : File in S3 Bucket (Target)'
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
    's3_file_source'
    's3_file_target'
    's3_server_side_encryption'
    's3_reduced_redundancy'
)
S3_BUCKET=""
S3_FILE_SOURCE=""
S3_FILE_TARGET=""
LOCAL_FILE=""
S3_SERVER_SIDE_ENCRYPTION=no
S3_REDUCED_REDUNDANCY=no

###------------------------------------------------------------------------------------------------
## Main
# Process Arguments
while getopts "hVb:f:F:srP:" OPTION; do
    case $OPTION in
        b) S3_BUCKET=$OPTARG;;
        f) S3_FILE_SOURCE=$OPTARG;;
        F) S3_FILE_TARGET=$OPTARG;;
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
log "- S3 File (Source):   [${S3_FILE_SOURCE}]"
log "- S3 File (Target):   [${S3_FILE_TARGET}]"
log "- Encryption:         [${S3_SERVER_SIDE_ENCRYPTION}]"
log "- Reduced Redundancy: [${S3_REDUCED_REDUNDANCY}]"
## END Initialize
###------------------------------------------------------------------------------------------------

###------------------------------------------------------------------------------------------------
## START Execution
line_break
log_highlight "Execution"

s3_move_in_bucket "${S3_BUCKET}" "${S3_FILE_SOURCE}" "${S3_FILE_TARGET}" "${S3_SERVER_SIDE_ENCRYPTION}" "${S3_REDUCED_REDUNDANCY}"
RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then exit_logic $E_BAD_ARGS; fi
## END Execution
###------------------------------------------------------------------------------------------------

log_success "${SELF_IDENTITY_H}: Finished"
exit_logic 0
