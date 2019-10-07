###------------------------------------------------------------------------------------------------
# script:  elasticache_redis.inc
# purpose: Collection of functions related to AWS (ElastiCache - Redis)
# version: 1.0.0
#
# function list:
#          - load_info_redis
#          - redis_create
#          - redis_delete
#          - redis_get_current_replication_group_id
#          - redis_get_endpoint
#          - redis_get_info
#          - redis_get_latest_snapshot_name
#          - redis_get_replication_group_endpoint_read
#          - redis_get_replication_group_endpoint_write
#          - redis_get_status
#          - redis_poll_status
#          - redis_set_dns
###------------------------------------------------------------------------------------------------
REQUIRED_EXECUTABLES+=('aws' 'redis-cli')

###------------------------------------------------------------------------------------------------
# Variables
AWS_EC_DEFAULT_RETRY_COUNT=3
AWS_EC_DEFAULT_RETRY_TIMER_MAX_SEC=30
AWS_EC_DEFAULT_OPERATION_TIMEOUT=30
AWS_EC_DEFAULT_VERIFICATION_TIMEOUT=60
AWS_EC_DEFAULT_VERIFICATION_SLEEP=30
AWS_EC_DNS_TTL=30

ELASTICACHE_DEFAULT_PORT="6379"

STACK_VARIABLES_REQUIRED+=(
    'ReferenceName'
    'CacheNodeType'
    'EngineVersion'
    'ReplicaCount'
    'EncryptionEnabled'
    'ParameterGroupName'
    'SubnetType'
    'ElasticacheSubnetGroupId'
    'ElasticacheSecurityGroupId'
    'Port'
    'DNSElasticacheDomain'
    'DNSElasticacheHostedZoneID'
    'DNSElasticacheRecordRead'
    'DNSElasticacheRecordWrite'
    'AutoMinorVersionUpgrade'
    'PreferredMaintenanceWindow'
    'SnapshotRetentionLimit'
    'SnapshotWindow'
    'AccountName'
    'AccountAbbr'
    'AccountNumber'
    'OrganizationName'
    'OrganizationAbbr'
    'Environment'
    'ProjectName'
    'ProjectAbbr'
    'FunctionName'
    'FunctionAbbr'
    'Owner'
    'Contact'
    'Region'
)

ELASTICACHE_DETAILS_VARIABLES+=(
    'ReferenceName'
    'ReferenceId'
    'CacheNodeType'
    'Description'
    'MultiAZEnabled'
    'Engine'
    'EngineVersion'
    'ParameterGroupName'
    'EncryptionEnabled'
)

ELASTICACHE_NETWORK_VARIABLES+=(
    'Port'
    'SubnetType'
    'ElasticacheSubnetGroupId'
    'ElasticacheSecurityGroupId'
)

ELASTICACHE_SNAPSHOT_VARIABLES+=(
    'SnapshotRetentionLimit'
    'SnapshotWindow'
)

ELASTICACHE_MAINTENANCE_VARIABLES+=(
    'AutoMinorVersionUpgrade'
    'PreferredMaintenanceWindow'
)

ELASTICACHE_DNS_VARIABLES+=(
    'DNSElasticacheHostedZoneID'
    'DNSElasticacheRecordWrite'
    'DNSElasticacheRecordRead'
)

ELASTICACHE_ENDPOINT_VARIABLES+=(
    'Endpoint_Read'
    'Endpoint_Write'
)

ELASTICACHE_INFO_VARIABLES+=(
    'AccountName'
    'AccountAbbr'
    'AccountNumber'
    'OrganizationName'
    'OrganizationAbbr'
    'Environment'
    'ProjectName'
    'ProjectAbbr'
    'FunctionName'
    'FunctionAbbr'
    'Owner'
    'Contact'
    'Region'
)

ELASTICACHE_ENGINE="redis"

