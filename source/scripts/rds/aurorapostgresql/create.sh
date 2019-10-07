#!/bin/bash
###------------------------------------------------------------------------------------------------
# alias:   aws_builder.rds.aurorapostgresql.create
# script:  create.sh
# purpose: Creates Aurora PostgreSQL Database (supports snapshots and SQL execution)
# version: 1.0.0
###------------------------------------------------------------------------------------------------

###------------------------------------------------------------------------------------------------
## Config
declare -r SELF_IDENTITY="aurorapsql_create"
declare -r SELF_IDENTITY_H="RDS (Aurora PostgreSQL - Create)"
declare -a ARGUMENTS_DESCRIPTION=(
    '-s <stack_name> : Cloudformation Stack Name'
    '-t <timeout> : Timeout (in minutes)'
    '-S <snapshot_arn> : Snapshot ARN to create new Database (optional)'
    '-c <cluster_id> : Cluster ID to use snapshots from to create new Database (optional)'
    '-e <sql_execution_directory> : Directory to execute SQL scripts from (optional)'
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
   "${LIB_FUNCTIONS_AWS_RDS}"
   "${LIB_FUNCTIONS_AWS_RDS_AURORA_POSTGRESQL}"
)

###------------------------------------------------------------------------------------------------
## Variables
VARIABLES_REQUIRED=(
    'cloudformation_stack_name'
    'aws_region'
)

CLOUDFORMATION_STACK_NAME="" # -s
AWS_OPERATION_TIMEOUT="" # -t
SNAPSHOT_ARN="" # -s
CLUSTER_ID_FOR_SNAPSHOT="" # -c
DIRECTORY_SQL_EXECUTION="" # -e
AWS_REGION="" # -r

TMP_FILE_SQL=""
TAKE_SNAPSHOT=no

###------------------------------------------------------------------------------------------------
## Main
# Process Arguments
while getopts "hVs:t:S:c:e:r:P:" OPTION; do
    case $OPTION in
        s) CLOUDFORMATION_STACK_NAME=$OPTARG;;
        t) AWS_OPERATION_TIMEOUT=$OPTARG;;
        S) SNAPSHOT_ARN=$OPTARG;;
        c) CLUSTER_ID_FOR_SNAPSHOT=$OPTARG;;
        e) DIRECTORY_SQL_EXECUTION=$OPTARG;;
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
    AWS_OPERATION_TIMEOUT="${AWS_RDS_DEFAULT_VERIFICATION_TIMEOUT}"
fi

line_break
log "- CloudFormation Stack:       [${CLOUDFORMATION_STACK_NAME}]"
log "- Operation Timeout:          [${AWS_OPERATION_TIMEOUT} minutes]"
log "- AWS Region:                 [${AWS_REGION}]"

if(! is_empty "${SNAPSHOT_ARN}"); then
    log "- Snapshot ARN:               [${SNAPSHOT_ARN}]"
fi
if(! is_empty "${CLUSTER_ID_FOR_SNAPSHOT}"); then
    log "- Cluster to Pull Snapshots:  [${CLUSTER_ID_FOR_SNAPSHOT}]"
fi
if(! is_empty "${CLUSTER_ID_FOR_SNAPSHOT}"); then
    line_break
    log_notice "Retrieving Latest Snapshot [source: ${CLUSTER_ID_FOR_SNAPSHOT}]"
    rds_get_latest_cluster_snapshot_arn SNAPSHOT_ARN "${CLUSTER_ID_FOR_SNAPSHOT}" "${AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        TMP_ERROR_MSG="Failed to retrieve snapshot, aborting operation"
        log_error "- ${TMP_ERROR_MSG}"
        exit_logic $RETURNVAL "${TMP_ERROR_MSG}"
    fi
    if(is_empty "${SNAPSHOT_ARN}"); then
        TMP_ERROR_MSG="No snapshot found. As we were expecting one, aborting operation"
        log_error "- ${TMP_ERROR_MSG}"
        exit_logic $E_OBJECT_NOT_FOUND "${TMP_ERROR_MSG}"
    fi
    log "- Snapshot ARN:               [${SNAPSHOT_ARN}]"
