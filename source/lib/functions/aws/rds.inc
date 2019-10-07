###------------------------------------------------------------------------------------------------
# script:  rds.inc
# purpose: Collection of functions related to AWS (RDS)
# version: 1.0.0
#
# function list:
#          - rds_delete_cluster
#          - rds_delete
#          - rds_delete_call
#          - rds_delete_cluster_member
#          - rds_get_cluster_endpoint_read
#          - rds_get_cluster_endpoint_write
#          - rds_get_cluster_membership
#          - rds_get_cluster_status
#          - rds_get_current_cluster_id
#          - rds_get_current_database_id
#          - rds_get_endpoint
#          - rds_get_engine_version
#          - rds_get_latest_cluster_snapshot_arn
#          - rds_get_latest_snapshot_arn
#          - rds_get_database_replica_ids
#          - rds_get_parameter_group
#          - rds_get_status
#          - rds_is_db_pending_reboot
#          - rds_poll_cluster_status
#          - rds_poll_status
#          - rds_reboot
#          - rds_set_backup_policy
#          - rds_set_engine_version
#          - rds_set_maintenance_policy
#          - rds_set_parameter_group
#          - rds_set_security_group
#          - rds_update_password
###------------------------------------------------------------------------------------------------
REQUIRED_EXECUTABLES+=('aws')

###------------------------------------------------------------------------------------------------
# Variables
AWS_RDS_DEFAULT_RETRY_COUNT=3
AWS_RDS_DEFAULT_RETRY_TIMER_MAX_SEC=30
AWS_RDS_DEFAULT_OPERATION_TIMEOUT=30
AWS_RDS_DEFAULT_VERIFICATION_TIMEOUT=30
AWS_RDS_DEFAULT_VERIFICATION_SLEEP=30
AWS_RDS_DNS_TTL=30

PARAMETER_VARIABLES+=(
    'Environment'
    'ProjectAbbr'
    'FunctionAbbr'
)

CREDENTIALS_VARIABLES+=(
    'MasterUsername'
    'MasterPassword'
)

