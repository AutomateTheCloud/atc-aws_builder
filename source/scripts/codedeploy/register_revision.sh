#!/bin/bash
###------------------------------------------------------------------------------------------------
# alias:   aws_builder.codedeploy.register_revision
# script:  register_revision.sh
# purpose: CodeDeploy (Register Revision)
# version: 1.0.0
###------------------------------------------------------------------------------------------------

###------------------------------------------------------------------------------------------------
## Config
declare -r SELF_IDENTITY="codedeploy_register_revision"
declare -r SELF_IDENTITY_H="CodeDeploy (Register Revision)"
declare -a ARGUMENTS_DESCRIPTION=(
    '-c <codedeploy_application> : CodeDeploy Application Name'
    '-f <file> : Revision File'
    '-b <bucket_name> : S3 Bucket Name'
    '-r <region> : AWS Region'
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
    "${LIB_FUNCTIONS_AWS_CODEDEPLOY}"
)

###------------------------------------------------------------------------------------------------
## Variables
VARIABLES_REQUIRED=(
    'codedeploy_application_name'
    'file_revision'
    's3_bucket'
    'aws_region'
    's3_target'
)

###------------------------------------------------------------------------------------------------
## Main
# Process Arguments
while getopts "hVc:f:b:r:P:" OPTION; do
    case $OPTION in
        c) CODEDEPLOY_APPLICATION_NAME=$OPTARG;;
        f) FILE_REVISION=$OPTARG;;
        b) S3_BUCKET=$OPTARG;;
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

S3_TARGET="codedeploy/${CODEDEPLOY_APPLICATION_NAME}/$($(which basename) "${FILE_REVISION}")"

verify_array_key_values "VARIABLES_REQUIRED[@]"
RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then exit_logic $E_BAD_ARGS; fi

line_break
log "- CodeDeploy Application: [${CODEDEPLOY_APPLICATION_NAME}]"
log "- Revision File:          [${FILE_REVISION}]"
log "- S3 Bucket:              [${S3_BUCKET}]"
log "- S3 Target:              [${S3_TARGET}]"
log "- AWS Region:             [${AWS_REGION}]"
## END Initialize
###------------------------------------------------------------------------------------------------

###------------------------------------------------------------------------------------------------
## START Execution
line_break
log_highlight "Execution"

log_notice "${SELF_IDENTITY_H}: Uploading CodeDeploy Revision to S3"
s3_cp_upload "${S3_BUCKET}" "${S3_TARGET}" "${FILE_REVISION}" yes yes
RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then exit_logic $RETURNVAL; fi

log_notice "${SELF_IDENTITY_H}: Registering CodeDeploy Revision"
codedeploy_register_revision "${CODEDEPLOY_APPLICATION_NAME}" "${S3_BUCKET}" "${S3_TARGET}" "${AWS_REGION}"
RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then exit_logic $RETURNVAL; fi
## END Execution
###------------------------------------------------------------------------------------------------

log_success "${SELF_IDENTITY_H}: Finished"
exit_logic 0