fi
if(! is_empty "${DIRECTORY_SQL_EXECUTION}"); then
    log "- SQL Execution Directory:    [${DIRECTORY_SQL_EXECUTION}]"
fi
line_break

load_info_aurora_postgresql "${CLOUDFORMATION_STACK_NAME}" "${AWS_REGION}"
RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then exit_logic ${RETURNVAL}; fi

line_break
log_notice "Checking to see if another Aurora PostgreSQL Database occurrence exists"
rds_get_current_cluster_id CLUSTER_PREVIOUS_OCCURRENCE "${DATABASE_REFERENCENAME}" "${AWS_REGION}"
RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then exit_logic ${RETURNVAL}; fi

if(! is_empty "${CLUSTER_PREVIOUS_OCCURRENCE}"); then
    log_notice "- Previous Aurora PostgreSQL Cluster Occurrence detected [${CLUSTER_PREVIOUS_OCCURRENCE}], we will attempt to replace after new PostgreSQL Cluster is created"
fi
## END Initialize
###------------------------------------------------------------------------------------------------

###------------------------------------------------------------------------------------------------
## START Execution
line_break
log_highlight "Execution"

if(! is_empty "${SNAPSHOT_ARN}"); then
    log_notice "Creating Aurora PostgreSQL Cluster from Snapshot"
    aurora_postgresql_create_cluster_from_snapshot "${SNAPSHOT_ARN}" "${AWS_OPERATION_TIMEOUT}" "${AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        TMP_ERROR_MSG="Failed to create PostgreSQL Database from Snapshot, cannot continue"
        log_error "- ${TMP_ERROR_MSG}"
        log_warning "Performing Cleanup"
        rds_delete "${POSTGRESQL_POSTGRESQLDBID}" no "${AWS_REGION}"
        exit_logic $RETURNVAL "${TMP_ERROR_MSG}"
    fi
else
    log_notice "Creating Aurora PostgreSQL Cluster"
    aurora_postgresql_create_cluster "${AWS_OPERATION_TIMEOUT}" "${AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        TMP_ERROR_MSG="Failed to create Aurora PostgreSQL Cluster, cannot continue"
        log_error "- ${TMP_ERROR_MSG}"
        log_warning "Performing Cleanup"
        rds_delete_cluster "${DATABASE_CLUSTERID}" "no" "${AWS_REGION}"
        exit_logic $RETURNVAL "${TMP_ERROR_MSG}"
    fi
fi

line_break
log_notice "Creating Aurora PostgreSQL Databases [count: ${#ARRAY_DATABASES[@]}]"
for DATABASE_ID in "${ARRAY_DATABASES[@]}"; do
    aurora_postgresql_create_database "${DATABASE_ID}" "${AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        TMP_ERROR_MSG="Failed to create Aurora PostgreSQL Database, cannot continue"
        log_error "- ${TMP_ERROR_MSG}"
        log_warning "Performing Cleanup"
        rds_delete_cluster "${DATABASE_CLUSTERID}" "no" "${AWS_REGION}"
        exit_logic $RETURNVAL "${TMP_ERROR_MSG}"
    fi
done
# CHANGE TO 60
call_sleep 15 "allowing database cluster membership time to warmup"

line_break
log_notice "Waiting for Aurora Cluster Databases to become available [count: ${#ARRAY_DATABASES[@]}]"
for DATABASE_ID in "${ARRAY_DATABASES[@]}"; do
    log "- database: [${DATABASE_ID}]"
    rds_poll_status "${DATABASE_ID}" yes "${AWS_OPERATION_TIMEOUT}" "" "${AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        TMP_ERROR_MSG="Database failed to become available, cannot continue"
        log_error "- ${TMP_ERROR_MSG}"
        log_warning "Performing Cleanup"
        rds_delete_cluster "${DATABASE_CLUSTERID}" "no" "${AWS_REGION}"
        exit_logic $RETURNVAL "${TMP_ERROR_MSG}"
    fi