###------------------------------------------------------------------------------------------------
## FUNCTION: rds_delete_cluster()
## - Delete RDS Cluster
## - Arguments
##   - $1: Cluster ID
##   - $2: Take Snapshot (yes/no, defaults to no)
##   - $3: Region
function rds_delete_cluster() {
    local FUNCTION_DESCRIPTION="RDS - Cluster (Delete)"
    local TMP_CLUSTER_ID="${1}"
    local TMP_TAKE_SNAPSHOT="${2}"
    local TMP_AWS_REGION="${3}"

    local RETURNVAL=""
    local TMP_STRING=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    local TMP_ARRAY_CLUSTER_DATABASES=()

    if(is_empty "${TMP_CLUSTER_ID}"); then
        log_error "${FUNCTION_DESCRIPTION}: Cluster ID not specified"
        return $E_BAD_ARGS
    fi

    if(is_empty "${TMP_TAKE_SNAPSHOT}"); then
        TMP_TAKE_SNAPSHOT=no
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    if option_enabled TMP_TAKE_SNAPSHOT; then
        local ARG_SNAPSHOT="--no-skip-final-snapshot --final-db-snapshot-identifier ${TMP_CLUSTER_ID}-final-$($(which date) +%Y%m%d-%H%M%S)"
    else
        local ARG_SNAPSHOT="--skip-final-snapshot"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    log_notice "${FUNCTION_DESCRIPTION}: started"
    log "- Cluster ID:  [${TMP_CLUSTER_ID}]"
    log "- Region:      [${TMP_AWS_REGION}]"

    rds_get_cluster_membership TMP_ARRAY_CLUSTER_DATABASES "${TMP_CLUSTER_ID}" "${TMP_AWS_REGION}"
    RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then return ${RETURNVAL}; fi

    for TMP_CLUSTER_DATABASE in "${TMP_ARRAY_CLUSTER_DATABASES[@]}"; do
        log "${FUNCTION_DESCRIPTION}: deleting cluster member [${TMP_CLUSTER_DATABASE}]"
        rds_delete_cluster_member "${TMP_CLUSTER_DATABASE}" "${TMP_AWS_REGION}"
        RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then return ${RETURNVAL}; fi
        call_sleep 5 "sleeping to allow cluster time to accept reconfiguration"
    done

    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_RDS_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_RDS_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_RDS_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no
        log "${FUNCTION_DESCRIPTION}: deleting cluster (Attempt::${COUNTER} of ${AWS_RDS_DEFAULT_RETRY_COUNT})"
        $(which aws) ${AWS_REGION_STRING} rds delete-db-cluster --db-cluster-identifier "${TMP_CLUSTER_ID}" ${ARG_SNAPSHOT} >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
        RETURNVAL="$?"
        # Fix Response File
        $(which sed) -i "s/\o15/_AWS_BAD_IGNORE\\n/g" "${AWS_FILE_RESPONSE}"
        $(which sed) -i '/_AWS_BAD_IGNORE/d' "${AWS_FILE_RESPONSE}"
        if [ ${RETURNVAL} -ne 0 ]; then
            log_add_from_file "${AWS_FILE_ERROR}" "${FUNCTION_DESCRIPTION}: Data containing error"
            log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Data"
            log_error "${FUNCTION_DESCRIPTION}: operation failed (aws_cli_exit_code::${RETURNVAL}])"
            RETRY_ENABLED=yes
            # Reset Files
            > ${AWS_FILE_RESPONSE}
            > ${AWS_FILE_ERROR}
        else
            log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Data"
            log_success "${FUNCTION_DESCRIPTION}: Successfully deleted Cluster"
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: rds_delete()
## - Delete RDS Database
## - Arguments
##   - $1: Database ID
##   - $2: Take Snapshot (yes/no, defaults to no)
##   - $3: Region
function rds_delete() {
    local FUNCTION_DESCRIPTION="RDS (Delete)"
    local TMP_DATABASE_ID="${1}"
    local TMP_TAKE_SNAPSHOT="${2}"
    local TMP_AWS_REGION="${3}"

    local RETURNVAL=""
    local TMP_KEY=""
    local TMP_ARRAY_REPLICA_IDS=()
    local HAS_REPLICAS=no

    if(is_empty "${TMP_DATABASE_ID}"); then
        log_error "${FUNCTION_DESCRIPTION}: Database ID not specified"
        return $E_BAD_ARGS
    fi

    log "${FUNCTION_DESCRIPTION}: started (Database ID: [${TMP_DATABASE_ID}])"
    log "- Take Final Snapshot: [${TMP_TAKE_SNAPSHOT}]"
    log "${FUNCTION_DESCRIPTION}: Checking for existence of replica databases"
    rds_get_database_replica_ids TMP_ARRAY_REPLICA_IDS "${TMP_DATABASE_ID}" "${TMP_AWS_REGION}"
    REPLICA_RECORDS=""
    for TMP_KEY in "${TMP_ARRAY_REPLICA_IDS[@]}"; do
        if(! is_empty "${TMP_KEY}"); then
            HAS_REPLICAS=yes
            log "- Deleting Replica: [${TMP_KEY}]"
            rds_delete_call "${TMP_KEY}" no "${TMP_AWS_REGION}"
            RETURNVAL="$?"
            if [ ${RETURNVAL} -ne 0 ]; then
                log_error "- Failed to delete Replica [${TMP_KEY}]"
                return $RETURNVAL
            fi
            call_sleep 2
        fi
    done
    if option_enabled HAS_REPLICAS; then
        call_sleep 20 "replica deletion detected, sleeping to allow master time to recalibrate"
        log "${FUNCTION_DESCRIPTION}: Waiting for database to enter a valid state to allow deletion"
        rds_poll_status "${TMP_DATABASE_ID}" yes "" "" "${TMP_AWS_REGION}"
        RETURNVAL="$?"
        if [ ${RETURNVAL} -ne 0 ]; then
            log_error "- PostgreSQL Database failed to become available, cannot continue"
            return $RETURNVAL
        else
            log_notice "- PostgreSQL Database is now available"
        fi
    fi
    log "${FUNCTION_DESCRIPTION}: Deleting Database: [${TMP_DATABASE_ID}]"
    rds_delete_call "${TMP_DATABASE_ID}" "${TMP_TAKE_SNAPSHOT}" "${TMP_AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "- Failed to delete Database [${TMP_DATABASE_ID}]"
        return $RETURNVAL
    fi

    return 0
}

###------------------------------------------------------------------------------------------------
## FUNCTION: rds_delete_call()
## - Delete RDS Database (the actual CLI call to perform delete)
## - Arguments
##   - $1: Database ID
##   - $2: Take Snapshot (yes/no, defaults to no)
##   - $3: Region
function rds_delete_call() {
    local FUNCTION_DESCRIPTION="RDS (Delete)"
    local TMP_DATABASE_ID="${1}"
    local TMP_TAKE_SNAPSHOT="${2}"
    local TMP_AWS_REGION="${3}"

    local RETURNVAL=""
    local TMP_STRING=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    if(is_empty "${TMP_DATABASE_ID}"); then
        log_error "${FUNCTION_DESCRIPTION}: Database ID not specified"
        return $E_BAD_ARGS
    fi

    if(is_empty "${TMP_TAKE_SNAPSHOT}"); then
        TMP_TAKE_SNAPSHOT=no
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    if option_enabled TMP_TAKE_SNAPSHOT; then
        local ARG_SNAPSHOT="--no-skip-final-snapshot --final-db-snapshot-identifier ${TMP_DATABASE_ID}-final-$($(which date) +%Y%m%d-%H%M%S)"
    else
        local ARG_SNAPSHOT="--skip-final-snapshot"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_RDS_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_RDS_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_RDS_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no
        log "${FUNCTION_DESCRIPTION}: Deleting Database (database_id: [${TMP_DATABASE_ID}] / region: [${TMP_AWS_REGION}]) (Attempt::${COUNTER} of ${AWS_RDS_DEFAULT_RETRY_COUNT})"
        $(which aws) ${AWS_REGION_STRING} rds delete-db-instance --db-instance-identifier "${TMP_DATABASE_ID}" ${ARG_SNAPSHOT} >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
        RETURNVAL="$?"
        # Fix Response File
        $(which sed) -i "s/\o15/_AWS_BAD_IGNORE\\n/g" "${AWS_FILE_RESPONSE}"
        $(which sed) -i '/_AWS_BAD_IGNORE/d' "${AWS_FILE_RESPONSE}"
        if [ ${RETURNVAL} -ne 0 ]; then
            log_add_from_file "${AWS_FILE_ERROR}" "${FUNCTION_DESCRIPTION}: Data containing error"
            log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Data"
            log_error "${FUNCTION_DESCRIPTION}: operation failed (aws_cli_exit_code::${RETURNVAL}])"
            RETRY_ENABLED=yes
            # Reset Files
            > ${AWS_FILE_RESPONSE}
            > ${AWS_FILE_ERROR}
        else
            log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Data"
            log_success "${FUNCTION_DESCRIPTION}: Successfully deleted Database"
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: rds_delete_cluster_member()
## - Delete Cluster Member database
## - Arguments
##   - $1: Database ID
##   - $2: Region
function rds_delete_cluster_member() {
    local FUNCTION_DESCRIPTION="RDS - Cluster (Delete Member)"
    local TMP_DATABASE_ID="${1}"
    local TMP_AWS_REGION="${2}"

    local RETURNVAL=""
    local TMP_STRING=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    local ARG_SNAPSHOT="--skip-final-snapshot"

    if(is_empty "${TMP_DATABASE_ID}"); then
        log_error "${FUNCTION_DESCRIPTION}: Database ID not specified"
        return $E_BAD_ARGS
    fi
    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    log_notice "${FUNCTION_DESCRIPTION}: started"
    log "- Database ID: [${TMP_DATABASE_ID}]"
    log "- Region:      [${TMP_AWS_REGION}]"
    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_RDS_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_RDS_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_RDS_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no
        log "${FUNCTION_DESCRIPTION}: deleting database (Attempt::${COUNTER} of ${AWS_RDS_DEFAULT_RETRY_COUNT})"
        $(which aws) ${AWS_REGION_STRING} rds delete-db-instance --db-instance-identifier "${TMP_DATABASE_ID}" ${ARG_SNAPSHOT} >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
        RETURNVAL="$?"
        # Fix Response File
        $(which sed) -i "s/\o15/_AWS_BAD_IGNORE\\n/g" "${AWS_FILE_RESPONSE}"
        $(which sed) -i '/_AWS_BAD_IGNORE/d' "${AWS_FILE_RESPONSE}"
        if [ ${RETURNVAL} -ne 0 ]; then
            log_add_from_file "${AWS_FILE_ERROR}" "${FUNCTION_DESCRIPTION}: Data containing error"
            log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Data"
            log_error "${FUNCTION_DESCRIPTION}: operation failed (aws_cli_exit_code::${RETURNVAL}])"
            RETRY_ENABLED=yes
            # Reset Files
            > ${AWS_FILE_RESPONSE}
            > ${AWS_FILE_ERROR}
        else
            log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Data"
            log "${FUNCTION_DESCRIPTION}: successfully deleted database"
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: rds_get_cluster_endpoint_read()
## - Retrieves Cluster Endpoint (Read) for specified Cluster
## - Arguments
##   - $1: Variable name to pass info to
##   - $2: Cluster ID
##   - $3: Region
function rds_get_cluster_endpoint_read() {
    local FUNCTION_DESCRIPTION="RDS - Cluster (Get Read Endpoint)"
    local __RESULTVAR=$1
    local TMP_CLUSTER_ID="${2}"
    local TMP_AWS_REGION="${3}"

    local RETURNVAL=""
    local TMP_STRING=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    if(is_empty "${TMP_CLUSTER_ID}"); then
        log_error "${FUNCTION_DESCRIPTION}: Cluster ID not specified"
        return $E_BAD_ARGS
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_RDS_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_RDS_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_RDS_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        $(which aws) ${AWS_REGION_STRING} rds describe-db-clusters --db-cluster-identifier "${TMP_CLUSTER_ID}" --no-paginate --query "DBClusters[].[ReaderEndpoint]" --output text >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
        RETURNVAL="$?"
        if [ ${RETURNVAL} -ne 0 ]; then
            log_add_from_file "${AWS_FILE_ERROR}" "${FUNCTION_DESCRIPTION}: Data containing error"
            log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Data"
            log_error "${FUNCTION_DESCRIPTION}: operation failed (aws_cli_exit_code::${RETURNVAL}])"
            RETRY_ENABLED=yes
            # Reset Files
            > ${AWS_FILE_RESPONSE}
            > ${AWS_FILE_ERROR}
        else
            TMP_STRING="$(cat ${AWS_FILE_RESPONSE} | tr '[:upper:]' '[:lower:]')"
            eval $__RESULTVAR="'$(echo "$(trim "${TMP_STRING}")" | sed -e 's/^ *//g' -e 's/ *$//g')'"
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: rds_get_cluster_endpoint_write()
## - Retrieves Cluster Endpoint (Write) for specified Cluster
## - Arguments
##   - $1: Variable name to pass info to
##   - $2: Cluster ID
##   - $3: Region
function rds_get_cluster_endpoint_write() {
    local FUNCTION_DESCRIPTION="RDS - Cluster (Get Write Endpoint)"
    local __RESULTVAR=$1
    local TMP_CLUSTER_ID="${2}"
    local TMP_AWS_REGION="${3}"

    local RETURNVAL=""
    local TMP_STRING=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    if(is_empty "${TMP_CLUSTER_ID}"); then
        log_error "${FUNCTION_DESCRIPTION}: Cluster ID not specified"
        return $E_BAD_ARGS
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_RDS_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_RDS_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_RDS_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        $(which aws) ${AWS_REGION_STRING} rds describe-db-clusters --db-cluster-identifier "${TMP_CLUSTER_ID}" --no-paginate --query "DBClusters[].[Endpoint]" --output text >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
        RETURNVAL="$?"
        if [ ${RETURNVAL} -ne 0 ]; then
            log_add_from_file "${AWS_FILE_ERROR}" "${FUNCTION_DESCRIPTION}: Data containing error"
            log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Data"
            log_error "${FUNCTION_DESCRIPTION}: operation failed (aws_cli_exit_code::${RETURNVAL}])"
            RETRY_ENABLED=yes
            # Reset Files
            > ${AWS_FILE_RESPONSE}
            > ${AWS_FILE_ERROR}
        else
            TMP_STRING="$(cat ${AWS_FILE_RESPONSE} | tr '[:upper:]' '[:lower:]')"
            eval $__RESULTVAR="'$(echo "$(trim "${TMP_STRING}")" | sed -e 's/^ *//g' -e 's/ *$//g')'"
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: rds_get_cluster_membership()
## - Retrieves current list of Databases in specified Cluster
## - Arguments
##   - $1: Array name to pass info to
##   - $2: Cluster ID
##   - $3: Region
function rds_get_cluster_membership() {
    local FUNCTION_DESCRIPTION="RDS (Get Cluster Membership)"
    local __RESULTVAR=$1
    local TMP_CLUSTER_ID="${2}"
    local TMP_AWS_REGION="${3}"

    local RETURNVAL=""
    local TMP_STRING=""
    local TMP_KEY=""
    local TMP_ARRAY_CLUSTER_MEMBERSHIP=()
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    if(is_empty "${TMP_CLUSTER_ID}"); then
        log_error "${FUNCTION_DESCRIPTION}: Cluster ID not specified"
        return $E_BAD_ARGS
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    log_notice "${FUNCTION_DESCRIPTION}: started"
    log "- Cluster ID: [${TMP_CLUSTER_ID}]"
    log "- Region:     [${TMP_AWS_REGION}]"

    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_RDS_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_RDS_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_RDS_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        log "${FUNCTION_DESCRIPTION}: polling (Attempt::${COUNTER} of ${AWS_RDS_DEFAULT_RETRY_COUNT})"
        $(which aws) ${AWS_REGION_STRING} rds describe-db-clusters --db-cluster-identifier "${TMP_CLUSTER_ID}" --no-paginate --query "DBClusters[].DBClusterMembers[].[DBInstanceIdentifier]" --output text 2>${AWS_FILE_ERROR} | sed -e 's/\t/ /' >${AWS_FILE_RESPONSE}
        RETURNVAL="$?"
        # Fix Response File
        $(which sed) -i "s/\o15/_AWS_BAD_IGNORE\\n/g" "${AWS_FILE_RESPONSE}"
        $(which sed) -i '/_AWS_BAD_IGNORE/d' "${AWS_FILE_RESPONSE}"
        if [ ${RETURNVAL} -ne 0 ]; then
            if grep -q "DBClusterNotFoundFault" "${AWS_FILE_ERROR}"; then
                eval $__RESULTVAR=""
                return 0
            fi
            log_add_from_file "${AWS_FILE_ERROR}" "${FUNCTION_DESCRIPTION}: Data containing error"
            log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Data"
            log_error "${FUNCTION_DESCRIPTION}: operation failed (aws_cli_exit_code::${RETURNVAL}])"
            RETRY_ENABLED=yes
            # Reset Files
            > ${AWS_FILE_RESPONSE}
            > ${AWS_FILE_ERROR}
        else
            readarray -t TMP_ARRAY_CLUSTER_MEMBERSHIP < ${AWS_FILE_RESPONSE}
            readarray -t TMP_ARRAY_CLUSTER_MEMBERSHIP < <(for a in "${TMP_ARRAY_CLUSTER_MEMBERSHIP[@]}"; do echo "${a}"; done | grep -v '^$' | sort | uniq)
            if [ ${#TMP_ARRAY_CLUSTER_MEMBERSHIP[@]} -eq 0 ]; then
                log "${FUNCTION_DESCRIPTION}: No Cluster Members found"
            else
                log "${FUNCTION_DESCRIPTION}: Membership Results [count: ${#TMP_ARRAY_CLUSTER_MEMBERSHIP[@]}]"
                for TMP_KEY in "${TMP_ARRAY_CLUSTER_MEMBERSHIP[@]}"; do
                    log "- [${TMP_KEY}]"
                done
                eval $__RESULTVAR="(${TMP_ARRAY_CLUSTER_MEMBERSHIP[@]})"
            fi
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: rds_get_cluster_status()
## - Retrieves status for specified Cluster
## - Arguments
##   - $1: Variable name to pass info to
##   - $2: Cluster ID
##   - $3: Region
function rds_get_cluster_status() {
    local FUNCTION_DESCRIPTION="RDS (Get Cluster Status)"
    local __RESULTVAR=$1
    local TMP_CLUSTER_ID="${2}"
    local TMP_AWS_REGION="${3}"

    local RETURNVAL=""
    local TMP_STRING=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    if(is_empty "${TMP_CLUSTER_ID}"); then
        log_error "${FUNCTION_DESCRIPTION}: Cluster ID not specified"
        return $E_BAD_ARGS
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_RDS_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_RDS_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_RDS_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        $(which aws) ${AWS_REGION_STRING} rds describe-db-clusters --db-cluster-identifier "${TMP_CLUSTER_ID}" --no-paginate --query "DBClusters[].[Status]" --output text >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
        RETURNVAL="$?"
        if [ ${RETURNVAL} -ne 0 ]; then
            if grep -q "DBClusterNotFoundFault" "${AWS_FILE_ERROR}"; then
                TMP_STRING="NOT_FOUND"
                eval $__RESULTVAR="'$(echo "$(trim "${TMP_STRING}")" | sed -e 's/^ *//g' -e 's/ *$//g')'"
                return 0
            fi
            log_add_from_file "${AWS_FILE_ERROR}" "${FUNCTION_DESCRIPTION}: Data containing error"
            log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Data"
            log_error "${FUNCTION_DESCRIPTION}: operation failed (aws_cli_exit_code::${RETURNVAL}])"
            RETRY_ENABLED=yes
            # Reset Files
            > ${AWS_FILE_RESPONSE}
            > ${AWS_FILE_ERROR}
        else
            TMP_STRING="$(cat ${AWS_FILE_RESPONSE} | tr '[:lower:]' '[:upper:]')"
            eval $__RESULTVAR="'$(echo "$(trim "${TMP_STRING}")" | sed -e 's/^ *//g' -e 's/ *$//g')'"
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: rds_get_current_cluster_id()
## - Retrieves current Cluster ID for RDS
## - Arguments
##   - $1: Variable name to pass info to
##   - $2: Cluster Reference Name
##   - $3: Region
function rds_get_current_cluster_id() {
    local FUNCTION_DESCRIPTION="RDS (Get Current Cluster ID)"
    local __RESULTVAR=$1
    local TMP_REFERENCE_NAME="${2}"
    local TMP_AWS_REGION="${3}"

    local RETURNVAL=""
    local TMP_STRING=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    if(is_empty "${TMP_REFERENCE_NAME}"); then
        log_error "${FUNCTION_DESCRIPTION}: Cluster Reference Name not specified"
        return $E_BAD_ARGS
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_RDS_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_RDS_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_RDS_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        $(which aws) ${AWS_REGION_STRING} rds describe-db-clusters --no-paginate --query "DBClusters[].[ClusterCreateTime,DBClusterIdentifier,Status]" --output text 2>${AWS_FILE_ERROR} | awk '$3 !~ /^deleting/' | awk "\$2 ~ \"^${TMP_REFERENCE_NAME}\"" | sort -r | head -1 | awk '{print $2}' >${AWS_FILE_RESPONSE}
        RETURNVAL="$?"
        # Fix Response File
        $(which sed) -i "s/\o15/_AWS_BAD_IGNORE\\n/g" "${AWS_FILE_RESPONSE}"
        $(which sed) -i '/_AWS_BAD_IGNORE/d' "${AWS_FILE_RESPONSE}"
        if [ ${RETURNVAL} -ne 0 ]; then
            if grep -q "DBClusterNotFoundFault" "${AWS_FILE_ERROR}"; then
                eval $__RESULTVAR=""
                return 0
            fi
            log_add_from_file "${AWS_FILE_ERROR}" "${FUNCTION_DESCRIPTION}: Data containing error"
            log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Data"
            log_error "${FUNCTION_DESCRIPTION}: operation failed (aws_cli_exit_code::${RETURNVAL}])"
            RETRY_ENABLED=yes
            # Reset Files
            > ${AWS_FILE_RESPONSE}
            > ${AWS_FILE_ERROR}
        else
            TMP_STRING="$(cat ${AWS_FILE_RESPONSE} | head -1)"
            if(is_empty "${TMP_STRING}"); then
                log "- Cluster not found"
            fi
            eval $__RESULTVAR="'$(echo "$(trim "${TMP_STRING}")" | sed -e 's/^ *//g' -e 's/ *$//g')'"
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: rds_get_current_database_id()
## - Retrieves current Database ID for RDS
## - Arguments
##   - $1: Variable name to pass info to
##   - $2: Database Reference Name
##   - $3: Region
function rds_get_current_database_id() {
    local FUNCTION_DESCRIPTION="RDS (Get Current Database ID)"
    local __RESULTVAR=$1
    local TMP_REFERENCE_NAME="${2}"
    local TMP_AWS_REGION="${3}"

    local RETURNVAL=""
    local TMP_STRING=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    if(is_empty "${TMP_REFERENCE_NAME}"); then
        log_error "${FUNCTION_DESCRIPTION}: Database Reference Name not specified"
        return $E_BAD_ARGS
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_RDS_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_RDS_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_RDS_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        $(which aws) ${AWS_REGION_STRING} rds describe-db-instances --no-paginate --query "DBInstances[].[InstanceCreateTime,DBInstanceIdentifier,StatusInfos[0].StatusType,DBInstanceStatus]" --output text 2>${AWS_FILE_ERROR} | awk '$4 !~ /^deleting/' | awk '$3 !~ /^read/' | awk "\$2 ~ \"^${TMP_REFERENCE_NAME}\"" | sort -r | head -1 | awk '{print $2}' >${AWS_FILE_RESPONSE}
        RETURNVAL="$?"
        # Fix Response File
        $(which sed) -i "s/\o15/_AWS_BAD_IGNORE\\n/g" "${AWS_FILE_RESPONSE}"
        $(which sed) -i '/_AWS_BAD_IGNORE/d' "${AWS_FILE_RESPONSE}"
        if [ ${RETURNVAL} -ne 0 ]; then
            if grep -q "DBInstanceNotFound" "${AWS_FILE_ERROR}"; then
                eval $__RESULTVAR=""
                return 0
            fi
            log_add_from_file "${AWS_FILE_ERROR}" "${FUNCTION_DESCRIPTION}: Data containing error"
            log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Data"
            log_error "${FUNCTION_DESCRIPTION}: operation failed (aws_cli_exit_code::${RETURNVAL}])"
            RETRY_ENABLED=yes
            # Reset Files
            > ${AWS_FILE_RESPONSE}
            > ${AWS_FILE_ERROR}
        else
            TMP_STRING="$(cat ${AWS_FILE_RESPONSE} | head -1)"
            if(is_empty "${TMP_STRING}"); then
                log "- Database not found"
            fi
            eval $__RESULTVAR="'$(echo "$(trim "${TMP_STRING}")" | sed -e 's/^ *//g' -e 's/ *$//g')'"
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: rds_get_database_replica_ids()
## - Retrieves current Database Replica IDs for RDS Database
## - Arguments
##   - $1: Array name to pass info to
##   - $2: Database ID
##   - $3: Region
function rds_get_database_replica_ids() {
    local FUNCTION_DESCRIPTION="RDS (Get Replica IDs)"
    local __RESULTVAR=$1
    local TMP_DATABASE_ID="${2}"
    local TMP_AWS_REGION="${3}"

    local RETURNVAL=""
    local TMP_STRING=""
    local TMP_KEY=""
    local TMP_ARRAY_REPLICA=()
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    if(is_empty "${TMP_DATABASE_ID}"); then
        log_error "${FUNCTION_DESCRIPTION}: Database ID not specified"
        return $E_BAD_ARGS
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_RDS_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_RDS_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_RDS_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        log "${FUNCTION_DESCRIPTION}: Retrieving Replia IDs (database_id: [${TMP_DATABASE_ID}] / region: [${TMP_AWS_REGION}]) (Attempt::${COUNTER} of ${AWS_RDS_DEFAULT_RETRY_COUNT})"
        $(which aws) ${AWS_REGION_STRING} rds describe-db-instances --db-instance-identifier "${TMP_DATABASE_ID}" --no-paginate --query "DBInstances[].[ReadReplicaDBInstanceIdentifiers]" --output text 2>${AWS_FILE_ERROR} | sed -e 's/\t/ /' >${AWS_FILE_RESPONSE}
        RETURNVAL="$?"
        # Fix Response File
        $(which sed) -i "s/\o15/_AWS_BAD_IGNORE\\n/g" "${AWS_FILE_RESPONSE}"
        $(which sed) -i '/_AWS_BAD_IGNORE/d' "${AWS_FILE_RESPONSE}"
        if [ ${RETURNVAL} -ne 0 ]; then
            if grep -q "DBInstanceNotFound" "${AWS_FILE_ERROR}"; then
                eval $__RESULTVAR=""
                return 0
            fi
            log_add_from_file "${AWS_FILE_ERROR}" "${FUNCTION_DESCRIPTION}: Data containing error"
            log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Data"
            log_error "${FUNCTION_DESCRIPTION}: operation failed (aws_cli_exit_code::${RETURNVAL}])"
            RETRY_ENABLED=yes
            # Reset Files
            > ${AWS_FILE_RESPONSE}
            > ${AWS_FILE_ERROR}
        else
            TMP_STRING="$(cat ${AWS_FILE_RESPONSE} | head -1)"
            if(is_empty "${TMP_STRING}"); then
                log "${FUNCTION_DESCRIPTION}: No Replicas found"
            else
                TMP_ARRAY_REPLICA=(${TMP_STRING})
                readarray -t TMP_ARRAY_REPLICA < <(for a in "${TMP_ARRAY_REPLICA[@]}"; do echo "${a}"; done | grep -v '^$' | sort | uniq)
                log_success "${FUNCTION_DESCRIPTION}: Successfully retrieved Replica IDs [replica_count: ${#TMP_ARRAY_REPLICA[@]}]"
                for TMP_KEY in "${TMP_ARRAY_REPLICA[@]}"; do
                    log "- [${TMP_KEY}]"
                done
                eval $__RESULTVAR="(${TMP_ARRAY_REPLICA[@]})"
            fi
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: rds_get_endpoint()
## - Retrieves Endpoint for specified Database
## - Arguments
##   - $1: Variable name to pass info to
##   - $2: Database ID
##   - $3: Region
function rds_get_endpoint() {
    local FUNCTION_DESCRIPTION="RDS (Get Endpoint)"
    local __RESULTVAR=$1
    local TMP_DATABASE_ID="${2}"
    local TMP_AWS_REGION="${3}"

    local RETURNVAL=""
    local TMP_STRING=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    if(is_empty "${TMP_DATABASE_ID}"); then
        log_error "${FUNCTION_DESCRIPTION}: Database ID not specified"
        return $E_BAD_ARGS
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_RDS_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_RDS_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_RDS_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        $(which aws) ${AWS_REGION_STRING} rds describe-db-instances --db-instance-identifier "${TMP_DATABASE_ID}" --no-paginate --query "DBInstances[].[Endpoint.Address]" --output text >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
        RETURNVAL="$?"
        if [ ${RETURNVAL} -ne 0 ]; then
            log_add_from_file "${AWS_FILE_ERROR}" "${FUNCTION_DESCRIPTION}: Data containing error"
            log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Data"
            log_error "${FUNCTION_DESCRIPTION}: operation failed (aws_cli_exit_code::${RETURNVAL}])"
            RETRY_ENABLED=yes
            # Reset Files
            > ${AWS_FILE_RESPONSE}
            > ${AWS_FILE_ERROR}
        else
            TMP_STRING="$(cat ${AWS_FILE_RESPONSE} | tr '[:upper:]' '[:lower:]')"
            eval $__RESULTVAR="'$(echo "$(trim "${TMP_STRING}")" | sed -e 's/^ *//g' -e 's/ *$//g')'"
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: rds_get_engine_version()
## - Retrieves engine version for specified Database
## - Arguments
##   - $1: Variable name to pass info to
##   - $2: Database ID
##   - $3: Region
function rds_get_engine_version() {
    local FUNCTION_DESCRIPTION="RDS (Get Engine Version)"
    local __RESULTVAR=$1
    local TMP_DATABASE_ID="${2}"
    local TMP_AWS_REGION="${3}"

    local RETURNVAL=""
    local TMP_STRING=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    if(is_empty "${TMP_DATABASE_ID}"); then
        log_error "${FUNCTION_DESCRIPTION}: Database ID not specified"
        return $E_BAD_ARGS
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_RDS_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_RDS_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_RDS_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        $(which aws) ${AWS_REGION_STRING} rds describe-db-instances --db-instance-identifier "${TMP_DATABASE_ID}" --no-paginate --query "DBInstances[].[EngineVersion]" --output text >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
        RETURNVAL="$?"
        if [ ${RETURNVAL} -ne 0 ]; then
            if grep -q "DBInstanceNotFound" "${AWS_FILE_ERROR}"; then
                log_error "${FUNCTION_DESCRIPTION}: database not found [${TMP_DATABASE_ID}]"
                return $E_AWS_FAILURE
            fi
            log_add_from_file "${AWS_FILE_ERROR}" "${FUNCTION_DESCRIPTION}: Data containing error"
            log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Data"
            log_error "${FUNCTION_DESCRIPTION}: operation failed (aws_cli_exit_code::${RETURNVAL}])"
            RETRY_ENABLED=yes
            # Reset Files
            > ${AWS_FILE_RESPONSE}
            > ${AWS_FILE_ERROR}
        else
            TMP_STRING="$(cat ${AWS_FILE_RESPONSE} | head -1)"
            eval $__RESULTVAR="'$(echo "$(trim "${TMP_STRING}")" | sed -e 's/^ *//g' -e 's/ *$//g')'"
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: rds_get_latest_cluster_snapshot_arn()
## - Retrieves latest snapshot ARN for specified Cluster ID
## - Arguments
##   - $1: Variable name to pass info to
##   - $2: Database ID
##   - $3: Region
function rds_get_latest_cluster_snapshot_arn() {
    local FUNCTION_DESCRIPTION="RDS (Get Latest Cluster Snapshot)"
    local __RESULTVAR=$1
    local TMP_CLUSTER_ID="${2}"
    local TMP_AWS_REGION="${3}"

    local RETURNVAL=""
    local TMP_STRING=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    if(is_empty "${TMP_CLUSTER_ID}"); then
        log_error "${FUNCTION_DESCRIPTION}: Cluster ID not specified"
        return $E_BAD_ARGS
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_RDS_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_RDS_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_RDS_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no
        $(which aws) ${AWS_REGION_STRING} rds describe-db-cluster-snapshots --db-cluster-identifier "${TMP_CLUSTER_ID}" --no-paginate --query "DBClusterSnapshots[].[SnapshotCreateTime,DBClusterSnapshotArn,Status]" --output text 2>${AWS_FILE_ERROR} | awk "\$3 ~ \"^available\"" | sort -r | head -1 | awk '{print $2}' >${AWS_FILE_RESPONSE}
        RETURNVAL="$?"
        # Fix Response File
        $(which sed) -i "s/\o15/_AWS_BAD_IGNORE\\n/g" "${AWS_FILE_RESPONSE}"
        $(which sed) -i '/_AWS_BAD_IGNORE/d' "${AWS_FILE_RESPONSE}"
        if [ ${RETURNVAL} -ne 0 ]; then
            log_add_from_file "${AWS_FILE_ERROR}" "${FUNCTION_DESCRIPTION}: Data containing error"
            log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Data"
            log_error "${FUNCTION_DESCRIPTION}: operation failed (aws_cli_exit_code::${RETURNVAL}])"
            RETRY_ENABLED=yes
            # Reset Files
            > ${AWS_FILE_RESPONSE}
            > ${AWS_FILE_ERROR}
        else
            TMP_STRING="$(cat ${AWS_FILE_RESPONSE} | head -1)"
            if(is_empty "${TMP_STRING}"); then
                log_warning "${FUNCTION_DESCRIPTION}: No snapshot found"
            fi
            eval $__RESULTVAR="'$(echo "$(trim "${TMP_STRING}")" | sed -e 's/^ *//g' -e 's/ *$//g')'"
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: rds_get_latest_snapshot_arn()
## - Retrieves latest snapshot ARN for specified Database ID
## - Arguments
##   - $1: Variable name to pass info to
##   - $2: Database ID
##   - $3: Region
function rds_get_latest_snapshot_arn() {
    local FUNCTION_DESCRIPTION="RDS (Get Latest Snapshot)"
    local __RESULTVAR=$1
    local TMP_DATABASE_ID="${2}"
    local TMP_AWS_REGION="${3}"

    local RETURNVAL=""
    local TMP_STRING=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    if(is_empty "${TMP_DATABASE_ID}"); then
        log_error "${FUNCTION_DESCRIPTION}: Database ID not specified"
        return $E_BAD_ARGS
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_RDS_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_RDS_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_RDS_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        $(which aws) ${AWS_REGION_STRING} rds describe-db-snapshots --db-instance-identifier "${TMP_DATABASE_ID}" --no-paginate --query "DBSnapshots[].[SnapshotCreateTime,DBSnapshotArn,Status]" --output text 2>${AWS_FILE_ERROR} | awk "\$3 ~ \"^available\"" | sort -r | head -1 | awk '{print $2}' >${AWS_FILE_RESPONSE}
        RETURNVAL="$?"
        # Fix Response File
        $(which sed) -i "s/\o15/_AWS_BAD_IGNORE\\n/g" "${AWS_FILE_RESPONSE}"
        $(which sed) -i '/_AWS_BAD_IGNORE/d' "${AWS_FILE_RESPONSE}"
        if [ ${RETURNVAL} -ne 0 ]; then
            log_add_from_file "${AWS_FILE_ERROR}" "${FUNCTION_DESCRIPTION}: Data containing error"
            log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Data"
            log_error "${FUNCTION_DESCRIPTION}: operation failed (aws_cli_exit_code::${RETURNVAL}])"
            RETRY_ENABLED=yes
            # Reset Files
            > ${AWS_FILE_RESPONSE}
            > ${AWS_FILE_ERROR}
        else
            TMP_STRING="$(cat ${AWS_FILE_RESPONSE} | head -1)"
            if(is_empty "${TMP_STRING}"); then
                log_warning "${FUNCTION_DESCRIPTION}: No snapshot found"
            fi
            eval $__RESULTVAR="'$(echo "$(trim "${TMP_STRING}")" | sed -e 's/^ *//g' -e 's/ *$//g')'"
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: rds_get_parameter_group()
## - Retrieves parameter group for specified Database
## - Arguments
##   - $1: Variable name to pass info to
##   - $2: Database ID
##   - $3: Region
function rds_get_parameter_group() {
    local FUNCTION_DESCRIPTION="RDS (Get Parameter Group)"
    local __RESULTVAR=$1
    local TMP_DATABASE_ID="${2}"
    local TMP_AWS_REGION="${3}"

    local RETURNVAL=""
    local TMP_STRING=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    if(is_empty "${TMP_DATABASE_ID}"); then
        log_error "${FUNCTION_DESCRIPTION}: Database ID not specified"
        return $E_BAD_ARGS
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_RDS_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_RDS_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_RDS_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        $(which aws) ${AWS_REGION_STRING} rds describe-db-instances --db-instance-identifier "${TMP_DATABASE_ID}" --no-paginate --query "DBInstances[].[DBParameterGroups[0].DBParameterGroupName]" --output text >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
        RETURNVAL="$?"
        if [ ${RETURNVAL} -ne 0 ]; then
            if grep -q "DBInstanceNotFound" "${AWS_FILE_ERROR}"; then
                log_error "${FUNCTION_DESCRIPTION}: database not found [${TMP_DATABASE_ID}]"
                return $E_AWS_FAILURE
            fi
            log_add_from_file "${AWS_FILE_ERROR}" "${FUNCTION_DESCRIPTION}: Data containing error"
            log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Data"
            log_error "${FUNCTION_DESCRIPTION}: operation failed (aws_cli_exit_code::${RETURNVAL}])"
            RETRY_ENABLED=yes
            # Reset Files
            > ${AWS_FILE_RESPONSE}
            > ${AWS_FILE_ERROR}
        else
            TMP_STRING="$(cat ${AWS_FILE_RESPONSE} | head -1)"
            eval $__RESULTVAR="'$(echo "$(trim "${TMP_STRING}")" | sed -e 's/^ *//g' -e 's/ *$//g')'"
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: rds_get_status()
## - Retrieves status for specified Database
## - Arguments
##   - $1: Variable name to pass info to
##   - $2: Database ID
##   - $3: Region
function rds_get_status() {
    local FUNCTION_DESCRIPTION="RDS (Get Status)"
    local __RESULTVAR=$1
    local TMP_DATABASE_ID="${2}"
    local TMP_AWS_REGION="${3}"

    local RETURNVAL=""
    local TMP_STRING=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    if(is_empty "${TMP_DATABASE_ID}"); then
        log_error "${FUNCTION_DESCRIPTION}: Database ID not specified"
        return $E_BAD_ARGS
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_RDS_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_RDS_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_RDS_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        $(which aws) ${AWS_REGION_STRING} rds describe-db-instances --db-instance-identifier "${TMP_DATABASE_ID}" --no-paginate --query "DBInstances[].[DBInstanceStatus]" --output text >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
        RETURNVAL="$?"
        if [ ${RETURNVAL} -ne 0 ]; then
            if grep -q "DBInstanceNotFound" "${AWS_FILE_ERROR}"; then
                TMP_STRING="NOT_FOUND"
                eval $__RESULTVAR="'$(echo "$(trim "${TMP_STRING}")" | sed -e 's/^ *//g' -e 's/ *$//g')'"
                return 0
            fi
            log_add_from_file "${AWS_FILE_ERROR}" "${FUNCTION_DESCRIPTION}: Data containing error"
            log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Data"
            log_error "${FUNCTION_DESCRIPTION}: operation failed (aws_cli_exit_code::${RETURNVAL}])"
            RETRY_ENABLED=yes
            # Reset Files
            > ${AWS_FILE_RESPONSE}
            > ${AWS_FILE_ERROR}
        else
            TMP_STRING="$(cat ${AWS_FILE_RESPONSE} | tr '[:lower:]' '[:upper:]')"
            eval $__RESULTVAR="'$(echo "$(trim "${TMP_STRING}")" | sed -e 's/^ *//g' -e 's/ *$//g')'"
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: rds_is_db_pending_reboot()
## - Checks if Database is currently pending reboot
## - Arguments
##   - $1: Database ID
##   - $2: Region
function rds_is_db_pending_reboot() {
    local TMP_DATABASE_ID="${1}"
    local TMP_AWS_REGION="${2}"

    local AWS_REGION_STRING=""

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    if $($(which aws) ${AWS_REGION_STRING} rds describe-db-instances --db-instance-identifier "${TMP_DATABASE_ID}" 2>/dev/null | grep --quiet "pending-reboot"); then
        return 0
    else
        return 1
    fi
}

###------------------------------------------------------------------------------------------------
## FUNCTION: rds_poll_cluster_status()
## - Polls RDS Cluster Status and waits for a stable state
## - Arguments
##   - $1: Cluster ID
##   - $2: Fail with error on NOT_FOUND <yes/no> (defaults to yes)
##   - $3: Verification Timeout <in minutes, defaults to 30 minutes>
##   - $4: Verification Time between polling <in seconds, defaults to 30 seconds)
##   - $5: Region
function rds_poll_cluster_status() {
    local FUNCTION_DESCRIPTION="RDS (Poll Cluster Status)"
    local TMP_CLUSTER_ID="${1}"
    local ERROR_ON_NOT_FOUND="${2}"
    local AWS_OPERATION_TIMEOUT="${3}"
    local AWS_RDS_VERIFICATION_SLEEP="${4}"
    local TMP_AWS_REGION="${5}"

    local RETURNVAL=""
    local TMP_STRING=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    local POLLING_PROGRESS=1
    local POLLING_STATUS=""
    local TMP_STATUS=""
    local TMP_STATUS_DESCRIPTION=""
    local TMP_LAST_STATUS=""
    local AWS_RDS_VERIFICATION_MAX_ATTEMPTS=""

    if(is_empty "${TMP_CLUSTER_ID}"); then
        log_error "${FUNCTION_DESCRIPTION}: Cluster ID not specified"
        return $E_BAD_ARGS
    fi

    if(is_empty "${ERROR_ON_NOT_FOUND}"); then
        ERROR_ON_NOT_FOUND=yes
    fi
    if(is_empty "${AWS_OPERATION_TIMEOUT}"); then
        AWS_OPERATION_TIMEOUT=${AWS_RDS_DEFAULT_VERIFICATION_TIMEOUT}
    fi
    if(is_empty "${AWS_RDS_VERIFICATION_SLEEP}"); then
        AWS_RDS_VERIFICATION_SLEEP=${AWS_RDS_DEFAULT_VERIFICATION_SLEEP}
    fi
    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi
    AWS_RDS_VERIFICATION_MAX_ATTEMPTS=$((AWS_OPERATION_TIMEOUT * 60 / AWS_RDS_VERIFICATION_SLEEP))

    log "${FUNCTION_DESCRIPTION}: started (cluster_id: [${TMP_CLUSTER_ID}] / timeout: [${AWS_OPERATION_TIMEOUT}] / attempts: [${AWS_RDS_VERIFICATION_MAX_ATTEMPTS}] / polling_interval: [${AWS_RDS_VERIFICATION_SLEEP} seconds])"
    while [ ${POLLING_PROGRESS} == 1 ]; do
        if [[ "ZZ_${TMP_STATUS}" != "ZZ_${TMP_LAST_STATUS}" ]]; then
            if(! is_empty "${TMP_LAST_STATUS}"); then
                log "-- Status Change Detected, resetting counter [$(color_text "${LOG_COLOR_MESSAGE_IMPORTANT}" "${TMP_LAST_STATUS}") => $(color_text "${LOG_COLOR_MESSAGE_IMPORTANT}" "${TMP_STATUS}")]"
                COUNTER=0
            fi
            TMP_LAST_STATUS="${TMP_STATUS}"
        fi
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_RDS_VERIFICATION_MAX_ATTEMPTS} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_RDS_VERIFICATION_MAX_ATTEMPTS}] exceeded, aborting tracking operation"
            return $E_RDS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            sleep ${AWS_RDS_VERIFICATION_SLEEP}
        fi

        rds_get_cluster_status TMP_STATUS "${TMP_CLUSTER_ID}" "${TMP_AWS_REGION}"
        RETURNVAL="$?"
        if [ ${RETURNVAL} -ne 0 ]; then
            return $RETURNVAL
        fi

        case "${TMP_STATUS}" in
            "AVAILABLE")
                TMP_STATUS_DESCRIPTION="The DB cluster is healthy and available"
                POLLING_PROGRESS=0
                POLLING_STATUS=0
                ;;
            "BACKING-UP")
                TMP_STATUS_DESCRIPTION="The DB cluster is currently being backed up"
                POLLING_PROGRESS=1
                ;;
            "BACKTRACKING")
                TMP_STATUS_DESCRIPTION="The DB cluster is currently being backtracked"
                POLLING_PROGRESS=1
                ;;
            "CLONING-FAILED")
                TMP_STATUS_DESCRIPTION="Cloning a DB cluster failed"
                POLLING_PROGRESS=0
                POLLING_STATUS=$E_RDS_FAILURE
                ;;
            "CREATING")
                TMP_STATUS_DESCRIPTION="The DB cluster is being created. The DB cluster is inaccessible while it is being created"
                POLLING_PROGRESS=1
                ;;
            "DELETING")
                TMP_STATUS_DESCRIPTION="The DB cluster is being deleted"
                POLLING_PROGRESS=1
                ;;
            "FAILING-OVER")
                TMP_STATUS_DESCRIPTION="A failover from the primary instance to an Aurora Replica is being performed"
                POLLING_PROGRESS=1
                ;;
            "INACCESSIBLE-ENCRYPTION-CREDENTIALS")
                TMP_STATUS_DESCRIPTION="The AWS KMS key used to encrypt or decrypt the DB cluster can't be accessed"
                POLLING_PROGRESS=0
                POLLING_STATUS=$E_RDS_FAILURE
                ;;
            "MAINTENANCE")
                TMP_STATUS_DESCRIPTION="Amazon RDS is applying a maintenance update to the DB cluster"
                POLLING_PROGRESS=1
                ;;
            "MIGRATING")
                TMP_STATUS_DESCRIPTION="A DB cluster snapshot is being restored to a DB cluster"
                POLLING_PROGRESS=1
                ;;
            "MIGRATION-FAILED")
                TMP_STATUS_DESCRIPTION="A migration failed"
                POLLING_PROGRESS=0
                POLLING_STATUS=$E_RDS_FAILURE
                ;;
            "MODIFYING")
                TMP_STATUS_DESCRIPTION="The DB cluster is being modified because of a customer request to modify the DB cluster"
                POLLING_PROGRESS=1
                ;;
            "NOT_FOUND")
                TMP_STATUS_DESCRIPTION="Database not found"
                POLLING_PROGRESS=0
                if option_enabled ERROR_ON_NOT_FOUND; then
                    POLLING_STATUS=$E_RDS_FAILURE
                else
                    POLLING_STATUS=0
                fi
                ;;
            "PROMOTING")
                TMP_STATUS_DESCRIPTION="A Read Replica is being promoted to a standalone DB cluster"
                POLLING_PROGRESS=1
                ;;
            "RENAMING")
                TMP_STATUS_DESCRIPTION="The DB cluster is being renamed because of a customer request to rename it"
                POLLING_PROGRESS=1
                ;;
            "RESETTING-MASTER-CREDENTIALS")
                TMP_STATUS_DESCRIPTION="The master credentials for the DB cluster are being reset because of a customer request to reset them"
                POLLING_PROGRESS=1
                ;;
            "STARTING")
                TMP_STATUS_DESCRIPTION="The DB cluster is starting"
                POLLING_PROGRESS=1
                ;;
            "STOPPING")
                TMP_STATUS_DESCRIPTION="The DB cluster is being stopped"
                POLLING_PROGRESS=1
                ;;
            "STOPPED")
                TMP_STATUS_DESCRIPTION="The DB cluster is stopped"
                POLLING_PROGRESS=0
                POLLING_STATUS=0
                ;;
            "UPDATE-IAM-DB-AUTH")
                TMP_STATUS_DESCRIPTION="IAM authorization for the DB cluster is being updated"
                POLLING_PROGRESS=1
                ;;
            "UPGRADING")
                TMP_STATUS_DESCRIPTION="The database engine version is being upgraded"
                POLLING_PROGRESS=1
                ;;
            *)
                TMP_STATUS_DESCRIPTION="n/a"
                POLLING_PROGRESS=1
                ;;
            ?)
                TMP_STATUS_DESCRIPTION="n/a"
                POLLING_PROGRESS=1
                ;;
        esac
        RETRY_ENABLED=yes

        log "- Attempt: [$(printf "%02d\n" "${COUNTER}")/${AWS_RDS_VERIFICATION_MAX_ATTEMPTS}] | Status: [$(color_text "${LOG_COLOR_MESSAGE_IMPORTANT}" "${TMP_STATUS}")] | Description: [$(color_text "${LOG_COLOR_NOTICE}" "${TMP_STATUS_DESCRIPTION}")]"
    done

    return $POLLING_STATUS
}

