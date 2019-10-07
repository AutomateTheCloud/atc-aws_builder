###------------------------------------------------------------------------------------------------
# script:  elasticache_memcached.inc
# purpose: Collection of functions related to AWS (ElastiCache - Memcached)
# version: 1.0.0
#
# function list:
#          - load_info_memcached
#          - memcached_create
#          - memcached_delete
#          - memcached_get_current_cache_cluster_id
#          - memcached_get_endpoint
#          - memcached_get_info
#          - memcached_get_status
#          - memcached_poll_status
#          - memcached_set_dns
###------------------------------------------------------------------------------------------------
REQUIRED_EXECUTABLES+=('aws' 'nc')

###------------------------------------------------------------------------------------------------
# Variables
AWS_EC_DEFAULT_RETRY_COUNT=3
AWS_EC_DEFAULT_RETRY_TIMER_MAX_SEC=30
AWS_EC_DEFAULT_OPERATION_TIMEOUT=30
AWS_EC_DEFAULT_VERIFICATION_TIMEOUT=60
AWS_EC_DEFAULT_VERIFICATION_SLEEP=30
AWS_EC_DNS_TTL=30

MEMCACHED_DEFAULT_PORT="11211"

MEMCACHED_VARIABLES+=(
    'MemcachedReferenceName'
    'MemcachedReferenceId'
    'CacheNodeType'
    'EngineVersion'
    'CacheNodeCount'

    'ParameterGroupName'
    'SubnetType'
    'ECSubnetGroupId'
    'MemcachedSecurityGroupId'
    'MemcachedPort'

    'MemcachedDNSRecord'
    'DNSDomainName'
    'DNSHostedZoneId'

    'AutoMinorVersionUpgrade'
    'PreferredMaintenanceWindow'

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

MEMCACHED_DNS_VARIABLES+=(
    'MemcachedDNSRecord'
    'DNSHostedZoneId'
    'Endpoint'
)

MEMCACHED_MEMCACHEDREFERENCENAME=""
MEMCACHED_MEMCACHEDREFERENCEID=""

MEMCACHED_CACHENODETYPE=""
MEMCACHED_ENGINEVERSION=""
MEMCACHED_CACHENODECOUNT=""

MEMCACHED_ENCRYPTIONENABLED=""

MEMCACHED_PARAMETERGROUPNAME=""
MEMCACHED_SUBNETTYPE=""
MEMCACHED_MULTIAZENABLED=""
MEMCACHED_ECSUBNETGROUPID=""
MEMCACHED_MEMCACHEDSECURITYGROUPID=""
MEMCACHED_MEMCACHEDPORT=""
MEMCACHED_MEMCACHEDDNSRECORD=""
MEMCACHED_DNSDOMAINNAME=""
MEMCACHED_DNSHOSTEDZONEID=""
MEMCACHED_ENDPOINT=""

MEMCACHED_AUTOMINORVERSIONUPGRADE=""
MEMCACHED_PREFERREDMAINTENANCEWINDOW=""

MEMCACHED_SNAPSHOTRETENTIONLIMIT=""
MEMCACHED_SNAPSHOTWINDOW=""

MEMCACHED_ACCOUNTNAME=""
MEMCACHED_ACCOUNTABBR=""
MEMCACHED_ACCOUNTNUMBER=""
MEMCACHED_ORGANIZATIONNAME=""
MEMCACHED_ORGANIZATIONABBR=""
MEMCACHED_ENVIRONMENT=""
MEMCACHED_PROJECTNAME=""
MEMCACHED_PROJECTABBR=""
MEMCACHED_FUNCTIONNAME=""
MEMCACHED_FUNCTIONABBR=""
MEMCACHED_OWNER=""
MEMCACHED_CONTACT=""
MEMCACHED_REGION=""

MEMCACHED_PREVIOUS_OCCURRENCE=""

###------------------------------------------------------------------------------------------------
## FUNCTION: load_info_memcached()
## - Loads Memcached Information into memory from CloudFormation Outputs file
## - Arguments
##   - $1: CloudFormation Outputs File
function load_info_memcached() {
    local FUNCTION_DESCRIPTION="Load Info (Memcached)"
    local TMP_FILE_CLOUDFORMATION_OUTPUTS="${1}"
    local TMP_UUID=""
    local TMP_KEY=""
    local TMP_VAR=""
    local KEY_MAX_LENGTH=0

    if(is_empty "${TMP_FILE_CLOUDFORMATION_OUTPUTS}"); then
        log_error "${FUNCTION_DESCRIPTION}: CloudFormation Outputs file not specified"
        return $E_BAD_ARGS
    fi
    if [ ! -f "${TMP_FILE_CLOUDFORMATION_OUTPUTS}" ]; then
        log_error "${FUNCTION_DESCRIPTION}: CloudFormation Outputs file does not exist [${TMP_FILE_CLOUDFORMATION_OUTPUTS}]"
        return $E_OBJECT_NOT_FOUND
    fi

    log_notice "${FUNCTION_DESCRIPTION}: loading"
    load_array_key_values_from_file "MEMCACHED_VARIABLES[@]" "${TMP_FILE_CLOUDFORMATION_OUTPUTS}" "MEMCACHED"

    generate_uuid TMP_UUID 2
    MEMCACHED_MEMCACHEDREFERENCEID="${MEMCACHED_MEMCACHEDREFERENCENAME}-${TMP_UUID}"

    if [ $MEMCACHED_CACHENODECOUNT -gt 1 ]; then
        MEMCACHED_MULTIAZENABLED=yes
    else
        MEMCACHED_MULTIAZENABLED=no
    fi

    if(is_empty "${MEMCACHED_ENCRYPTIONENABLED}"); then MEMCACHED_ENCRYPTIONENABLED=no; fi
    verify_array_key_values "MEMCACHED_VARIABLES[@]" "MEMCACHED"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "${FUNCTION_DESCRIPTION}: Failed to load required variables [Memcached]"
        return $RETURNVAL
    fi

    line_break
    log_highlight "Memcached Information"
    for TMP_KEY in "${MEMCACHED_VARIABLES[@]}"; do
        if [ ${#TMP_KEY} -gt $KEY_MAX_LENGTH ]; then
            KEY_MAX_LENGTH=${#TMP_KEY}
        fi
    done
    KEY_MAX_LENGTH=$((${KEY_MAX_LENGTH}+1))

    for TMP_KEY in "${MEMCACHED_VARIABLES[@]}"; do
        TMP_VAR="$(to_upper "MEMCACHED_${TMP_KEY}")"
        log "$(printf "%-1s %-${KEY_MAX_LENGTH}s %s" "-" "${TMP_KEY}:" "[${!TMP_VAR}]")"
    done
    line_break
    return 0
}

###------------------------------------------------------------------------------------------------
## FUNCTION: memcached_create()
## - Creates Memcached
## - Arguments
##   - $1: Region
function memcached_create() {
    local FUNCTION_DESCRIPTION="Memcached (Create)"
    local TMP_AWS_REGION="${1}"

    local RETURNVAL=""
    local TMP_STRING=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    verify_array_key_values "MEMCACHED_VARIABLES[@]" "MEMCACHED"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "${FUNCTION_DESCRIPTION}: Failed to load required variables [Memcached]"
        return $RETURNVAL
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    local ARG_CACHE_CLUSTER_ID="--cache-cluster-id ${MEMCACHED_MEMCACHEDREFERENCEID}"
    if option_enabled MEMCACHED_MULTIAZENABLED; then
        local ARG_AZ_MODE="--az-mode cross-az"
    else
        local ARG_AZ_MODE=""
    fi
    local ARG_CACHE_NODE_TYPE="--cache-node-type ${MEMCACHED_CACHENODETYPE}"
    local ARG_ENGINE="--engine memcached"
    local ARG_ENGINE_VERSION="--engine-version ${MEMCACHED_ENGINEVERSION}"
    local ARG_NUM_CACHE_NODES="--num-cache-nodes ${MEMCACHED_CACHENODECOUNT}"
    local ARG_CACHE_PARAMETER_GROUP_NAME="--cache-parameter-group-name ${MEMCACHED_PARAMETERGROUPNAME}"
    local ARG_CACHE_SUBNET_GROUP_NAME="--cache-subnet-group-name ${MEMCACHED_ECSUBNETGROUPID}"
    local ARG_SECURITY_GROUP_IDS="--security-group-ids ${MEMCACHED_MEMCACHEDSECURITYGROUPID}"
    local ARG_PORT="--port ${MEMCACHED_MEMCACHEDPORT}"
    local ARG_PREFERRED_MAINTENANCE_WINDOW="--preferred-maintenance-window ${MEMCACHED_PREFERREDMAINTENANCEWINDOW}"
    if option_enabled MEMCACHED_AUTOMINORVERSIONUPGRADE; then
        local ARG_MINOR_VERSION_UPGRADE="--auto-minor-version-upgrade"
    else
        local ARG_MINOR_VERSION_UPGRADE="--no-auto-minor-version-upgrade"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"
    generate_temp_file FILE_TAGS_JSON "tags json"


    log "${FUNCTION_DESCRIPTION}: started"

    log "${FUNCTION_DESCRIPTION}: Generating Tags JSON"
cat > ${FILE_TAGS_JSON} << ZZEOF
[
  { "Key": "Organization", "Value": "${MEMCACHED_ORGANIZATIONNAME}"},
  { "Key": "Project", "Value": "${MEMCACHED_PROJECTNAME}"},
  { "Key": "Function", "Value": "${MEMCACHED_FUNCTIONNAME}"},
  { "Key": "Environment", "Value": "${MEMCACHED_ENVIRONMENT}"},
  { "Key": "Owner", "Value": "${MEMCACHED_OWNER}"},
  { "Key": "Contact", "Value": "${MEMCACHED_CONTACT}"},
  { "Key": "DNS", "Value": "${MEMCACHED_MEMCACHEDDNSRECORD}"}
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

        log "${FUNCTION_DESCRIPTION}: Creating Memcached (Attempt::${COUNTER} of ${AWS_EC_DEFAULT_RETRY_COUNT})"
        $(which aws) ${AWS_REGION_STRING} elasticache create-cache-cluster ${ARG_CACHE_CLUSTER_ID} ${ARG_AZ_MODE} ${ARG_CACHE_NODE_TYPE} ${ARG_ENGINE} ${ARG_ENGINE_VERSION} ${ARG_NUM_CACHE_NODES} ${ARG_CACHE_PARAMETER_GROUP_NAME} ${ARG_CACHE_SUBNET_GROUP_NAME} ${ARG_SECURITY_GROUP_IDS} ${ARG_PORT} ${ARG_PREFERRED_MAINTENANCE_WINDOW} ${ARG_MINOR_VERSION_UPGRADE} --tags file://${FILE_TAGS_JSON}>${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
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
            log_success "${FUNCTION_DESCRIPTION}: Successfully created Memcached"
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: memcached_delete()
## - Delete Memcached
## - Arguments
##   - $1: Memcached Reference ID
##   - $2: Region
function memcached_delete() {
    local FUNCTION_DESCRIPTION="Memcached (Delete)"
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
        log_error "${FUNCTION_DESCRIPTION}: Memcached Reference ID not specified"
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

        log "${FUNCTION_DESCRIPTION}: Deleting Memcached (reference_id: [${TMP_REFERENCE_ID}] / region: [${TMP_AWS_REGION}]) (Attempt::${COUNTER} of ${AWS_EC_DEFAULT_RETRY_COUNT})"
        $(which aws) ${AWS_REGION_STRING} elasticache delete-cache-cluster --cache-cluster-id "${TMP_REFERENCE_ID}" >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
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
            log_success "${FUNCTION_DESCRIPTION}: Successfully deleted Memcached"
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: memcached_get_current_cache_cluster_id()
## - Retrieves current Cache Cluster ID for Memcached
## - Arguments
##   - $1: Variable name to pass info to
##   - $2: Reference Name
##   - $3: Region
function memcached_get_current_cache_cluster_id() {
    local FUNCTION_DESCRIPTION="Memcached (Get Current ID)"
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
        log_error "${FUNCTION_DESCRIPTION}: Memcached Reference Name not specified"
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

        $(which aws) ${AWS_REGION_STRING} elasticache describe-cache-clusters --no-show-cache-node-info --no-paginate --query "CacheClusters[].[CacheClusterCreateTime,CacheClusterId,CacheClusterStatus]" --output text 2>${AWS_FILE_ERROR} | awk "\$2 ~ \"^${TMP_REFERENCE_NAME}\"" | sort -r | head -1 | awk '{print $2}' >${AWS_FILE_RESPONSE}
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
                log "${FUNCTION_DESCRIPTION}: Memcached not found"
            fi
            eval $__RESULTVAR="'$(echo "$(trim "${TMP_STRING}")" | sed -e 's/^ *//g' -e 's/ *$//g')'"
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: memcached_get_endpoint()
## - Retrieves Endpoint for specified Memcached
## - Arguments
##   - $1: Variable name to pass info to
##   - $2: Memcached Reference ID
##   - $3: Region
function memcached_get_endpoint() {
    local FUNCTION_DESCRIPTION="Memcached (Get Endpoint)"
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
        log_error "${FUNCTION_DESCRIPTION}: Memcached Reference ID not specified"
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

        $(which aws) ${AWS_REGION_STRING} elasticache describe-cache-clusters --cache-cluster-id "${TMP_REFERENCE_ID}" --no-paginate --query "CacheClusters[].[ConfigurationEndpoint.Address]" --output text >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
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
## FUNCTION: memcached_get_info()
## - Retrieves Memcached Info for specified endpoint using memcached-cli
## - Arguments
##   - $1: Endpoint Address
##   - $2: Port (optional, defaults to 11211)
function memcached_get_info() {
    local FUNCTION_DESCRIPTION="Memcached (Get Info)"
    local TMP_ENDPOINT_ADDRESS="${1}"
    local TMP_PORT="${2}"

    local RETURNVAL=""
    local TMP_FILE_INFO=""

    if(is_empty "${TMP_ENDPOINT_ADDRESS}"); then
        log_error "${FUNCTION_DESCRIPTION}: Memcached Endpoint Address not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_PORT}"); then
        TMP_PORT="${MEMCACHED_DEFAULT_PORT}"
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file TMP_FILE_INFO "aws_cli response file"

    log_highlight "Memcached Info (${TMP_ENDPOINT_ADDRESS}:${TMP_PORT})"
    echo "stats" | nc -w 15 ${TMP_ENDPOINT_ADDRESS} ${TMP_PORT} >${TMP_FILE_INFO} 2>&1
    RETURNVAL="$?"
    $(which sed) -i 's/\r$//g' "${TMP_FILE_INFO}"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_add_from_file "${TMP_FILE_INFO}" "${FUNCTION_DESCRIPTION}: Error"
        log_error "${FUNCTION_DESCRIPTION}: operation failed (nc::${RETURNVAL}])"
        return $E_MEMCACHED_FAILURE
    fi
    log_add_from_file "${TMP_FILE_INFO}" "${FUNCTION_DESCRIPTION}: stats"

    echo "version" | nc -w 15 ${TMP_ENDPOINT_ADDRESS} ${TMP_PORT} >${TMP_FILE_INFO} 2>&1
    RETURNVAL="$?"
    $(which sed) -i 's/\r$//g' "${TMP_FILE_INFO}"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_add_from_file "${TMP_FILE_INFO}" "${FUNCTION_DESCRIPTION}: Error"
        log_error "${FUNCTION_DESCRIPTION}: operation failed (nc::${RETURNVAL}])"
        return $E_MEMCACHED_FAILURE
    fi
    log_add_from_file "${TMP_FILE_INFO}" "${FUNCTION_DESCRIPTION}: version"
    return 0
}

###------------------------------------------------------------------------------------------------
## FUNCTION: memcached_get_status()
## - Retrieves status for specified Memcached
## - Arguments
##   - $1: Variable name to pass info to
##   - $2: Memcached Reference ID
##   - $3: Region
function memcached_get_status() {
    local FUNCTION_DESCRIPTION="Memcached (Get Status)"
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
        log_error "${FUNCTION_DESCRIPTION}: Memcached Reference ID not specified"
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

        $(which aws) ${AWS_REGION_STRING} elasticache describe-cache-clusters --cache-cluster-id "${TMP_REFERENCE_ID}" --no-paginate --query "CacheClusters[].[CacheClusterStatus]" --output text >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
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
## FUNCTION: memcached_poll_status()
## - Polls Memcached Status and waits for a stable state
## - Arguments
##   - $1: Memcached Reference ID
##   - $2: Fail with error on NOT_FOUND <yes/no> (defaults to yes)
##   - $3: Verification Timeout <in minutes, defaults to 30 minutes>
##   - $4: Verification Time between polling <in seconds, defaults to 30 seconds)
##   - $5: Region
function memcached_poll_status() {
    local FUNCTION_DESCRIPTION="Memcached (Poll Status)"
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
        log_error "${FUNCTION_DESCRIPTION}: Memcached Reference ID not specified"
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

        memcached_get_status TMP_STATUS "${TMP_REFERENCE_ID}" "${TMP_AWS_REGION}"
        RETURNVAL="$?"
        if [ ${RETURNVAL} -ne 0 ]; then
            return $RETURNVAL
        fi

        case "${TMP_STATUS}" in
            "AVAILABLE")
                TMP_STATUS_DESCRIPTION="Memcached is healthy and available"
                POLLING_PROGRESS=0
                POLLING_STATUS=0
                ;;
            "SNAPSHOTTING")
                TMP_STATUS_DESCRIPTION="Memcached is currently being backed up"
                POLLING_PROGRESS=1
                ;;
            "CREATING")
                TMP_STATUS_DESCRIPTION="Memcached is being created. Memcached is inaccessible while it is being created"
                POLLING_PROGRESS=1
                ;;
            "DELETING")
                TMP_STATUS_DESCRIPTION="Memcached is being deleted"
                POLLING_PROGRESS=1
                ;;
            "DELETED")
                TMP_STATUS_DESCRIPTION="Memcached has been deleted and is no longer accessible"
                POLLING_PROGRESS=0
                if option_enabled ERROR_ON_NOT_FOUND; then
                    POLLING_STATUS=$E_ELASTICACHE_FAILURE
                else
                    POLLING_STATUS=0
                fi
                ;;
            "INCOMPATIBLE-NETWORK")
                TMP_STATUS_DESCRIPTION="Memcached is attempting to perform a recovery action on an instance but is unable to do so because the VPC is in a state that is preventing the action from being completed.  This status can occur if, for example, all available IP addresses in a subnet were in use and Memcached was unable to get an IP address for itself"
                POLLING_PROGRESS=0
                POLLING_STATUS=$E_ELASTICACHE_FAILURE
                ;;
            "NOT_FOUND")
                TMP_STATUS_DESCRIPTION="Memcached not found"
                POLLING_PROGRESS=0
                if option_enabled ERROR_ON_NOT_FOUND; then
                    POLLING_STATUS=$E_ELASTICACHE_FAILURE
                else
                    POLLING_STATUS=0
                fi
                ;;
            "MODIFYING")
                TMP_STATUS_DESCRIPTION="Memcached is being modified because of a customer request to modify Memcached"
                POLLING_PROGRESS=1
                ;;
            "REBOOTING")
                TMP_STATUS_DESCRIPTION="Memcached is being rebooted because of a customer request or an Amazon ElastiCache process that requires the rebooting of Memcached"
                POLLING_PROGRESS=1
                ;;
            "REBOOTING CLUSTER NODES")
                TMP_STATUS_DESCRIPTION="Memcached is being rebooted because of a customer request or an Amazon ElastiCache process that requires the rebooting of Memcached"
                POLLING_PROGRESS=1
                ;;
            "RESTORE-FAILED")
                TMP_STATUS_DESCRIPTION="Memcached is unable to do a point-in-time restore"
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
## FUNCTION: memcached_set_dns()
## - Sets DNS record for Memcached
## - Arguments
##   - $1: Region
function memcached_set_dns() {
    local FUNCTION_DESCRIPTION="Memcached (Set DNS)"
    local TMP_AWS_REGION="${1}"

    local RETURNVAL=""
    local TMP_KEY=""
    local TMP_VAR=""
    local KEY_MAX_LENGTH=0

    verify_array_key_values "MEMCACHED_DNS_VARIABLES[@]" "MEMCACHED"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "${FUNCTION_DESCRIPTION}: Failed to load required variables [Memcached DNS]"
        return $RETURNVAL
    fi
    line_break
    log_highlight "DNS Information"
    for TMP_KEY in "${MEMCACHED_DNS_VARIABLES[@]}"; do
        if [ ${#TMP_KEY} -gt $KEY_MAX_LENGTH ]; then
            KEY_MAX_LENGTH=${#TMP_KEY}
        fi
    done
    KEY_MAX_LENGTH=$((${KEY_MAX_LENGTH}+1))

    for TMP_KEY in "${MEMCACHED_DNS_VARIABLES[@]}"; do
        TMP_VAR="$(to_upper "MEMCACHED_${TMP_KEY}")"
        log "$(printf "%-1s %-${KEY_MAX_LENGTH}s %s" "-" "${TMP_KEY}:" "[${!TMP_VAR}]")"
    done
    line_break

    route53_upsert_record "${MEMCACHED_DNSHOSTEDZONEID}" "${MEMCACHED_MEMCACHEDDNSRECORD}" "CNAME" ${AWS_EC_DNS_TTL} "${MEMCACHED_ENDPOINT}" "" "" "${TMP_AWS_REGION}"
    RETURNVAL="$?"
    return $RETURNVAL
}