done

line_break
log_notice "Verifying Database"
aurora_postgresql_verify_connection "${DATABASE_ENDPOINT_WRITE}" "${DATABASE_MASTERUSERNAME}" "${DATABASE_MASTERPASSWORD}" "${DATABASE_DATABASENAME}" "${DATABASE_PORT}"
RETURNVAL="$?"
if [ ${RETURNVAL} -ne 0 ]; then
    TMP_ERROR_MSG="Failed to verify database"
    log_error "- ${TMP_ERROR_MSG}"
    log_warning "Performing Cleanup"
    rds_delete_cluster "${DATABASE_CLUSTERID}" "no" "${AWS_REGION}"
    exit_logic $RETURNVAL "${TMP_ERROR_MSG}"
fi

if(! is_empty "${DIRECTORY_SQL_EXECUTION}"); then
    line_break
    log_notice "Executing SQL files"
    for TMP_FILE_SQL in $(find "${DIRECTORY_SQL_EXECUTION}" -type f -name "*.sql" | grep ".sql\$" | sort); do
        log "- SQL file: [${TMP_FILE_SQL}]"
        aurora_postgresql_execute_sql "${DATABASE_ENDPOINT_WRITE}" "${DATABASE_MASTERUSERNAME}" "${DATABASE_MASTERPASSWORD}" "${DATABASE_DATABASENAME}"  "${DATABASE_PORT}" "${TMP_FILE_SQL}"
        RETURNVAL="$?"
        if [ ${RETURNVAL} -ne 0 ]; then
            TMP_ERROR_MSG="Failed to execute SQL file, cannot continue"
            log_error "- ${TMP_ERROR_MSG}"
            log_warning "Performing Cleanup"
            rds_delete_cluster "${DATABASE_CLUSTERID}" "no" "${AWS_REGION}"
            exit_logic $RETURNVAL "${TMP_ERROR_MSG}"
        fi
    done
fi

log_notice "Setting DNS Records"
aurora_postgresql_set_dns "${AWS_REGION}"
RETURNVAL="$?"
if [ ${RETURNVAL} -ne 0 ]; then
    TMP_ERROR_MSG="Failed to set DNS Record, cannot continue"
    log_error "- ${TMP_ERROR_MSG}"
    log_warning "Performing Cleanup"
    rds_delete_cluster "${DATABASE_CLUSTERID}" "no" "${AWS_REGION}"
    exit_logic $RETURNVAL "${TMP_ERROR_MSG}"
fi

if(! is_empty "${CLUSTER_PREVIOUS_OCCURRENCE}"); then
    line_break
    log_notice "- Previous Aurora PostgreSQL Cluster Occurrence detected [${CLUSTER_PREVIOUS_OCCURRENCE}], performing cleanup"
    call_sleep 5 "giving application time to switch over to the new DNS endpoint"
    if [[ "ZZ_$(to_upper "${DATABASE_ENVIRONMENT}")" == "ZZ_PRD" ]]; then
        TAKE_SNAPSHOT=yes
    fi
    rds_delete_cluster "${CLUSTER_PREVIOUS_OCCURRENCE}" "${TAKE_SNAPSHOT}" "${AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        TMP_ERROR_MSG="Failed to delete previous Aurora PostgreSQL Cluster Occurrence, manual intervention required"
        log_error "- ${TMP_ERROR_MSG}"
        exit_logic $RETURNVAL "${TMP_ERROR_MSG}"
    fi
fi
## END Execution
###------------------------------------------------------------------------------------------------

log_success "${SELF_IDENTITY_H}: Finished"
exit_logic 0
