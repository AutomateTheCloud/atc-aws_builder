#!/bin/bash
###------------------------------------------------------------------------------------------------
# alias:   aws_builder.codedeploy.deploy_revision
# script:  deploy_revision.sh
# purpose: CodeDeploy (Deploy Revision)
# version: 1.0.0
###------------------------------------------------------------------------------------------------

###------------------------------------------------------------------------------------------------
## Config
declare -r SELF_IDENTITY="codedeploy_deploy_revision"
declare -r SELF_IDENTITY_H="CodeDeploy (Deploy Revision)"
declare -a ARGUMENTS_DESCRIPTION=(
    '-c <codedeploy_application> : CodeDeploy Application Name'
    '-g <codedeploy_deployment_group> : CodeDeploy Deployment Group'
    '-d <codedeploy_deployment_config> : CodeDeploy Deployment Config'
    '-t <codedeploy_timeout> : CodeDeploy Timeout (in minutes, optional, defaults to 30 minutes)'
    '-f <filename> : Revision Filename'
    '-b <bucket_name> : S3 Bucket Name'
    '-D <deployment_desc> : Deployment Description (optional)'
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
    'codedeploy_deployment_group'
    'codedeploy_deployment_config'
    'codedeploy_timeout'
    'filename_revision'
    's3_bucket'
    'aws_region'
    's3_target'
)

DEPLOYMENT_DESCRIPTION=""
DEPLOYMENT_STATUS=""
CODEDEPLOY_MAX_ATTEMPTS=""
DEPLOYMENT_ID=""

TRACKING_PROGRESS=1
COUNTER=0
RETRY_ENABLED=no

###------------------------------------------------------------------------------------------------
## Main
# Process Arguments
while getopts "hVc:g:d:t:f:b:D:r:P:" OPTION; do
    case $OPTION in
        c) CODEDEPLOY_APPLICATION_NAME=$OPTARG;;
        g) CODEDEPLOY_DEPLOYMENT_GROUP=$OPTARG;;
        d) CODEDEPLOY_DEPLOYMENT_CONFIG=$OPTARG;;
        t) CODEDEPLOY_TIMEOUT=$OPTARG;;
        f) FILENAME_REVISION=$OPTARG;;
        b) S3_BUCKET=$OPTARG;;
        D) DEPLOYMENT_DESCRIPTION=$OPTARG;;
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

S3_TARGET="codedeploy/${CODEDEPLOY_APPLICATION_NAME}/${FILENAME_REVISION}"

if(is_empty "${CODEDEPLOY_TIMEOUT}"); then
    CODEDEPLOY_TIMEOUT="${AWS_CODEDEPLOY_DEFAULT_OPERATION_TIMEOUT}"
fi
CODEDEPLOY_MAX_ATTEMPTS=$((CODEDEPLOY_TIMEOUT * 60 / AWS_CODEDEPLOY_DEFAULT_VERIFICATION_SLEEP))

verify_array_key_values "VARIABLES_REQUIRED[@]"
RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then exit_logic $E_BAD_ARGS; fi

line_break
log "- CodeDeploy Application:       [${CODEDEPLOY_APPLICATION_NAME}]"
log "- CodeDeploy Deployment Group:  [${CODEDEPLOY_DEPLOYMENT_GROUP}]"
log "- CodeDeploy Deployment Config: [${CODEDEPLOY_DEPLOYMENT_CONFIG}]"
log "- CodeDeploy Timeout:           [${CODEDEPLOY_TIMEOUT}]"
log "- Revision File:                [${FILENAME_REVISION}]"
log "- S3 Bucket:                    [${S3_BUCKET}]"
log "- S3 Target:                    [${S3_TARGET}]"
log "- Deployment Description:       [${DEPLOYMENT_DESCRIPTION}]"
log "- AWS Region:                   [${AWS_REGION}]"
## END Initialize
###------------------------------------------------------------------------------------------------

###------------------------------------------------------------------------------------------------
## START Execution
line_break
log_highlight "Execution"

codedeploy_create_deployment DEPLOYMENT_ID "${CODEDEPLOY_APPLICATION_NAME}" "${CODEDEPLOY_DEPLOYMENT_GROUP}" "${CODEDEPLOY_DEPLOYMENT_CONFIG}" "${S3_BUCKET}" "${S3_TARGET}" "${DEPLOYMENT_DESCRIPTION}" "${AWS_REGION}"
RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then exit_logic $RETURNVAL; fi

log "- Deployment ID: [${DEPLOYMENT_ID}]"
call_sleep 10 "waiting for deployment to startup"

while [ ${TRACKING_PROGRESS} == 1 ]; do
    COUNTER=$((${COUNTER}+1))
    if [ ${COUNTER} -gt ${CODEDEPLOY_MAX_ATTEMPTS} ]; then
        log_error "${SELF_IDENTITY_H}: retry count [${CODEDEPLOY_MAX_ATTEMPTS}] exceeded, aborting tracking operation"
        exit_logic $E_CODEDEPLOY_DEPLOYMENT_FAILURE
    fi
    if option_enabled RETRY_ENABLED; then
        call_sleep ${AWS_CODEDEPLOY_DEFAULT_VERIFICATION_SLEEP}
    fi
    log_notice "Attempt: [$(printf "%02d\n" "${COUNTER}")/${CODEDEPLOY_MAX_ATTEMPTS}] | Tracking Deployment [${DEPLOYMENT_ID}]"
    DEPLOYMENT_STATUS=""
    codedeploy_get_deployment DEPLOYMENT_STATUS "${DEPLOYMENT_ID}" "${AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        exit_logic $RETURNVAL
    fi
    case $DEPLOYMENT_STATUS in
        "Succeeded")  TRACKING_PROGRESS=0;;
        "Failed")     TRACKING_PROGRESS=0; log_error "${SELF_IDENTITY_H}: Deployment failed"; exit_logic $E_CODEDEPLOY_DEPLOYMENT_FAILURE;;
    esac
    RETRY_ENABLED=yes
done
## END Execution
###------------------------------------------------------------------------------------------------

log_success "${SELF_IDENTITY_H}: Finished"
exit_logic 0
