#!/bin/bash
###------------------------------------------------------------------------------------------------
# alias:   aws_builder.elasticache.redis.create
# script:  create.sh
# purpose: Creates Redis Elasticache (supports snapshots)
# version: 1.0.1
###------------------------------------------------------------------------------------------------

###------------------------------------------------------------------------------------------------
## Config
declare -r SELF_IDENTITY="redis_create"
declare -r SELF_IDENTITY_H="ElastiCache (Redis - Create)"
declare -a ARGUMENTS_DESCRIPTION=(
    '-s <stack_name> : Cloudformation Stack Name'
    '-t <timeout> : Timeout (in minutes)'
    '-S <snapshot_arn> : Snapshot Name to create new Redis (optional)'
    '-c <cluster_name> : Cluster Name to use snapshots from to create new Redis (optional)'
    '-v : Verify Connection after Creation'
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
   "${LIB_FUNCTIONS_AWS_CLOUDFORMATION}"
   "${LIB_FUNCTIONS_AWS_SSM}"
   "${LIB_FUNCTIONS_AWS_ROUTE53}"
   "${LIB_FUNCTIONS_AWS_ELASTICACHE_REDIS}"
)

###------------------------------------------------------------------------------------------------
## Variables
VARIABLES_REQUIRED=(
    'cloudformation_stack_name'
    'aws_region'
)

CLOUDFORMATION_STACK_NAME="" # -s
AWS_OPERATION_TIMEOUT="" # -t
SNAPSHOT_NAME="" # -s
CLUSTER_NAME_FOR_SNAPSHOT="" # -c
VERIFY_CONNECTION=no # -v
AWS_REGION="" # -r

TAKE_SNAPSHOT=no
ELASTICACHE_PREVIOUS_OCCURRENCE=""

###------------------------------------------------------------------------------------------------
## Main
# Process Arguments
while getopts "hVs:t:S:c:v:r:P:" OPTION; do
    case $OPTION in
        s) CLOUDFORMATION_STACK_NAME=$OPTARG;;
        t) AWS_OPERATION_TIMEOUT=$OPTARG;;
        S) SNAPSHOT_NAME=$OPTARG;;
        c) CLUSTER_NAME_FOR_SNAPSHOT=$OPTARG;;
        v) VERIFY_CONNECTION=yes;;
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

verify_array_key_values "VARIABLES_REQUIRED[@]"
RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then exit_logic $E_BAD_ARGS; fi

DIRECTORY_SQL_EXECUTION="$(echo "${DIRECTORY_SQL_EXECUTION}" | sed 's:/*$::')"
if(! is_empty "${DIRECTORY_SQL_EXECUTION}"); then
    if [ ! -d "${DIRECTORY_SQL_EXECUTION}" ]; then
        TMP_ERROR_MSG="SQL Execution directory does not exist [${DIRECTORY_SQL_EXECUTION}]"
        log_error "- ${TMP_ERROR_MSG}"
        exit_logic $E_OBJECT_NOT_FOUND "${TMP_ERROR_MSG}"
    fi
fi

if(is_empty "${AWS_OPERATION_TIMEOUT}"); then
    AWS_OPERATION_TIMEOUT="${AWS_EC_DEFAULT_VERIFICATION_TIMEOUT}"
fi

line_break
log "- CloudFormation Stack:       [${CLOUDFORMATION_STACK_NAME}]"
log "- Operation Timeout:          [${AWS_OPERATION_TIMEOUT} minutes]"
log "- AWS Region:                 [${AWS_REGION}]"

if(! is_empty "${SNAPSHOT_NAME}"); then
    log "- Snapshot Name:              [${SNAPSHOT_NAME}]"
fi
if(! is_empty "${CLUSTER_NAME_FOR_SNAPSHOT}"); then
    log "- Redis to Pull Snapshots:    [${CLUSTER_NAME_FOR_SNAPSHOT}]"