###------------------------------------------------------------------------------------------------
## FUNCTION: rds_poll_status()
## - Polls RDS Database Status and waits for a stable state
## - Arguments
##   - $1: Database ID
##   - $2: Fail with error on NOT_FOUND <yes/no> (defaults to yes)
##   - $3: Verification Timeout <in minutes, defaults to 30 minutes>
##   - $4: Verification Time between polling <in seconds, defaults to 30 seconds)
##   - $5: Region
function rds_poll_status() {
    local FUNCTION_DESCRIPTION="RDS (Poll Status)"
    local TMP_DATABASE_ID="${1}"
    local ERROR_ON_NOT_FOUND="${2}"
    local AWS_OPERATION_TIMEOUT="${3}"
    local AWS_RDS_VERIFICATION_SLEEP="${4}"
    local TMP_AWS_REGION="${5}"

    local RETURNVAL=""
    local TMP_STRING=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    local POLLING_PROGRESS=1
    local POLLING_STATUS=""
    local TMP_STATUS=""
    local TMP_STATUS_DESCRIPTION=""
    local TMP_LAST_STATUS=""
    local AWS_RDS_VERIFICATION_MAX_ATTEMPTS=""

    if(is_empty "${TMP_DATABASE_ID}"); then
        log_error "${FUNCTION_DESCRIPTION}: Database ID not specified"
        return $E_BAD_ARGS
    fi

    if(is_empty "${ERROR_ON_NOT_FOUND}"); then
        ERROR_ON_NOT_FOUND=yes
    fi
    if(is_empty "${AWS_OPERATION_TIMEOUT}"); then
        AWS_OPERATION_TIMEOUT=${AWS_RDS_DEFAULT_VERIFICATION_TIMEOUT}
    fi
    if(is_empty "${AWS_RDS_VERIFICATION_SLEEP}"); then
        AWS_RDS_VERIFICATION_SLEEP=${AWS_RDS_DEFAULT_VERIFICATION_SLEEP}
    fi
    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi
    AWS_RDS_VERIFICATION_MAX_ATTEMPTS=$((AWS_OPERATION_TIMEOUT * 60 / AWS_RDS_VERIFICATION_SLEEP))

    log "${FUNCTION_DESCRIPTION}: started (database_id: [${TMP_DATABASE_ID}] / timeout: [${AWS_OPERATION_TIMEOUT}] / attempts: [${AWS_RDS_VERIFICATION_MAX_ATTEMPTS}] / polling_interval: [${AWS_RDS_VERIFICATION_SLEEP} seconds])"
    while [ ${POLLING_PROGRESS} == 1 ]; do
        if [[ "ZZ_${TMP_STATUS}" != "ZZ_${TMP_LAST_STATUS}" ]]; then
            if(! is_empty "${TMP_LAST_STATUS}"); then
                log "-- Status Change Detected, resetting counter [$(color_text "${LOG_COLOR_MESSAGE_IMPORTANT}" "${TMP_LAST_STATUS}") => $(color_text "${LOG_COLOR_MESSAGE_IMPORTANT}" "${TMP_STATUS}")]"
                COUNTER=0
            fi
            TMP_LAST_STATUS="${TMP_STATUS}"
        fi
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_RDS_VERIFICATION_MAX_ATTEMPTS} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_RDS_VERIFICATION_MAX_ATTEMPTS}] exceeded, aborting tracking operation"
            return $E_RDS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            sleep ${AWS_RDS_VERIFICATION_SLEEP}
        fi

        rds_get_status TMP_STATUS "${TMP_DATABASE_ID}" "${TMP_AWS_REGION}"
        RETURNVAL="$?"
        if [ ${RETURNVAL} -ne 0 ]; then
            return $RETURNVAL
        fi

        case "${TMP_STATUS}" in
            "AVAILABLE")
                TMP_STATUS_DESCRIPTION="The instance is healthy and available"
                POLLING_PROGRESS=0
                POLLING_STATUS=0
                ;;
            "BACKING-UP")
                TMP_STATUS_DESCRIPTION="The instance is currently being backed up"
                POLLING_PROGRESS=1
                ;;
            "CONFIGURING-ENHANCED-MONITORING")
                TMP_STATUS_DESCRIPTION="Enhanced Monitoring is being enabled or disabled for this instance"
                POLLING_PROGRESS=1
                ;;
            "CREATING")
                TMP_STATUS_DESCRIPTION="The instance is being created. The instance is inaccessible while it is being created"
                POLLING_PROGRESS=1
                ;;
            "DELETING")
                TMP_STATUS_DESCRIPTION="The instance is being deleted"
                POLLING_PROGRESS=1
                ;;
            "FAILED")
                TMP_STATUS_DESCRIPTION="The instance has failed and Amazon RDS was unable to recover it.  Perform a point-in-time restore to the latest restorable time of the instance to recover the data"
                POLLING_PROGRESS=0
                POLLING_STATUS=$E_RDS_FAILURE
                ;;
            "INACCESSIBLE-ENCRYPTION-CREDENTIALS")
                TMP_STATUS_DESCRIPTION="The KMS key used to encrypt or decrypt the DB instance could not be accessed"
                POLLING_PROGRESS=0
                POLLING_STATUS=$E_RDS_FAILURE
                ;;
            "INCOMPATIBLE-CREDENTIALS")
                TMP_STATUS_DESCRIPTION="The supplied CloudHSM username or password is incorrect. Please update the CloudHSM credentials for the DB instance"
                POLLING_PROGRESS=0
                POLLING_STATUS=$E_RDS_FAILURE
                ;;
            "INCOMPATIBLE-NETWORK")
                TMP_STATUS_DESCRIPTION="Amazon RDS is attempting to perform a recovery action on an instance but is unable to do so because the VPC is in a state that is preventing the action from being completed.  This status can occur if, for example, all available IP addresses in a subnet were in use and Amazon RDS was unable to get an IP address for the DB instance"
                POLLING_PROGRESS=0
                POLLING_STATUS=$E_RDS_FAILURE
                ;;
            "INCOMPATIBLE-OPTION-GROUP")
                TMP_STATUS_DESCRIPTION="Amazon RDS attempted to apply an option group change but was unable to do so, and Amazon RDS was unable to roll back to the previous option group state. Consult the Recent Events list for the DB instance for more information. This status can occur if, for example, the option group contains an option such as TDE and the DB instance does not contain encrypted information"
                POLLING_PROGRESS=0
                POLLING_STATUS=$E_RDS_FAILURE
                ;;
            "INCOMPATIBLE-PARAMETERS")
                TMP_STATUS_DESCRIPTION="Amazon RDS was unable to start up the DB instance because the parameters specified in the instance's DB parameter group were not compatible. Revert the parameter changes or make them compatible with the instance to regain access to your instance. Consult the Recent Events list for the DB instance for more information about the incompatible parameters"
                POLLING_PROGRESS=0
                POLLING_STATUS=$E_RDS_FAILURE
                ;;
            "INCOMPATIBLE-RESTORE")
                TMP_STATUS_DESCRIPTION="Amazon RDS is unable to do a point-in-time restore. Common causes for this status include using temp tables, using MyISAM tables with MySQL, or using Aria tables with MariaDB"
                POLLING_PROGRESS=0
                POLLING_STATUS=$E_RDS_FAILURE
                ;;
            "MAINTENANCE")
                TMP_STATUS_DESCRIPTION="Amazon RDS is applying a maintenance update to the DB instance. This status is used for instance-level maintenance that RDS schedules well in advance. We're evaluating ways to expose additional maintenance actions to customers through this status"
                POLLING_PROGRESS=1
                ;;
            "MODIFYING")
                TMP_STATUS_DESCRIPTION="The instance is being modified because of a customer request to modify the instance"
                POLLING_PROGRESS=1
                ;;
            "NOT_FOUND")
                TMP_STATUS_DESCRIPTION="Database not found"
                POLLING_PROGRESS=0
                if option_enabled ERROR_ON_NOT_FOUND; then
                    POLLING_STATUS=$E_RDS_FAILURE
                else
                    POLLING_STATUS=0
                fi
                ;;
            "REBOOTING")
                TMP_STATUS_DESCRIPTION="The instance is being rebooted because of a customer request or an Amazon RDS process that requires the rebooting of the instance"
                POLLING_PROGRESS=1
                ;;
            "RENAMING")
                TMP_STATUS_DESCRIPTION="The instance is being renamed because of a customer request to rename it"
                POLLING_PROGRESS=1
                ;;
            "RESETTING-MASTER-CREDENTIALS")
                TMP_STATUS_DESCRIPTION="The master credentials for the instance are being reset because of a customer request to reset them"
                POLLING_PROGRESS=1
                ;;
            "RESTORE-ERROR")
                TMP_STATUS_DESCRIPTION="The DB instance encountered an error attempting to restore to a point-in-time or from a snapshot"
                POLLING_PROGRESS=0
                POLLING_STATUS=$E_RDS_FAILURE
                ;;
            "STARTING")
                TMP_STATUS_DESCRIPTION="The DB instance is starting"
                POLLING_PROGRESS=1
                ;;
            "STOPPING")
                TMP_STATUS_DESCRIPTION="The DB instance is being stopped"
                POLLING_PROGRESS=1
                ;;
            "STOPPED")
                TMP_STATUS_DESCRIPTION="The DB instance is stopped"
                POLLING_PROGRESS=0
                POLLING_STATUS=0
                ;;
            "STORAGE-FULL")
                TMP_STATUS_DESCRIPTION="The instance has reached its storage capacity allocation. This is a critical status and should be remedied immediately; you should scale up your storage by modifying the DB instance. Set CloudWatch alarms to warn you when storage space is getting low so you don't run into this situation"
                POLLING_PROGRESS=0
                POLLING_STATUS=$E_RDS_FAILURE
                ;;
            "UPGRADING")
                TMP_STATUS_DESCRIPTION="The database engine version is being upgraded"
                POLLING_PROGRESS=1
                ;;
            *)
                TMP_STATUS_DESCRIPTION="n/a"
                POLLING_PROGRESS=1
                ;;
            ?)
                TMP_STATUS_DESCRIPTION="n/a"
                POLLING_PROGRESS=1
                ;;
        esac
        RETRY_ENABLED=yes

        log "- Attempt: [$(printf "%02d\n" "${COUNTER}")/${AWS_RDS_VERIFICATION_MAX_ATTEMPTS}] | Status: [$(color_text "${LOG_COLOR_MESSAGE_IMPORTANT}" "${TMP_STATUS}")] | Description: [$(color_text "${LOG_COLOR_NOTICE}" "${TMP_STATUS_DESCRIPTION}")]"
    done

    return $POLLING_STATUS
}