###------------------------------------------------------------------------------------------------
## FUNCTION: load_info_redis()
## - Loads Redis Information into memory from CloudFormation Outputs file
## - Arguments
##   - $1: CloudFormation Stack Name
##   - $2: Region
function load_info_redis() {
    local FUNCTION_DESCRIPTION="Load Info (Redis)"
    local TMP_CLOUDFORMATION_STACK_NAME="${1}"
    local TMP_AWS_REGION="${2}"

    local TMP_KEY=""
    local TMP_VAR=""
    local KEY_MAX_LENGTH=0

    local TMP_FILE_CLOUDFORMATION_OUTPUT=""

    if(is_empty "${TMP_CLOUDFORMATION_STACK_NAME}"); then
        log_error "${FUNCTION_DESCRIPTION}: CloudFormation Stack Name not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_AWS_REGION}"); then
        log_error "${FUNCTION_DESCRIPTION}: AWS Region not specified"
        return $E_BAD_ARGS
    fi

    generate_temp_file TMP_FILE_CLOUDFORMATION_OUTPUT "CloudFormation Stack Output"

    log_notice "${FUNCTION_DESCRIPTION}: loading CloudFormation Stack"
    cloudformation_get_outputs_silent "${TMP_FILE_CLOUDFORMATION_OUTPUT}" "${TMP_CLOUDFORMATION_STACK_NAME}" "${TMP_AWS_REGION}"
    RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then return ${RETURNVAL}; fi

    load_array_properties_from_file "STACK_VARIABLES_REQUIRED[@]" "${TMP_FILE_CLOUDFORMATION_OUTPUT}" "ELASTICACHE"
    verify_array_key_values "STACK_VARIABLES_REQUIRED[@]" "ELASTICACHE"
    RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then return $RETURNVAL; fi

    if(! is_int "${ELASTICACHE_REPLICACOUNT}"); then
        log_error "${FUNCTION_DESCRIPTION}: Replica Count is not an integer [${ELASTICACHE_REPLICACOUNT}]"
        return $E_BAD_ARGS
    fi

    ELASTICACHE_REFERENCEID="${ELASTICACHE_REFERENCENAME}-$(generate_uuid "" 2)"
    ELASTICACHE_DESCRIPTION="${ELASTICACHE_PROJECTNAME} - ${ELASTICACHE_FUNCTIONNAME} (${ELASTICACHE_ENVIRONMENT}) - [${ELASTICACHE_ORGANIZATIONNAME} / ${ELASTICACHE_REGION}]"

    if [ $ELASTICACHE_REPLICACOUNT -gt 0 ]; then
        ELASTICACHE_MULTIAZENABLED=yes
    else
        ELASTICACHE_MULTIAZENABLED=no
    fi


    for TMP_KEY in "${ELASTICACHE_DETAILS_VARIABLES[@]}"; do
        if [ ${#TMP_KEY} -gt $KEY_MAX_LENGTH ]; then KEY_MAX_LENGTH=${#TMP_KEY}; fi
    done
    for TMP_KEY in "${ELASTICACHE_NETWORK_VARIABLES[@]}"; do
        if [ ${#TMP_KEY} -gt $KEY_MAX_LENGTH ]; then KEY_MAX_LENGTH=${#TMP_KEY}; fi
    done
    for TMP_KEY in "${ELASTICACHE_SNAPSHOT_VARIABLES[@]}"; do
        if [ ${#TMP_KEY} -gt $KEY_MAX_LENGTH ]; then KEY_MAX_LENGTH=${#TMP_KEY}; fi
    done
    for TMP_KEY in "${ELASTICACHE_MAINTENANCE_VARIABLES[@]}"; do
        if [ ${#TMP_KEY} -gt $KEY_MAX_LENGTH ]; then KEY_MAX_LENGTH=${#TMP_KEY}; fi
    done
    for TMP_KEY in "${ELASTICACHE_DNS_VARIABLES[@]}"; do
        if [ ${#TMP_KEY} -gt $KEY_MAX_LENGTH ]; then KEY_MAX_LENGTH=${#TMP_KEY}; fi
    done
    for TMP_KEY in "${ELASTICACHE_INFO_VARIABLES[@]}"; do
        if [ ${#TMP_KEY} -gt $KEY_MAX_LENGTH ]; then KEY_MAX_LENGTH=${#TMP_KEY}; fi
    done
    KEY_MAX_LENGTH=$((${KEY_MAX_LENGTH}+1))

    line_break
    log_highlight "Elasticache - Details"
    for TMP_KEY in "${ELASTICACHE_DETAILS_VARIABLES[@]}"; do
        TMP_VAR="$(to_upper "ELASTICACHE_${TMP_KEY}")"
        log "$(printf "%-1s %-${KEY_MAX_LENGTH}s %s" "-" "${TMP_KEY}:" "[${!TMP_VAR}]")"
    done

    line_break
    log_highlight "Elasticache - Info"
    for TMP_KEY in "${ELASTICACHE_INFO_VARIABLES[@]}"; do
        TMP_VAR="$(to_upper "ELASTICACHE_${TMP_KEY}")"
        log "$(printf "%-1s %-${KEY_MAX_LENGTH}s %s" "-" "${TMP_KEY}:" "[${!TMP_VAR}]")"
    done

    line_break
    log_highlight "Elasticache - Network"
    for TMP_KEY in "${ELASTICACHE_NETWORK_VARIABLES[@]}"; do
        TMP_VAR="$(to_upper "ELASTICACHE_${TMP_KEY}")"
        log "$(printf "%-1s %-${KEY_MAX_LENGTH}s %s" "-" "${TMP_KEY}:" "[${!TMP_VAR}]")"
    done

    line_break
    log_highlight "Elasticache - Snapshot"
    for TMP_KEY in "${ELASTICACHE_SNAPSHOT_VARIABLES[@]}"; do
        TMP_VAR="$(to_upper "ELASTICACHE_${TMP_KEY}")"
        log "$(printf "%-1s %-${KEY_MAX_LENGTH}s %s" "-" "${TMP_KEY}:" "[${!TMP_VAR}]")"
    done

    line_break
    log_highlight "Elasticache - Maintenance"
    for TMP_KEY in "${ELASTICACHE_MAINTENANCE_VARIABLES[@]}"; do
        TMP_VAR="$(to_upper "ELASTICACHE_${TMP_KEY}")"
        log "$(printf "%-1s %-${KEY_MAX_LENGTH}s %s" "-" "${TMP_KEY}:" "[${!TMP_VAR}]")"
    done

    line_break
    log_highlight "Elasticache - DNS"
    for TMP_KEY in "${ELASTICACHE_DNS_VARIABLES[@]}"; do
        TMP_VAR="$(to_upper "ELASTICACHE_${TMP_KEY}")"
        log "$(printf "%-1s %-${KEY_MAX_LENGTH}s %s" "-" "${TMP_KEY}:" "[${!TMP_VAR}]")"
    done

    return 0
}

###------------------------------------------------------------------------------------------------
## FUNCTION: redis_create()
## - Creates Redis
## - Arguments
##   - $1: Snapshot Name (optional, if not supplied, an empty Redis will be created)
##   - $2: Region
function redis_create() {
    local FUNCTION_DESCRIPTION="Redis (Create)"
    local TMP_SNAPSHOT_NAME="${1}"
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

    verify_array_key_values "ELASTICACHE_VARIABLES[@]" "ELASTICACHE"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "${FUNCTION_DESCRIPTION}: Failed to load required variables [Redis]"
        return $RETURNVAL
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    local ARG_REPLICATION_GROUP_ID="--replication-group-id ${ELASTICACHE_REFERENCEID}"
    local ARG_CACHE_NODE_TYPE="--cache-node-type ${ELASTICACHE_CACHENODETYPE}"
    local ARG_ENGINE="--engine ${ELASTICACHE_ENGINE}"
    local ARG_ENGINE_VERSION="--engine-version ${ELASTICACHE_ENGINEVERSION}"
    local ARG_REPLICAS_PER_NODE_GROUP="--replicas-per-node-group ${ELASTICACHE_REPLICACOUNT}"
    local ARG_CACHE_PARAMETER_GROUP_NAME="--cache-parameter-group-name ${ELASTICACHE_PARAMETERGROUPNAME}"
    local ARG_CACHE_SUBNET_GROUP_NAME="--cache-subnet-group-name ${ELASTICACHE_ELASTICACHESUBNETGROUPID}"
    local ARG_SECURITY_GROUP_IDS="--security-group-ids ${ELASTICACHE_ELASTICACHESECURITYGROUPID}"
    local ARG_PORT="--port ${ELASTICACHE_PORT}"
    local ARG_SNAPSHOT_RETENTION_LIMIT="--snapshot-retention-limit ${ELASTICACHE_SNAPSHOTRETENTIONLIMIT}"
    if [ ${ELASTICACHE_SNAPSHOTRETENTIONLIMIT} -gt 0 ]; then
        local ARG_SNAPSHOT_WINDOW="--snapshot-window ${ELASTICACHE_SNAPSHOTWINDOW}"
    else
        local ARG_SNAPSHOT_WINDOW=""
    fi
    if [[ ${ELASTICACHE_CACHENODETYPE} = "cache.t2."* ]]; then
        ARG_SNAPSHOT_RETENTION_LIMIT=""
        ARG_SNAPSHOT_WINDOW=""
    fi
    local ARG_PREFERRED_MAINTENANCE_WINDOW="--preferred-maintenance-window ${ELASTICACHE_PREFERREDMAINTENANCEWINDOW}"
    if option_enabled ELASTICACHE_AUTOMINORVERSIONUPGRADE; then
        local ARG_MINOR_VERSION_UPGRADE="--auto-minor-version-upgrade"
    else
        local ARG_MINOR_VERSION_UPGRADE="--no-auto-minor-version-upgrade"
    fi
    if(! is_empty "${TMP_SNAPSHOT_NAME}"); then
        local ARG_SNAPSHOT_NAME="--snapshot-name ${TMP_SNAPSHOT_NAME}"
    else
        local ARG_SNAPSHOT_NAME=""
    fi
    if option_enabled ELASTICACHE_MULTIAZENABLED; then
        local ARG_AUTOMATIC_FAILOVER="--automatic-failover-enabled"
    else
        local ARG_AUTOMATIC_FAILOVER="--no-automatic-failover-enabled"
    fi
    if option_enabled ELASTICACHE_ENCRYPTIONENABLED; then
        local ARG_ENCRYPTION="--at-rest-encryption-enabled"
    else
        local ARG_ENCRYPTION=""
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"
    generate_temp_file FILE_TAGS_JSON "tags json"

    log "${FUNCTION_DESCRIPTION}: started"
    if(! is_empty "${TMP_SNAPSHOT_NAME}"); then
        log "- Using Snapshot: [${TMP_SNAPSHOT_NAME}]"
    fi

    log "${FUNCTION_DESCRIPTION}: Generating Tags JSON"
cat > ${FILE_TAGS_JSON} << ZZEOF
[
  { "Key": "Organization", "Value": "${ELASTICACHE_ORGANIZATIONNAME}"},
  { "Key": "Project", "Value": "${ELASTICACHE_PROJECTNAME}"},
  { "Key": "Function", "Value": "${ELASTICACHE_FUNCTIONNAME}"},
  { "Key": "Environment", "Value": "${ELASTICACHE_ENVIRONMENT}"},
  { "Key": "Owner", "Value": "${ELASTICACHE_OWNER}"},
  { "Key": "Contact", "Value": "${ELASTICACHE_CONTACT}"},
  { "Key": "DNS", "Value": "${ELASTICACHE_DNSELASTICACHERECORD}"}
]
ZZEOF

    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_EC_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_EC_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_EC_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        log "${FUNCTION_DESCRIPTION}: Creating Redis (Attempt::${COUNTER} of ${AWS_EC_DEFAULT_RETRY_COUNT})"
        $(which aws) ${AWS_REGION_STRING} elasticache create-replication-group ${ARG_REPLICATION_GROUP_ID} --replication-group-description "${ELASTICACHE_DESCRIPTION}" ${ARG_SNAPSHOT_NAME} ${ARG_CACHE_NODE_TYPE} ${ARG_ENGINE} ${ARG_ENGINE_VERSION} ${ARG_REPLICAS_PER_NODE_GROUP} ${ARG_CACHE_PARAMETER_GROUP_NAME} ${ARG_CACHE_SUBNET_GROUP_NAME} ${ARG_SECURITY_GROUP_IDS} ${ARG_PORT} ${ARG_SNAPSHOT_RETENTION_LIMIT} ${ARG_SNAPSHOT_WINDOW} ${ARG_PREFERRED_MAINTENANCE_WINDOW} ${ARG_MINOR_VERSION_UPGRADE} ${ARG_AUTOMATIC_FAILOVER} ${ARG_ENCRYPTION} --tags file://${FILE_TAGS_JSON}>${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
        RETURNVAL="$?"
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
            log "${FUNCTION_DESCRIPTION}: successfully sent create request"
            RUN=0
        fi
    done

    call_sleep 30 "allowing Elasticache Redis time to warmup"
    line_break

    log "- Waiting for Elasticache Redis to become available"
    redis_poll_status "${ELASTICACHE_REFERENCEID}" yes "${AWS_OPERATION_TIMEOUT}" "" "${TMP_AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "- Elasticache Redis failed to become available, cannot continue"
        return $RETURNVAL
    else
        log "- Elasticache Redis is now available"
    fi

    log "${FUNCTION_DESCRIPTION}: gathering endpoints"
    redis_get_replication_group_endpoint_read ELASTICACHE_ENDPOINT_READ "${ELASTICACHE_REFERENCEID}" "${TMP_AWS_REGION}"
    redis_get_replication_group_endpoint_write ELASTICACHE_ENDPOINT_WRITE "${ELASTICACHE_REFERENCEID}" "${TMP_AWS_REGION}"

    verify_array_key_values "ELASTICACHE_ENDPOINT_VARIABLES[@]" "ELASTICACHE"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "${FUNCTION_DESCRIPTION}: Failed to load required variables [Elasticache Endpoints]"
        return $RETURNVAL
    fi

    log "${FUNCTION_DESCRIPTION}: successfully created and configured cluster"
    line_break
    return 0
}

###------------------------------------------------------------------------------------------------
## FUNCTION: redis_delete()
## - Delete Redis
## - Arguments
##   - $1: Redis Reference ID
##   - $2: Region
function redis_delete() {
    local FUNCTION_DESCRIPTION="Redis (Delete)"
    local TMP_REFERENCE_ID="${1}"
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

    if(is_empty "${TMP_REFERENCE_ID}"); then
        log_error "${FUNCTION_DESCRIPTION}: Redis Reference ID not specified"
        return $E_BAD_ARGS
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    log "${FUNCTION_DESCRIPTION}: started (reference_id: [${TMP_REFERENCE_ID}] / region: [${TMP_AWS_REGION}])"
    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_EC_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_EC_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_EC_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        log "${FUNCTION_DESCRIPTION}: Deleting Redis (reference_id: [${TMP_REFERENCE_ID}] / region: [${TMP_AWS_REGION}]) (Attempt::${COUNTER} of ${AWS_EC_DEFAULT_RETRY_COUNT})"
        $(which aws) ${AWS_REGION_STRING} elasticache delete-replication-group --replication-group-id "${TMP_REFERENCE_ID}" >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
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
            log_success "${FUNCTION_DESCRIPTION}: Successfully deleted Redis"
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: redis_get_current_replication_group_id()
## - Retrieves current Replication Group ID for Redis
## - Arguments
##   - $1: Variable name to pass info to
##   - $2: Reference Name
##   - $3: Region
function redis_get_current_replication_group_id() {
    local FUNCTION_DESCRIPTION="Redis (Get Current ID)"
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
        log_error "${FUNCTION_DESCRIPTION}: Redis Reference Name not specified"
        return $E_BAD_ARGS
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_EC_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_EC_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_EC_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        $(which aws) ${AWS_REGION_STRING} elasticache describe-cache-clusters --no-show-cache-node-info --no-paginate --query "CacheClusters[].[CacheClusterCreateTime,ReplicationGroupId,CacheClusterStatus]" --output text 2>${AWS_FILE_ERROR} | awk "\$2 ~ \"^${TMP_REFERENCE_NAME}\"" | sort -r | head -1 | awk '{print $2}' >${AWS_FILE_RESPONSE}
        RETURNVAL="$?"
        # Fix Response File
        $(which sed) -i "s/\o15/_AWS_BAD_IGNORE\\n/g" "${AWS_FILE_RESPONSE}"
        $(which sed) -i '/_AWS_BAD_IGNORE/d' "${AWS_FILE_RESPONSE}"
        if [ ${RETURNVAL} -ne 0 ]; then
            if grep -q "CacheClusterNotFound" "${AWS_FILE_ERROR}"; then
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
                log "${FUNCTION_DESCRIPTION}: Redis not found"
            fi
            eval $__RESULTVAR="'$(echo "$(trim "${TMP_STRING}")" | sed -e 's/^ *//g' -e 's/ *$//g')'"
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: redis_get_endpoint()
## - Retrieves Endpoint for specified Redis
## - Arguments
##   - $1: Variable name to pass info to
##   - $2: Redis Reference ID
##   - $3: Region
function redis_get_endpoint() {
    local FUNCTION_DESCRIPTION="Redis (Get Endpoint)"
    local __RESULTVAR=$1
    local TMP_REFERENCE_ID="${2}"
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

    if(is_empty "${TMP_REFERENCE_ID}"); then
        log_error "${FUNCTION_DESCRIPTION}: Redis Reference ID not specified"
        return $E_BAD_ARGS
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_EC_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_EC_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_EC_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        $(which aws) ${AWS_REGION_STRING} elasticache describe-replication-groups --replication-group-id "${TMP_REFERENCE_ID}" --no-paginate --query "ReplicationGroups[].[NodeGroups[0].PrimaryEndpoint.Address]" --output text >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
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
## FUNCTION: redis_get_info()
## - Retrieves Redis Info for specified endpoint using redis-cli
## - Arguments
##   - $1: Endpoint Address
##   - $2: Port (optional, defaults to 6379)
function redis_get_info() {
    local FUNCTION_DESCRIPTION="Redis (Get Info)"
    local TMP_ENDPOINT_ADDRESS="${1}"
    local TMP_PORT="${2}"

    local RETURNVAL=""
    local TMP_FILE_INFO=""

    if(is_empty "${TMP_ENDPOINT_ADDRESS}"); then
        log_error "${FUNCTION_DESCRIPTION}: Redis Endpoint Address not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_PORT}"); then
        TMP_PORT="${ELASTICACHE_DEFAULT_PORT}"
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file TMP_FILE_INFO "aws_cli response file"

    log_highlight "Redis Info (${TMP_ENDPOINT_ADDRESS}:${TMP_PORT})"
    $(which redis-cli) -h "${TMP_ENDPOINT_ADDRESS}" -p "${TMP_PORT}" INFO >${TMP_FILE_INFO} 2>&1
    RETURNVAL="$?"
    $(which sed) -i 's/\r$//g' "${TMP_FILE_INFO}"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_add_from_file "${TMP_FILE_INFO}" "${FUNCTION_DESCRIPTION}: Info"
        log_error "${FUNCTION_DESCRIPTION}: operation failed (redis-cli::${RETURNVAL}])"
        return $E_ELASTICACHE_CLI_FAILURE
    fi

    log_add_from_file "${TMP_FILE_INFO}"
    return 0
}

###------------------------------------------------------------------------------------------------
## FUNCTION: redis_get_latest_snapshot_name()
## - Retrieves latest snapshot name for specified Redis Cluster
## - Arguments
##   - $1: Variable name to pass info to
##   - $2: Redis Reference ID
##   - $3: Region
function redis_get_latest_snapshot_name() {
    local FUNCTION_DESCRIPTION="Redis (Get Latest Snapshot)"
    local __RESULTVAR=$1
    local TMP_REFERENCE_ID="${2}"
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

    if(is_empty "${TMP_REFERENCE_ID}"); then
        log_error "${FUNCTION_DESCRIPTION}: Redis Reference ID not specified"
        return $E_BAD_ARGS
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_EC_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_EC_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_EC_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        $(which aws) ${AWS_REGION_STRING} elasticache describe-snapshots --no-paginate --query "Snapshots[].[NodeSnapshots[0].SnapshotCreateTime,SnapshotName,CacheClusterId]" --output text 2>${AWS_FILE_ERROR} | awk "\$3 ~ \"^${TMP_REFERENCE_ID}\"" | sort -r | head -1 | awk '{print $2}' >${AWS_FILE_RESPONSE}
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
## FUNCTION: redis_get_replication_group_endpoint_read()
## - Retrieves Redis Endpoint (Read) for specified Replication Group
## - Arguments
##   - $1: Variable name to pass info to
##   - $2: Replication Group ID
##   - $3: Region
function redis_get_replication_group_endpoint_read() {
    local FUNCTION_DESCRIPTION="Redis - Replication Group (Get Read Endpoint)"
    local __RESULTVAR=$1
    local TMP_REPLICATION_GROUP_ID="${2}"
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

    if(is_empty "${TMP_REPLICATION_GROUP_ID}"); then
        log_error "${FUNCTION_DESCRIPTION}: Replication Group ID not specified"
        return $E_BAD_ARGS
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_EC_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_EC_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_EC_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        $(which aws) ${AWS_REGION_STRING} elasticache describe-replication-groups --replication-group-id "${TMP_REPLICATION_GROUP_ID}" --query "ReplicationGroups[].[NodeGroups[0].ReaderEndpoint.Address]" --output text >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
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
## FUNCTION: redis_get_replication_group_endpoint_write()
## - Retrieves Redis Endpoint (Write) for specified Replication Group
## - Arguments
##   - $1: Variable name to pass info to
##   - $2: Replication Group ID
##   - $3: Region
function redis_get_replication_group_endpoint_write() {
    local FUNCTION_DESCRIPTION="Redis - Replication Group (Get Read Endpoint)"
    local __RESULTVAR=$1
    local TMP_REPLICATION_GROUP_ID="${2}"
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

    if(is_empty "${TMP_REPLICATION_GROUP_ID}"); then
        log_error "${FUNCTION_DESCRIPTION}: Replication Group ID not specified"
        return $E_BAD_ARGS
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_EC_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_EC_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_EC_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        $(which aws) ${AWS_REGION_STRING} elasticache describe-replication-groups --replication-group-id "${TMP_REPLICATION_GROUP_ID}" --query "ReplicationGroups[].[NodeGroups[0].PrimaryEndpoint.Address]" --output text >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
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
## FUNCTION: redis_get_status()
## - Retrieves status for specified Redis
## - Arguments
##   - $1: Variable name to pass info to
##   - $2: Redis Reference ID
##   - $3: Region
function redis_get_status() {
    local FUNCTION_DESCRIPTION="Redis (Get Status)"
    local __RESULTVAR=$1
    local TMP_REFERENCE_ID="${2}"
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

    if(is_empty "${TMP_REFERENCE_ID}"); then
        log_error "${FUNCTION_DESCRIPTION}: Redis Reference ID not specified"
        return $E_BAD_ARGS
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_EC_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_EC_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_EC_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        $(which aws) ${AWS_REGION_STRING} elasticache describe-replication-groups --replication-group-id "${TMP_REFERENCE_ID}" --no-paginate --query "ReplicationGroups[].[Status]" --output text >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
        RETURNVAL="$?"
        if [ ${RETURNVAL} -ne 0 ]; then
            if grep -q "CacheClusterNotFound" "${AWS_FILE_ERROR}"; then
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
## FUNCTION: redis_poll_status()
## - Polls Redis Status and waits for a stable state
## - Arguments
##   - $1: Redis Reference ID
##   - $2: Fail with error on NOT_FOUND <yes/no> (defaults to yes)
##   - $3: Verification Timeout <in minutes, defaults to 30 minutes>
##   - $4: Verification Time between polling <in seconds, defaults to 30 seconds)
##   - $5: Region
function redis_poll_status() {
    local FUNCTION_DESCRIPTION="Redis (Poll Status)"
    local TMP_REFERENCE_ID="${1}"
    local ERROR_ON_NOT_FOUND="${2}"
    local AWS_OPERATION_TIMEOUT="${3}"
    local AWS_EC_VERIFICATION_SLEEP="${4}"
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
    local AWS_EC_VERIFICATION_MAX_ATTEMPTS=""

    if(is_empty "${TMP_REFERENCE_ID}"); then
        log_error "${FUNCTION_DESCRIPTION}: Redis Reference ID not specified"
        return $E_BAD_ARGS
    fi

    if(is_empty "${ERROR_ON_NOT_FOUND}"); then
        ERROR_ON_NOT_FOUND=yes
    fi
    if(is_empty "${AWS_OPERATION_TIMEOUT}"); then
        AWS_OPERATION_TIMEOUT=${AWS_EC_DEFAULT_VERIFICATION_TIMEOUT}
    fi
    if(is_empty "${AWS_EC_VERIFICATION_SLEEP}"); then
        AWS_EC_VERIFICATION_SLEEP=${AWS_EC_DEFAULT_VERIFICATION_SLEEP}
    fi
    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi
    AWS_EC_VERIFICATION_MAX_ATTEMPTS=$((AWS_OPERATION_TIMEOUT * 60 / AWS_EC_VERIFICATION_SLEEP))

    log "${FUNCTION_DESCRIPTION}: started (reference_id: [${TMP_REFERENCE_ID}] / timeout: [${AWS_OPERATION_TIMEOUT}] / attempts: [${AWS_EC_VERIFICATION_MAX_ATTEMPTS}] / polling_interval: [${AWS_EC_VERIFICATION_SLEEP} seconds])"
    while [ ${POLLING_PROGRESS} == 1 ]; do
        if [[ "ZZ_${TMP_STATUS}" != "ZZ_${TMP_LAST_STATUS}" ]]; then
            if(! is_empty "${TMP_LAST_STATUS}"); then
                log "-- Status Change Detected, resetting counter [$(color_text "${LOG_COLOR_MESSAGE_IMPORTANT}" "${TMP_LAST_STATUS}") => $(color_text "${LOG_COLOR_MESSAGE_IMPORTANT}" "${TMP_STATUS}")]"
                COUNTER=0
            fi
            TMP_LAST_STATUS="${TMP_STATUS}"
        fi
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_EC_VERIFICATION_MAX_ATTEMPTS} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_EC_VERIFICATION_MAX_ATTEMPTS}] exceeded, aborting tracking operation"
            return $E_ELASTICACHE_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            sleep ${AWS_EC_VERIFICATION_SLEEP}
        fi

        redis_get_status TMP_STATUS "${TMP_REFERENCE_ID}" "${TMP_AWS_REGION}"
        RETURNVAL="$?"
        if [ ${RETURNVAL} -ne 0 ]; then
            return $RETURNVAL
        fi

        case "${TMP_STATUS}" in
            "AVAILABLE")
                TMP_STATUS_DESCRIPTION="Redis is healthy and available"
                POLLING_PROGRESS=0
                POLLING_STATUS=0
                ;;
            "SNAPSHOTTING")
                TMP_STATUS_DESCRIPTION="Redis is currently being backed up"
                POLLING_PROGRESS=1
                ;;
            "CREATING")
                TMP_STATUS_DESCRIPTION="Redis is being created. Redis is inaccessible while it is being created"
                POLLING_PROGRESS=1
                ;;
            "DELETING")
                TMP_STATUS_DESCRIPTION="Redis is being deleted"
                POLLING_PROGRESS=1
                ;;
            "DELETED")
                TMP_STATUS_DESCRIPTION="Redis has been deleted and is no longer accessible"
                POLLING_PROGRESS=0
                if option_enabled ERROR_ON_NOT_FOUND; then
                    POLLING_STATUS=$E_ELASTICACHE_FAILURE
                else
                    POLLING_STATUS=0
                fi
                ;;
            "INCOMPATIBLE-NETWORK")
                TMP_STATUS_DESCRIPTION="Redis is attempting to perform a recovery action on an instance but is unable to do so because the VPC is in a state that is preventing the action from being completed.  This status can occur if, for example, all available IP addresses in a subnet were in use and Redis was unable to get an IP address for itself"
                POLLING_PROGRESS=0
                POLLING_STATUS=$E_ELASTICACHE_FAILURE
                ;;
            "NOT_FOUND")
                TMP_STATUS_DESCRIPTION="Redis not found"
                POLLING_PROGRESS=0
                if option_enabled ERROR_ON_NOT_FOUND; then
                    POLLING_STATUS=$E_ELASTICACHE_FAILURE
                else
                    POLLING_STATUS=0
                fi
                ;;
            "MODIFYING")
                TMP_STATUS_DESCRIPTION="Redis is being modified because of a customer request to modify Redis"
                POLLING_PROGRESS=1
                ;;
            "REBOOTING")
                TMP_STATUS_DESCRIPTION="Redis is being rebooted because of a customer request or an Amazon ElastiCache process that requires the rebooting of Redis"
                POLLING_PROGRESS=1
                ;;
            "REBOOTING CLUSTER NODES")
                TMP_STATUS_DESCRIPTION="Redis is being rebooted because of a customer request or an Amazon ElastiCache process that requires the rebooting of Redis"
                POLLING_PROGRESS=1
                ;;
            "RESTORE-FAILED")
                TMP_STATUS_DESCRIPTION="Redis is unable to do a point-in-time restore"
                POLLING_PROGRESS=0
                POLLING_STATUS=$E_ELASTICACHE_FAILURE
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

        log "- Attempt: [$(printf "%02d\n" "${COUNTER}")/${AWS_EC_VERIFICATION_MAX_ATTEMPTS}] | Status: [$(color_text "${LOG_COLOR_MESSAGE_IMPORTANT}" "${TMP_STATUS}")] | Description: [$(color_text "${LOG_COLOR_NOTICE}" "${TMP_STATUS_DESCRIPTION}")]"
    done

    return $POLLING_STATUS
}

###------------------------------------------------------------------------------------------------
## FUNCTION: redis_set_dns()
## - Sets DNS records (Read & Write) for Redis
## - Arguments
##   - $1: Region
function redis_set_dns() {
    local FUNCTION_DESCRIPTION="Redis (Set DNS)"
    local TMP_AWS_REGION="${1}"

    local RETURNVAL=""
    local TMP_KEY=""
    local TMP_VAR=""
    local KEY_MAX_LENGTH=0

    verify_array_key_values "ELASTICACHE_DNS_VARIABLES[@]" "ELASTICACHE"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "${FUNCTION_DESCRIPTION}: Failed to load required variables [DNS]"
        return $RETURNVAL
    fi

    verify_array_key_values "ELASTICACHE_ENDPOINT_VARIABLES[@]" "ELASTICACHE"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "${FUNCTION_DESCRIPTION}: Failed to load required variables [Endpoints]"
        return $RETURNVAL
    fi

    log_notice "${FUNCTION_DESCRIPTION}: started"

    log "${FUNCTION_DESCRIPTION}: setting DNS record (write)"
    log "- DNS Record:       [${ELASTICACHE_DNSELASTICACHERECORDWRITE}]"
    log "- DNS Zone ID:      [${ELASTICACHE_DNSELASTICACHEHOSTEDZONEID}]"
    log "- Endpoint Address: [${ELASTICACHE_ENDPOINT_WRITE}]"
    route53_upsert_record "${ELASTICACHE_DNSELASTICACHEHOSTEDZONEID}" "${ELASTICACHE_DNSELASTICACHERECORDWRITE}" "CNAME" ${AWS_EC_DNS_TTL} "${ELASTICACHE_ENDPOINT_WRITE}" "" "" "${TMP_AWS_REGION}"
    RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then return ${RETURNVAL}; fi

    log "${FUNCTION_DESCRIPTION}: setting DNS record (read)"
    log "- DNS Record:       [${ELASTICACHE_DNSELASTICACHERECORDREAD}]"
    log "- DNS Zone ID:      [${ELASTICACHE_DNSELASTICACHEHOSTEDZONEID}]"
    log "- Endpoint Address: [${ELASTICACHE_ENDPOINT_READ}]"
    route53_upsert_record "${ELASTICACHE_DNSELASTICACHEHOSTEDZONEID}" "${ELASTICACHE_DNSELASTICACHERECORDREAD}" "CNAME" ${AWS_EC_DNS_TTL} "${ELASTICACHE_ENDPOINT_READ}" "" "" "${TMP_AWS_REGION}"
    RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then return ${RETURNVAL}; fi

    return 0
}