fi
if(! is_empty "${CLUSTER_NAME_FOR_SNAPSHOT}"); then
    line_break
    log_notice "Retrieving Latest Snapshot [source: ${CLUSTER_NAME_FOR_SNAPSHOT}]"
    redis_get_latest_snapshot_name SNAPSHOT_NAME "${CLUSTER_NAME_FOR_SNAPSHOT}" "${AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        TMP_ERROR_MSG="Failed to retrieve snapshot, aborting operation"
        log_error "- ${TMP_ERROR_MSG}"
        exit_logic $RETURNVAL "${TMP_ERROR_MSG}"
    fi
    if(is_empty "${SNAPSHOT_NAME}"); then
        TMP_ERROR_MSG="No snapshot found. As we were expecting one, aborting operation"
        log_error "- ${TMP_ERROR_MSG}"
        exit_logic $E_OBJECT_NOT_FOUND "${TMP_ERROR_MSG}"
    fi
    log "- Snapshot ARN:               [${SNAPSHOT_NAME}]"
fi
line_break

load_info_redis "${CLOUDFORMATION_STACK_NAME}" "${AWS_REGION}"
RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then exit_logic ${RETURNVAL}; fi

line_break
log_notice "Checking to see if another Elasticache Redis occurrence exists"
redis_get_current_replication_group_id ELASTICACHE_PREVIOUS_OCCURRENCE "${ELASTICACHE_REFERENCENAME}" "${AWS_REGION}"
RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then exit_logic ${RETURNVAL}; fi

if(! is_empty "${ELASTICACHE_PREVIOUS_OCCURRENCE}"); then
    log_notice "- Previous Elasticache Redis Occurrence detected [${ELASTICACHE_PREVIOUS_OCCURRENCE}], we will attempt to replace after new Elasticache Redis is created"
fi
## END Initialize
###------------------------------------------------------------------------------------------------

###------------------------------------------------------------------------------------------------
## START Execution
line_break
log_highlight "Execution"

log_notice "Creating Redis"
redis_create "${SNAPSHOT_NAME}" "${AWS_REGION}"
RETURNVAL="$?"
if [ ${RETURNVAL} -ne 0 ]; then
    TMP_ERROR_MSG="Failed to create Redis, cannot continue"
    log_error "- ${TMP_ERROR_MSG}"
    log_warning "Performing Cleanup"
    redis_delete "${ELASTICACHE_REFERENCEID}" "${AWS_REGION}"
    exit_logic $RETURNVAL "${TMP_ERROR_MSG}"
fi
line_break

if option_enabled VERIFY_CONNECTION; then
    log_notice "Checking to see if Elasticache Redis Endpoint is Healthy"
    redis_get_info "${ELASTICACHE_ENDPOINT_WRITE}" "${ELASTICACHE_PORT}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        TMP_ERROR_MSG="Failed to retrieve Redis Information. To be safe, we will assume something went wrong and abort the operation"
        log_error "- ${TMP_ERROR_MSG}"
        log_warning "Performing Cleanup"
        redis_delete "${ELASTICACHE_REFERENCEID}" "${AWS_REGION}"
        exit_logic $RETURNVAL "${TMP_ERROR_MSG}"
    fi
    line_break
fi

log_notice "Setting DNS Record"
redis_set_dns
RETURNVAL="$?"
if [ ${RETURNVAL} -ne 0 ]; then
    TMP_ERROR_MSG="Failed to set DNS Record, cannot continue"
    log_error "- ${TMP_ERROR_MSG}"
    log_warning "Performing Cleanup"
    redis_delete "${ELASTICACHE_REFERENCEID}" "${AWS_REGION}"
    exit_logic $RETURNVAL "${TMP_ERROR_MSG}"
fi
line_break

if(! is_empty "${ELASTICACHE_PREVIOUS_OCCURRENCE}"); then
    log_notice "NOTICE: Previous Elasticache Redis Occurrence detected [${ELASTICACHE_PREVIOUS_OCCURRENCE}], performing cleanup"
    call_sleep 60 "giving application time to switch over to the new DNS endpoint"
    redis_delete "${ELASTICACHE_PREVIOUS_OCCURRENCE}" "${AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        TMP_ERROR_MSG="Failed to delete previous Redis Occurrence, manual intervention required"
        log_error "- ${TMP_ERROR_MSG}"
        exit_logic $RETURNVAL "${TMP_ERROR_MSG}"
    fi
fi
## END Execution
###------------------------------------------------------------------------------------------------

log_success "${SELF_IDENTITY_H}: Finished"
exit_logic 0