###------------------------------------------------------------------------------------------------
## FUNCTION: rds_reboot()
## - Reboots specified RDS Instance
## - Arguments
##   - $1: Database ID
##   - $2: Enable Failover (optional, defaults to no)
##   - $3: Verification Timeout <in minutes, defaults to 30 minutes>
##   - $4: Region
function rds_reboot() {
    local FUNCTION_DESCRIPTION="RDS (Reboot Database)"
    local TMP_DATABASE_ID="${1}"
    local TMP_ENABLE_FAILOVER="${2}"
    local AWS_OPERATION_TIMEOUT="${3}"
    local TMP_AWS_REGION="${4}"

    local RETURNVAL=""
    local TMP_STRING=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    local AWS_RDS_VERIFICATION_SLEEP=${AWS_RDS_DEFAULT_VERIFICATION_SLEEP}
    local ARG_FAILOVER=""

    if(is_empty "${TMP_DATABASE_ID}"); then
        log_error "${FUNCTION_DESCRIPTION}: Database ID not specified"
        return $E_BAD_ARGS
    fi

    if(is_empty "${TMP_ENABLE_FAILOVER}"); then
        TMP_ENABLE_FAILOVER=no
    fi
    if(is_empty "${AWS_OPERATION_TIMEOUT}"); then
        AWS_OPERATION_TIMEOUT=${AWS_RDS_DEFAULT_VERIFICATION_TIMEOUT}
    fi
    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    if option_enabled TMP_ENABLE_FAILOVER; then
        ARG_FAILOVER="--force-failover"
    else
        ARG_FAILOVER="--no-force-failover"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_RDS_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_RDS_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_RDS_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        log "${FUNCTION_DESCRIPTION}: Rebooting Database (database_id: [${TMP_DATABASE_ID}] / region: [${TMP_AWS_REGION}]) (Attempt::${COUNTER} of ${AWS_RDS_DEFAULT_RETRY_COUNT})"
        $(which aws) ${AWS_REGION_STRING} rds reboot-db-instance --db-instance-identifier "${TMP_DATABASE_ID}" ${ARG_FAILOVER} >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
        RETURNVAL="$?"
        # Fix Response File
        $(which sed) -i "s/\o15/_AWS_BAD_IGNORE\\n/g" "${AWS_FILE_RESPONSE}"
        $(which sed) -i '/_AWS_BAD_IGNORE/d' "${AWS_FILE_RESPONSE}"
        if [ ${RETURNVAL} -ne 0 ]; then
            if grep -q "DBInstanceNotFound" "${AWS_FILE_ERROR}"; then
                log_error "- Database not found, cannot reboot"
                return $E_OBJECT_NOT_FOUND
            fi
            log_add_from_file "${AWS_FILE_ERROR}" "${FUNCTION_DESCRIPTION}: Data containing error"
            log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Data"
            log_error "${FUNCTION_DESCRIPTION}: operation failed (aws_cli_exit_code::${RETURNVAL}])"
            RETRY_ENABLED=yes
            # Reset Files
            > ${AWS_FILE_RESPONSE}
            > ${AWS_FILE_ERROR}
        else
            log "${FUNCTION_DESCRIPTION}: Successfully sent reboot request"
            call_sleep 60 "giving RDS time to process request"
            rds_poll_status "${TMP_DATABASE_ID}" yes "${AWS_OPERATION_TIMEOUT}" "${AWS_RDS_VERIFICATION_SLEEP}" "${TMP_AWS_REGION}"
            RETURNVAL="$?"
            if [ ${RETURNVAL} -ne 0 ]; then
                log_error "${FUNCTION_DESCRIPTION}: Database failed to reboot in a timely fashion"
                return $RETURNVAL
            fi
            log_success "${FUNCTION_DESCRIPTION}: Successfully rebooted database [${TMP_DATABASE_ID}]"
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: rds_set_backup_policy()
## - Sets Backup Policy for specified Database
## - Arguments
##   - $1: Database ID
##   - $2: Backup Retention Period
##   - $3: Preferred Backup Window
##   - $4: Apply Immediately (yes/no, defaults to yes)
##   - $5: Region
function rds_set_backup_policy() {
    local FUNCTION_DESCRIPTION="RDS (Set Backup Policy)"
    local TMP_DATABASE_ID="${1}"
    local TMP_BACKUP_RETENTION_PERIOD="${2}"
    local TMP_PREFERRED_BACKUP_WINDOW="${3}"
    local TMP_APPLY_IMMEDIATELY="${4}"
    local TMP_AWS_REGION="${5}"

    local RETURNVAL=""
    local TMP_STRING=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local ARG_APPLY_IMMEDIATELY=""
    local ARG_PREFERRED_BACKUP_WINDOW=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    if(is_empty "${TMP_DATABASE_ID}"); then
        log_error "${FUNCTION_DESCRIPTION}: Database ID not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_BACKUP_RETENTION_PERIOD}" no); then
        log_error "${FUNCTION_DESCRIPTION}: Backup Retention Period not specified"
        return $E_BAD_ARGS
    fi

    if [ ${TMP_BACKUP_RETENTION_PERIOD} -gt 0 ]; then
        if(is_empty "${TMP_PREFERRED_BACKUP_WINDOW}"); then
            log_error "${FUNCTION_DESCRIPTION}: Preferred Backup Window not specified (this is a requirement when Backup Retention is greater than Zero)"
            return $E_BAD_ARGS
        fi
    fi

    if(is_empty "${TMP_APPLY_IMMEDIATELY}"); then
        TMP_APPLY_IMMEDIATELY=yes
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    if(! is_empty "${TMP_PREFERRED_BACKUP_WINDOW}"); then
        ARG_PREFERRED_BACKUP_WINDOW="--preferred-backup-window ${TMP_PREFERRED_BACKUP_WINDOW}"
    fi

    if option_enabled TMP_APPLY_IMMEDIATELY; then
        ARG_APPLY_IMMEDIATELY="--apply-immediately"
    else
        ARG_APPLY_IMMEDIATELY="--no-apply-immediately"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_RDS_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_RDS_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_RDS_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        log "${FUNCTION_DESCRIPTION}: Setting backup policy (database_id: [${TMP_DATABASE_ID}] / backup_retention_period: [${TMP_BACKUP_RETENTION_PERIOD}] / preferred_backup_window: [${TMP_PREFERRED_BACKUP_WINDOW}] / apply_immediately: [${TMP_APPLY_IMMEDIATELY}] / region: [${TMP_AWS_REGION}]) (Attempt::${COUNTER} of ${AWS_RDS_DEFAULT_RETRY_COUNT})"
        $(which aws) ${AWS_REGION_STRING} rds modify-db-instance --db-instance-identifier "${TMP_DATABASE_ID}" --backup-retention-period "${TMP_BACKUP_RETENTION_PERIOD}" ${ARG_PREFERRED_BACKUP_WINDOW} ${ARG_APPLY_IMMEDIATELY} --copy-tags-to-snapshot >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
        RETURNVAL="$?"
        # Fix Response File
        $(which sed) -i "s/\o15/_AWS_BAD_IGNORE\\n/g" "${AWS_FILE_RESPONSE}"
        $(which sed) -i '/_AWS_BAD_IGNORE/d' "${AWS_FILE_RESPONSE}"
        if [ ${RETURNVAL} -ne 0 ]; then
            if grep -q "DBInstanceNotFound" "${AWS_FILE_ERROR}"; then
                log_error "- Database Instance not found, cannot proceed"
                return $E_OBJECT_NOT_FOUND
            fi
            log_add_from_file "${AWS_FILE_ERROR}" "${FUNCTION_DESCRIPTION}: Data containing error"
            log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Data"
            log_error "${FUNCTION_DESCRIPTION}: operation failed (aws_cli_exit_code::${RETURNVAL}])"
            RETRY_ENABLED=yes
            # Reset Files
            > ${AWS_FILE_RESPONSE}
            > ${AWS_FILE_ERROR}
        else
            log "${FUNCTION_DESCRIPTION}: Successfully sent update request"
            call_sleep 30 "giving RDS time to process request"
            rds_poll_status "${TMP_DATABASE_ID}" yes 30 "" "${TMP_AWS_REGION}"
            RETURNVAL="$?"
            if [ ${RETURNVAL} -ne 0 ]; then
                return $RETURNVAL
            fi
            log "${FUNCTION_DESCRIPTION}: Successfully updated backup policy"
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: rds_set_engine_version()
## - Sets Engine Version for specified Database
## - Arguments
##   - $1: Database ID
##   - $2: Engine Version
##   - $3: Parameter Group (optional, only required if changing the Major version of the Engine)
##   - $4: Apply Immediately (yes/no, defaults to yes)
##   - $5: Region
function rds_set_engine_version() {
    local FUNCTION_DESCRIPTION="RDS (Set Engine Version)"
    local TMP_DATABASE_ID="${1}"
    local TMP_ENGINE_VERSION="${2}"
    local TMP_PARAMETER_GROUP="${3}"
    local TMP_APPLY_IMMEDIATELY="${4}"
    local TMP_AWS_REGION="${5}"

    local RETURNVAL=""
    local TMP_STRING=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local ARG_APPLY_IMMEDIATELY=""
    local ARG_PARAMETER_GROUP=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    if(is_empty "${TMP_DATABASE_ID}"); then
        log_error "${FUNCTION_DESCRIPTION}: Database ID not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_ENGINE_VERSION}"); then
        log_error "${FUNCTION_DESCRIPTION}: Engine Version not specified"
        return $E_BAD_ARGS
    fi

    if(is_empty "${TMP_APPLY_IMMEDIATELY}"); then
        TMP_APPLY_IMMEDIATELY=yes
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    if(! is_empty "${TMP_PARAMETER_GROUP}"); then
        ARG_PARAMETER_GROUP="--db-parameter-group-name ${TMP_PARAMETER_GROUP}"
    fi

    if option_enabled TMP_APPLY_IMMEDIATELY; then
        ARG_APPLY_IMMEDIATELY="--apply-immediately"
    else
        ARG_APPLY_IMMEDIATELY="--no-apply-immediately"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_RDS_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_RDS_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_RDS_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        log "${FUNCTION_DESCRIPTION}: Setting engine version (database_id: [${TMP_DATABASE_ID}] / engine_version: [${TMP_ENGINE_VERSION}] / parameter_group: [${TMP_PARAMETER_GROUP}] / apply_immediately: [${TMP_APPLY_IMMEDIATELY}] / region: [${TMP_AWS_REGION}]) (Attempt::${COUNTER} of ${AWS_RDS_DEFAULT_RETRY_COUNT})"
        $(which aws) ${AWS_REGION_STRING} rds modify-db-instance --db-instance-identifier "${TMP_DATABASE_ID}" --engine-version "${TMP_ENGINE_VERSION}" ${ARG_PARAMETER_GROUP} ${ARG_APPLY_IMMEDIATELY} >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
        RETURNVAL="$?"
        # Fix Response File
        $(which sed) -i "s/\o15/_AWS_BAD_IGNORE\\n/g" "${AWS_FILE_RESPONSE}"
        $(which sed) -i '/_AWS_BAD_IGNORE/d' "${AWS_FILE_RESPONSE}"
        if [ ${RETURNVAL} -ne 0 ]; then
            if grep -q "DBInstanceNotFound" "${AWS_FILE_ERROR}"; then
                log_error "- Database Instance not found, cannot proceed"
                return $E_OBJECT_NOT_FOUND
            fi
            log_add_from_file "${AWS_FILE_ERROR}" "${FUNCTION_DESCRIPTION}: Data containing error"
            log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Data"
            log_error "${FUNCTION_DESCRIPTION}: operation failed (aws_cli_exit_code::${RETURNVAL}])"
            RETRY_ENABLED=yes
            # Reset Files
            > ${AWS_FILE_RESPONSE}
            > ${AWS_FILE_ERROR}
        else
            log "${FUNCTION_DESCRIPTION}: Successfully sent update request"
            call_sleep 30 "giving RDS time to process request"
            rds_poll_status "${TMP_DATABASE_ID}" yes 30 "" "${TMP_AWS_REGION}"
            RETURNVAL="$?"
            if [ ${RETURNVAL} -ne 0 ]; then
                return $RETURNVAL
            fi
            log "${FUNCTION_DESCRIPTION}: Successfully updated engine version"
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: rds_set_maintenance_policy()
## - Sets Maintenance Policy for specified Database
## - Arguments
##   - $1: Database ID
##   - $2: Preferred Maintenance Window
##   - $3: Auto Minor Version Upgrade (yes/no, defaults to no)
##   - $4: Allow Major Version Upgrade (yes/no, defaults to no)
##   - $5: Apply Immediately (yes/no, defaults to yes)
##   - $6: Region
function rds_set_maintenance_policy() {
    local FUNCTION_DESCRIPTION="RDS (Set Maintenance Policy)"
    local TMP_DATABASE_ID="${1}"
    local TMP_PREFERRED_MAINTENANCE_WINDOW="${2}"
    local TMP_AUTO_MINOR_VERSION_UPGRADE="${3}"
    local TMP_ALLOW_MAJOR_VERSION_UPGRADE="${4}"
    local TMP_APPLY_IMMEDIATELY="${5}"
    local TMP_AWS_REGION="${6}"

    local RETURNVAL=""
    local TMP_STRING=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local ARG_APPLY_IMMEDIATELY=""
    local ARG_PREFERRED_MAINTENANCE_WINDOW=""
    local ARG_AUTO_MINOR_VERSION_UPGRADE=""
    local ARG_ALLOW_MAJOR_VERSION_UPGRADE=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    if(is_empty "${TMP_DATABASE_ID}"); then
        log_error "${FUNCTION_DESCRIPTION}: Database ID not specified"
        return $E_BAD_ARGS
    fi

    if(is_empty "${TMP_AUTO_MINOR_VERSION_UPGRADE}"); then
        TMP_AUTO_MINOR_VERSION_UPGRADE=no
    fi
    if(is_empty "${TMP_ALLOW_MAJOR_VERSION_UPGRADE}"); then
        TMP_ALLOW_MAJOR_VERSION_UPGRADE=no
    fi
    if(is_empty "${TMP_APPLY_IMMEDIATELY}"); then
        TMP_APPLY_IMMEDIATELY=yes
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    if(! is_empty "${TMP_PREFERRED_MAINTENANCE_WINDOW}"); then
        ARG_PREFERRED_MAINTENANCE_WINDOW="--preferred-maintenance-window ${TMP_PREFERRED_MAINTENANCE_WINDOW}"
    fi
    if option_enabled TMP_AUTO_MINOR_VERSION_UPGRADE; then
        ARG_AUTO_MINOR_VERSION_UPGRADE="--auto-minor-version-upgrade"
    else
        ARG_AUTO_MINOR_VERSION_UPGRADE="--no-auto-minor-version-upgrade"
    fi
    if option_enabled TMP_ALLOW_MAJOR_VERSION_UPGRADE; then
        ARG_ALLOW_MAJOR_VERSION_UPGRADE="--allow-major-version-upgrade"
    else
        ARG_ALLOW_MAJOR_VERSION_UPGRADE="--no-allow-major-version-upgrade"
    fi
    if option_enabled TMP_APPLY_IMMEDIATELY; then
        ARG_APPLY_IMMEDIATELY="--apply-immediately"
    else
        ARG_APPLY_IMMEDIATELY="--no-apply-immediately"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_RDS_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_RDS_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_RDS_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        log "${FUNCTION_DESCRIPTION}: Setting maintenance policy (database_id: [${TMP_DATABASE_ID}] / preferred_maintenance_window: [${TMP_PREFERRED_MAINTENANCE_WINDOW}] / auto_minor_version_upgrade: [${TMP_AUTO_MINOR_VERSION_UPGRADE}] / allow_major_version_upgrade: [${TMP_ALLOW_MAJOR_VERSION_UPGRADE}] / apply_immediately: [${TMP_APPLY_IMMEDIATELY}] / region: [${TMP_AWS_REGION}]) (Attempt::${COUNTER} of ${AWS_RDS_DEFAULT_RETRY_COUNT})"
        $(which aws) ${AWS_REGION_STRING} rds modify-db-instance --db-instance-identifier "${TMP_DATABASE_ID}" ${ARG_PREFERRED_MAINTENANCE_WINDOW} ${ARG_AUTO_MINOR_VERSION_UPGRADE} ${ARG_ALLOW_MAJOR_VERSION_UPGRADE} ${ARG_APPLY_IMMEDIATELY} >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
        RETURNVAL="$?"
        # Fix Response File
        $(which sed) -i "s/\o15/_AWS_BAD_IGNORE\\n/g" "${AWS_FILE_RESPONSE}"
        $(which sed) -i '/_AWS_BAD_IGNORE/d' "${AWS_FILE_RESPONSE}"
        if [ ${RETURNVAL} -ne 0 ]; then
            if grep -q "DBInstanceNotFound" "${AWS_FILE_ERROR}"; then
                log_error "- Database Instance not found, cannot proceed"
                return $E_OBJECT_NOT_FOUND
            fi
            log_add_from_file "${AWS_FILE_ERROR}" "${FUNCTION_DESCRIPTION}: Data containing error"
            log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Data"
            log_error "${FUNCTION_DESCRIPTION}: operation failed (aws_cli_exit_code::${RETURNVAL}])"
            RETRY_ENABLED=yes
            # Reset Files
            > ${AWS_FILE_RESPONSE}
            > ${AWS_FILE_ERROR}
        else
            log "${FUNCTION_DESCRIPTION}: Successfully sent update request"
            call_sleep 30 "giving RDS time to process request"
            rds_poll_status "${TMP_DATABASE_ID}" yes 30 "" "${TMP_AWS_REGION}"
            RETURNVAL="$?"
            if [ ${RETURNVAL} -ne 0 ]; then
                return $RETURNVAL
            fi
            log "${FUNCTION_DESCRIPTION}: Successfully updated maintenance policy"
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: rds_set_parameter_group()
## - Sets Parameter Group for specified Database
## - Arguments
##   - $1: Database ID
##   - $2: Parameter Group
##   - $3: Apply Immediately (yes/no, defaults to yes)
##   - $4: Region
function rds_set_parameter_group() {
    local FUNCTION_DESCRIPTION="RDS (Set Parameter Group)"
    local TMP_DATABASE_ID="${1}"
    local TMP_PARAMETER_GROUP="${2}"
    local TMP_APPLY_IMMEDIATELY="${3}"
    local TMP_AWS_REGION="${4}"

    local RETURNVAL=""
    local TMP_STRING=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local ARG_APPLY_IMMEDIATELY=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    if(is_empty "${TMP_DATABASE_ID}"); then
        log_error "${FUNCTION_DESCRIPTION}: Database ID not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_PARAMETER_GROUP}"); then
        log_error "${FUNCTION_DESCRIPTION}: Parameter Group not specified"
        return $E_BAD_ARGS
    fi

    if(is_empty "${TMP_APPLY_IMMEDIATELY}"); then
        TMP_APPLY_IMMEDIATELY=yes
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    if option_enabled TMP_APPLY_IMMEDIATELY; then
        ARG_APPLY_IMMEDIATELY="--apply-immediately"
    else
        ARG_APPLY_IMMEDIATELY="--no-apply-immediately"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_RDS_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_RDS_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_RDS_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        log "${FUNCTION_DESCRIPTION}: Setting paramater group (database_id: [${TMP_DATABASE_ID}] / parameter_group: [${TMP_PARAMETER_GROUP}] / apply_immediately: [${TMP_APPLY_IMMEDIATELY}] / region: [${TMP_AWS_REGION}]) (Attempt::${COUNTER} of ${AWS_RDS_DEFAULT_RETRY_COUNT})"
        $(which aws) ${AWS_REGION_STRING} rds modify-db-instance --db-instance-identifier "${TMP_DATABASE_ID}" --db-parameter-group-name "${TMP_PARAMETER_GROUP}" ${ARG_APPLY_IMMEDIATELY} >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
        RETURNVAL="$?"
        # Fix Response File
        $(which sed) -i "s/\o15/_AWS_BAD_IGNORE\\n/g" "${AWS_FILE_RESPONSE}"
        $(which sed) -i '/_AWS_BAD_IGNORE/d' "${AWS_FILE_RESPONSE}"
        if [ ${RETURNVAL} -ne 0 ]; then
            if grep -q "DBInstanceNotFound" "${AWS_FILE_ERROR}"; then
                log_error "- Database Instance not found, cannot proceed"
                return $E_OBJECT_NOT_FOUND
            fi
            log_add_from_file "${AWS_FILE_ERROR}" "${FUNCTION_DESCRIPTION}: Data containing error"
            log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Data"
            log_error "${FUNCTION_DESCRIPTION}: operation failed (aws_cli_exit_code::${RETURNVAL}])"
            RETRY_ENABLED=yes
            # Reset Files
            > ${AWS_FILE_RESPONSE}
            > ${AWS_FILE_ERROR}
        else
            log "${FUNCTION_DESCRIPTION}: Successfully sent update request"
            call_sleep 60 "giving RDS time to process request"
            rds_poll_status "${TMP_DATABASE_ID}" yes 30 "" "${TMP_AWS_REGION}"
            RETURNVAL="$?"
            if [ ${RETURNVAL} -ne 0 ]; then
                return $RETURNVAL
            fi
            log "${FUNCTION_DESCRIPTION}: Successfully updated parameter group"
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: rds_set_security_group()
## - Sets Security Group for specified Database
## - Arguments
##   - $1: Database ID
##   - $2: Security Group
##   - $3: Apply Immediately (yes/no, defaults to yes)
##   - $4: Region
function rds_set_security_group() {
    local FUNCTION_DESCRIPTION="RDS (Set Security Group)"
    local TMP_DATABASE_ID="${1}"
    local TMP_SECURITY_GROUP="${2}"
    local TMP_APPLY_IMMEDIATELY="${3}"
    local TMP_AWS_REGION="${4}"

    local RETURNVAL=""
    local TMP_STRING=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local ARG_APPLY_IMMEDIATELY=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    if(is_empty "${TMP_DATABASE_ID}"); then
        log_error "${FUNCTION_DESCRIPTION}: Database ID not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_SECURITY_GROUP}"); then
        log_error "${FUNCTION_DESCRIPTION}: Security Group not specified"
        return $E_BAD_ARGS
    fi

    if(is_empty "${TMP_APPLY_IMMEDIATELY}"); then
        TMP_APPLY_IMMEDIATELY=yes
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    if option_enabled TMP_APPLY_IMMEDIATELY; then
        ARG_APPLY_IMMEDIATELY="--apply-immediately"
    else
        ARG_APPLY_IMMEDIATELY="--no-apply-immediately"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_RDS_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_RDS_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_RDS_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        log "${FUNCTION_DESCRIPTION}: Setting security group (database_id: [${TMP_DATABASE_ID}] / security_group: [${TMP_SECURITY_GROUP}] / apply_immediately: [${TMP_APPLY_IMMEDIATELY}] / region: [${TMP_AWS_REGION}]) (Attempt::${COUNTER} of ${AWS_RDS_DEFAULT_RETRY_COUNT})"
        $(which aws) ${AWS_REGION_STRING} rds modify-db-instance --db-instance-identifier "${TMP_DATABASE_ID}" --vpc-security-group-ids "${TMP_SECURITY_GROUP}" ${ARG_APPLY_IMMEDIATELY} >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
        RETURNVAL="$?"
        # Fix Response File
        $(which sed) -i "s/\o15/_AWS_BAD_IGNORE\\n/g" "${AWS_FILE_RESPONSE}"
        $(which sed) -i '/_AWS_BAD_IGNORE/d' "${AWS_FILE_RESPONSE}"
        if [ ${RETURNVAL} -ne 0 ]; then
            if grep -q "DBInstanceNotFound" "${AWS_FILE_ERROR}"; then
                log_error "- Database Instance not found, cannot proceed"
                return $E_OBJECT_NOT_FOUND
            fi
            log_add_from_file "${AWS_FILE_ERROR}" "${FUNCTION_DESCRIPTION}: Data containing error"
            log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Data"
            log_error "${FUNCTION_DESCRIPTION}: operation failed (aws_cli_exit_code::${RETURNVAL}])"
            RETRY_ENABLED=yes
            # Reset Files
            > ${AWS_FILE_RESPONSE}
            > ${AWS_FILE_ERROR}
        else
            log "${FUNCTION_DESCRIPTION}: Successfully sent update request"
            call_sleep 30 "giving RDS time to process request"
            rds_poll_status "${TMP_DATABASE_ID}" yes 30 "" "${TMP_AWS_REGION}"
            RETURNVAL="$?"
            if [ ${RETURNVAL} -ne 0 ]; then
                return $RETURNVAL
            fi
            log "${FUNCTION_DESCRIPTION}: Successfully updated security group"
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: rds_update_password()
## - Updates Master Password for database
## - Arguments
##   - $1: Database ID
##   - $2: New Password
##   - $3: Verification Timeout <in minutes, defaults to 30 minutes>
##   - $4: Region
function rds_update_password() {
    local FUNCTION_DESCRIPTION="RDS (Update Password)"
    local TMP_DATABASE_ID="${1}"
    local TMP_PASSWORD="${2}"
    local AWS_OPERATION_TIMEOUT="${3}"
    local TMP_AWS_REGION="${4}"

    local RETURNVAL=""
    local TMP_STRING=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    if(is_empty "${TMP_DATABASE_ID}"); then
        log_error "${FUNCTION_DESCRIPTION}: Database ID not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_PASSWORD}"); then
        log_error "${FUNCTION_DESCRIPTION}: Password not specified"
        return $E_BAD_ARGS
    fi

    if(is_empty "${AWS_OPERATION_TIMEOUT}"); then
        AWS_OPERATION_TIMEOUT=${AWS_RDS_DEFAULT_VERIFICATION_TIMEOUT}
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_RDS_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_RDS_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_RDS_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        log "${FUNCTION_DESCRIPTION}: Updating Password (database_id: [${TMP_DATABASE_ID}] / new_password: [${TMP_PASSWORD//?/*}] / region: [${TMP_AWS_REGION}]) (Attempt::${COUNTER} of ${AWS_RDS_DEFAULT_RETRY_COUNT})"
        $(which aws) ${AWS_REGION_STRING} rds modify-db-instance --db-instance-identifier "${TMP_DATABASE_ID}" --master-user-password "${TMP_PASSWORD}" --apply-immediately >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
        RETURNVAL="$?"
        # Fix Response File
        $(which sed) -i "s/\o15/_AWS_BAD_IGNORE\\n/g" "${AWS_FILE_RESPONSE}"
        $(which sed) -i '/_AWS_BAD_IGNORE/d' "${AWS_FILE_RESPONSE}"
        if [ ${RETURNVAL} -ne 0 ]; then
            if grep -q "DBInstanceNotFound" "${AWS_FILE_ERROR}"; then
                log_error "- Database Instance not found, cannot update password"
                return $E_OBJECT_NOT_FOUND
            fi
            log_add_from_file "${AWS_FILE_ERROR}" "${FUNCTION_DESCRIPTION}: Data containing error"
            log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Data"
            log_error "${FUNCTION_DESCRIPTION}: operation failed (aws_cli_exit_code::${RETURNVAL}])"
            RETRY_ENABLED=yes
            # Reset Files
            > ${AWS_FILE_RESPONSE}
            > ${AWS_FILE_ERROR}
        else
            log_notice "${FUNCTION_DESCRIPTION}: Successfully sent update password request"
            call_sleep 60
            rds_poll_status "${TMP_DATABASE_ID}" yes "${AWS_OPERATION_TIMEOUT}" "" "${TMP_AWS_REGION}"
            RETURNVAL="$?"
            if [ ${RETURNVAL} -ne 0 ]; then
                return $RETURNVAL
            fi
            log "${FUNCTION_DESCRIPTION}: successfully updated password"
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}
