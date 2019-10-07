###------------------------------------------------------------------------------------------------
# script:  rds_aurora_postgresql.inc
# purpose: Collection of functions related to AWS (RDS - Aurora PostgreSQL)
# version: 1.0.0
#
# function list:
#          - load_info_aurora_postgresql
#          - aurora_postgresql_create_cluster
#          - aurora_postgresql_create_cluster_from_snapshot
#          - aurora_postgresql_create_database
#          - aurora_postgresql_create_from_snapshot
#          - aurora_postgresql_execute_sql
#          - aurora_postgresql_set_dns
#          - aurora_postgresql_verify_connection
###------------------------------------------------------------------------------------------------
REQUIRED_EXECUTABLES+=('aws' 'psql')

###------------------------------------------------------------------------------------------------
# Variables
DATABASE_DEFAULT_PORT="5432"

STACK_VARIABLES_REQUIRED+=(
    'ReferenceName'
    'DatabaseInstanceClass'
    'DatabaseCount'
    'EngineVersion'
    'ClusterParameterGroupName'
    'ParameterGroupName'
    'DatabaseName'
    'EncryptionEnabled'
    'SubnetType'
    'Port'
    'DatabaseSubnetGroupId'
    'DatabaseSecurityGroupId'
    'AutoMinorVersionUpgrade'
    'PreferredMaintenanceWindow'
    'BackupRetentionLimit'
    'BackupWindow'
    'PerformanceInsightsEnabled'
    'PerformanceInsightsRetentionPeriod'
    'DNSDatabaseDomain'
    'DNSDatabaseHostedZoneID'
    'DNSDatabaseRecordWrite'
    'DNSDatabaseRecordRead'
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

SECRETS_VARIABLES_REQUIRED+=(
    'MasterUsername'
    'MasterPassword'
)

DATABASE_DETAILS_VARIABLES+=(
    'ReferenceName'
    'DatabaseInstanceClass'
    'EngineVersion'
    'ClusterParameterGroupName'
    'ParameterGroupName'
    'DatabaseName'
    'EncryptionEnabled'
)

DATABASE_CLUSTER_VARIABLES+=(
    'ClusterId'
    'DatabaseCount'
)

DATABASE_NETWORK_VARIABLES+=(
    'Port'
    'SubnetType'
    'DatabaseSubnetGroupId'
    'DatabaseSecurityGroupId'
)

DATABASE_BACKUP_VARIABLES+=(
    'BackupRetentionLimit'
    'BackupWindow'
)

DATABASE_MAINTENANCE_VARIABLES+=(
    'AutoMinorVersionUpgrade'
    'PreferredMaintenanceWindow'
)

DATABASE_DNS_VARIABLES+=(
    'DNSDatabaseHostedZoneID'
    'DNSDatabaseRecordWrite'
    'DNSDatabaseRecordRead'
)

DATABASE_ENDPOINT_VARIABLES+=(
    'Endpoint_Read'
    'Endpoint_Write'
)

DATABASE_INFO_VARIABLES+=(
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

CLUSTER_VARIABLES_REQUIRED+=(
    'ClusterId'
    'Engine'
    'EngineVersion'
    'Port'
    'DatabaseName'
    'ClusterParameterGroupName'
    'DatabaseSubnetGroupId'
    'DatabaseSecurityGroupId'
    'EncryptionEnabled'
    'PreferredMaintenanceWindow'
    'BackupRetentionLimit'
    'BackupWindow'
    'MasterUsername'
    'MasterPassword'
)

DATABASE_VARIABLES_REQUIRED+=(
    'ClusterId'
    'Engine'
    'DatabaseInstanceClass'
    'ParameterGroupName'
    'AutoMinorVersionUpgrade'
    'PerformanceInsightsEnabled'
    'PerformanceInsightsRetentionPeriod'
)

DATABASE_ENGINE="aurora-postgresql"
DATABASE_CLUSTERID=""
DATABASE_ENDPOINT_READ=""
DATABASE_ENDPOINT_WRITE=""
ARRAY_DATABASES=()

###------------------------------------------------------------------------------------------------
## FUNCTION: load_info_aurora_postgresql()
## - Loads Aurora PostgreSQL Information into memory
## - Arguments
##   - $1: CloudFormation Stack Name
##   - $2: Region
function load_info_aurora_postgresql() {
    local FUNCTION_DESCRIPTION="Load Info (Aurora PostgreSQL)"
    local TMP_CLOUDFORMATION_STACK_NAME="${1}"
    local TMP_AWS_REGION="${2}"

    local TMP_KEY=""
    local TMP_VAR=""
    local TMP_PARAMETER=""
    local KEY_MAX_LENGTH=0
    local TMP_COUNTER=0

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

    load_array_properties_from_file "STACK_VARIABLES_REQUIRED[@]" "${TMP_FILE_CLOUDFORMATION_OUTPUT}" "DATABASE"
    verify_array_key_values "STACK_VARIABLES_REQUIRED[@]" "DATABASE"
    RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then return $RETURNVAL; fi

    log_notice "${FUNCTION_DESCRIPTION}: loading Secrets"
    for TMP_KEY in "${SECRETS_VARIABLES_REQUIRED[@]}"; do
        TMP_VAR="$(to_upper "DATABASE_${TMP_KEY}")"
        TMP_PARAMETER="/secrets/${DATABASE_PROJECTABBR}/${DATABASE_FUNCTIONABBR}/${DATABASE_ENVIRONMENT}/vars/${TMP_KEY}"
        parameter_get_silent ${TMP_VAR} "${TMP_PARAMETER}" yes "${TMP_AWS_REGION}"
    done
    verify_array_key_values "SECRETS_VARIABLES_REQUIRED[@]" "DATABASE"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "${FUNCTION_DESCRIPTION}: Failed to load required variables [Secrets]"
        return $RETURNVAL
    fi

    if(! is_int "${DATABASE_DATABASECOUNT}"); then
        log_error "${FUNCTION_DESCRIPTION}: Database Count is not an integer [${DATABASE_DATABASECOUNT}]"
        return $E_BAD_ARGS
    fi

    DATABASE_CLUSTERID="${DATABASE_REFERENCENAME}-$(generate_uuid "" 2)"

    for (( DATABASE_ITERATION=1; DATABASE_ITERATION<=${DATABASE_DATABASECOUNT}; DATABASE_ITERATION++ )); do
        ARRAY_DATABASES+=("${DATABASE_CLUSTERID}-$(generate_uuid "" 3)")
    done

    for TMP_KEY in "${DATABASE_DETAILS_VARIABLES[@]}"; do
        if [ ${#TMP_KEY} -gt $KEY_MAX_LENGTH ]; then KEY_MAX_LENGTH=${#TMP_KEY}; fi
    done
    for TMP_KEY in "${DATABASE_NETWORK_VARIABLES[@]}"; do
        if [ ${#TMP_KEY} -gt $KEY_MAX_LENGTH ]; then KEY_MAX_LENGTH=${#TMP_KEY}; fi
    done
    for TMP_KEY in "${DATABASE_BACKUP_VARIABLES[@]}"; do
        if [ ${#TMP_KEY} -gt $KEY_MAX_LENGTH ]; then KEY_MAX_LENGTH=${#TMP_KEY}; fi
    done
    for TMP_KEY in "${DATABASE_MAINTENANCE_VARIABLES[@]}"; do
        if [ ${#TMP_KEY} -gt $KEY_MAX_LENGTH ]; then KEY_MAX_LENGTH=${#TMP_KEY}; fi
    done
    for TMP_KEY in "${DATABASE_DNS_VARIABLES[@]}"; do
        if [ ${#TMP_KEY} -gt $KEY_MAX_LENGTH ]; then KEY_MAX_LENGTH=${#TMP_KEY}; fi
    done
    for TMP_KEY in "${DATABASE_INFO_VARIABLES[@]}"; do
        if [ ${#TMP_KEY} -gt $KEY_MAX_LENGTH ]; then KEY_MAX_LENGTH=${#TMP_KEY}; fi
    done
    for TMP_KEY in "${DATABASE_CLUSTER_VARIABLES[@]}"; do
        if [ ${#TMP_KEY} -gt $KEY_MAX_LENGTH ]; then KEY_MAX_LENGTH=${#TMP_KEY}; fi
    done
    KEY_MAX_LENGTH=$((${KEY_MAX_LENGTH}+1))

    line_break
    log_highlight "Database - Details"
    for TMP_KEY in "${DATABASE_DETAILS_VARIABLES[@]}"; do
        TMP_VAR="$(to_upper "DATABASE_${TMP_KEY}")"
        log "$(printf "%-1s %-${KEY_MAX_LENGTH}s %s" "-" "${TMP_KEY}:" "[${!TMP_VAR}]")"
    done

    line_break
    log_highlight "Database - Info"
    for TMP_KEY in "${DATABASE_INFO_VARIABLES[@]}"; do
        TMP_VAR="$(to_upper "DATABASE_${TMP_KEY}")"
        log "$(printf "%-1s %-${KEY_MAX_LENGTH}s %s" "-" "${TMP_KEY}:" "[${!TMP_VAR}]")"
    done

    line_break
    log_highlight "Database - Network"
    for TMP_KEY in "${DATABASE_NETWORK_VARIABLES[@]}"; do
        TMP_VAR="$(to_upper "DATABASE_${TMP_KEY}")"
        log "$(printf "%-1s %-${KEY_MAX_LENGTH}s %s" "-" "${TMP_KEY}:" "[${!TMP_VAR}]")"
    done

    line_break
    log_highlight "Database - Backup"
    for TMP_KEY in "${DATABASE_BACKUP_VARIABLES[@]}"; do
        TMP_VAR="$(to_upper "DATABASE_${TMP_KEY}")"
        log "$(printf "%-1s %-${KEY_MAX_LENGTH}s %s" "-" "${TMP_KEY}:" "[${!TMP_VAR}]")"
    done

    line_break
    log_highlight "Database - Maintenance"
    for TMP_KEY in "${DATABASE_MAINTENANCE_VARIABLES[@]}"; do
        TMP_VAR="$(to_upper "DATABASE_${TMP_KEY}")"
        log "$(printf "%-1s %-${KEY_MAX_LENGTH}s %s" "-" "${TMP_KEY}:" "[${!TMP_VAR}]")"
    done

    line_break
    log_highlight "Database - DNS"
    for TMP_KEY in "${DATABASE_DNS_VARIABLES[@]}"; do
        TMP_VAR="$(to_upper "DATABASE_${TMP_KEY}")"
        log "$(printf "%-1s %-${KEY_MAX_LENGTH}s %s" "-" "${TMP_KEY}:" "[${!TMP_VAR}]")"
    done

    line_break
    log_highlight "Database - Credentials"
    log "$(printf "%-1s %-${KEY_MAX_LENGTH}s %s" "-" "MasterUsername:" "[${DATABASE_MASTERUSERNAME}]")"
    log "$(printf "%-1s %-${KEY_MAX_LENGTH}s %s" "-" "MasterPassword:" "[${DATABASE_MASTERPASSWORD//?/*}]")"
    line_break

    line_break
    log_highlight "Database - Cluster"
    for TMP_KEY in "${DATABASE_CLUSTER_VARIABLES[@]}"; do
        TMP_VAR="$(to_upper "DATABASE_${TMP_KEY}")"
        log "$(printf "%-1s %-${KEY_MAX_LENGTH}s %s" "-" "${TMP_KEY}:" "[${!TMP_VAR}]")"
    done

    TMP_COUNTER=0
    for TMP_VAR in "${ARRAY_DATABASES[@]}"; do
        COUNTER=$((${COUNTER}+1))
        log "$(printf "%-1s %-${KEY_MAX_LENGTH}s %s" "-" "Database ${COUNTER}:" "[${TMP_VAR}]")"
    done

    return 0
}

###------------------------------------------------------------------------------------------------
## FUNCTION: aurora_postgresql_create_cluster()
## - Creates Aurora PostgreSQL Cluster
## - Arguments
##   - $1: Verification Timeout <in minutes, defaults to 30 minutes>
##   - $2: Region
function aurora_postgresql_create_cluster() {
    local FUNCTION_DESCRIPTION="Aurora Cluster (Create)"
    local AWS_OPERATION_TIMEOUT="${1}"
    local TMP_AWS_REGION="${2}"

    local RETURNVAL=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    verify_array_key_values "CLUSTER_VARIABLES_REQUIRED[@]" "DATABASE"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "${FUNCTION_DESCRIPTION}: Failed to load required variables [Aurora Cluster]"
        return $RETURNVAL
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    local ARG_DATABASE_CLUSTER_IDENTIFIER="--db-cluster-identifier ${DATABASE_CLUSTERID}"
    local ARG_DATABASE_NAME="--database-name ${DATABASE_DATABASENAME}"
    local ARG_MASTER_USERNAME="--master-username ${DATABASE_MASTERUSERNAME}"
    local ARG_MASTER_USER_PASSWORD="--master-user-password ${DATABASE_MASTERPASSWORD}"
    local ARG_ENGINE="--engine ${DATABASE_ENGINE}"
    local ARG_ENGINE_VERSION="--engine-version ${DATABASE_ENGINEVERSION}"
    local ARG_VPC_SECURITY_GROUP_IDS="--vpc-security-group-ids ${DATABASE_DATABASESECURITYGROUPID}"
    local ARG_DATABASE_SUBNET_GROUP_NAME="--db-subnet-group-name ${DATABASE_DATABASESUBNETGROUPID}"
    local ARG_PORT="--port ${DATABASE_PORT}"
    local ARG_CLUSTER_PARAMETER_GROUP_NAME="--db-cluster-parameter-group-name ${DATABASE_CLUSTERPARAMETERGROUPNAME}"
    if [ ${DATABASE_BACKUPRETENTIONLIMIT} -gt 0 ]; then
        local ARG_BACKUP_RETENTION_PERIOD="--backup-retention-period ${DATABASE_BACKUPRETENTIONLIMIT}"
        local ARG_PREFERRED_BACKUP_WINDOW="--preferred-backup-window ${DATABASE_BACKUPWINDOW}"
        local ARG_COPY_TAGS_TO_SNAPSHOT="--copy-tags-to-snapshot"
    else
        local ARG_BACKUP_RETENTION_PERIOD=""
        local ARG_PREFERRED_BACKUP_WINDOW=""
        local ARG_COPY_TAGS_TO_SNAPSHOT=""
    fi
    local ARG_PREFERRED_MAINTENANCE_WINDOW="--preferred-maintenance-window ${DATABASE_PREFERREDMAINTENANCEWINDOW}"
    if option_enabled DATABASE_ENCRYPTIONENABLED; then
        local ARG_ENCRYPTION="--storage-encrypted"
    else
        local ARG_ENCRYPTION="--no-storage-encrypted"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"
    generate_temp_file FILE_TAGS_JSON "tags json"

    log_notice "${FUNCTION_DESCRIPTION}: started (database_id: [${DATABASE_CLUSTERID}])"

    log "${FUNCTION_DESCRIPTION}: Generating Tags JSON"
cat > ${FILE_TAGS_JSON} << ZZEOF
[
  { "Key": "Organization", "Value": "${DATABASE_ORGANIZATIONNAME}"},
  { "Key": "Project", "Value": "${DATABASE_PROJECTNAME}"},
  { "Key": "Function", "Value": "${DATABASE_FUNCTIONNAME}"},
  { "Key": "Environment", "Value": "${DATABASE_ENVIRONMENT}"},
  { "Key": "Owner", "Value": "${DATABASE_OWNER}"},
  { "Key": "Contact", "Value": "${DATABASE_CONTACT}"},
  { "Key": "DNS_Write", "Value": "${DATABASE_DNSDATABASERECORDWRITE}"},
  { "Key": "DNS_Read", "Value": "${DATABASE_DNSDATABASERECORDREAD}"}
]
ZZEOF

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

        log "${FUNCTION_DESCRIPTION}: Creating Cluster (Attempt::${COUNTER} of ${AWS_RDS_DEFAULT_RETRY_COUNT})"
        $(which aws) ${AWS_REGION_STRING} rds create-db-cluster \
            ${ARG_DATABASE_CLUSTER_IDENTIFIER} \
            ${ARG_DATABASE_NAME} \
            ${ARG_MASTER_USERNAME} \
            ${ARG_MASTER_USER_PASSWORD} \
            ${ARG_ENGINE} \
            ${ARG_ENGINE_VERSION} \
            ${ARG_VPC_SECURITY_GROUP_IDS} \
            ${ARG_DATABASE_SUBNET_GROUP_NAME} \
            ${ARG_PORT} \
            ${ARG_CLUSTER_PARAMETER_GROUP_NAME} \
            ${ARG_BACKUP_RETENTION_PERIOD} \
            ${ARG_PREFERRED_BACKUP_WINDOW} \
            ${ARG_COPY_TAGS_TO_SNAPSHOT} \
            ${ARG_PREFERRED_MAINTENANCE_WINDOW} \
            ${ARG_ENCRYPTION} \
            --tags file://${FILE_TAGS_JSON}>${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
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

    call_sleep 30 "allowing Aurora Cluster time to warmup"
    line_break

    log "- Waiting for Aurora Cluster to become available"
    rds_poll_cluster_status "${DATABASE_CLUSTERID}" yes "${AWS_OPERATION_TIMEOUT}" "" "${TMP_AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "- Aurora Cluster failed to become available, cannot continue"
        return $RETURNVAL
    else
        log "- Aurora Cluster is now available"
    fi

    log "${FUNCTION_DESCRIPTION}: gathering endpoints"
    rds_get_cluster_endpoint_read DATABASE_ENDPOINT_READ "${DATABASE_CLUSTERID}" "${TMP_AWS_REGION}"
    rds_get_cluster_endpoint_write DATABASE_ENDPOINT_WRITE "${DATABASE_CLUSTERID}" "${TMP_AWS_REGION}"

    verify_array_key_values "DATABASE_ENDPOINT_VARIABLES[@]" "DATABASE"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "${FUNCTION_DESCRIPTION}: Failed to load required variables [Cluster Endpoints]"
        return $RETURNVAL
    fi

    log "${FUNCTION_DESCRIPTION}: successfully created and configured cluster"
    line_break
    return 0
}

###------------------------------------------------------------------------------------------------
## FUNCTION: aurora_postgresql_create_cluster_from_snapshot()
## - Creates Aurora PostgreSQL Cluster from Snapshot
## - Arguments
##   - $1: Snapshot ARN
##   - $2: Verification Timeout <in minutes, defaults to 30 minutes>
##   - $3: Region
function aurora_postgresql_create_cluster_from_snapshot() {
    local FUNCTION_DESCRIPTION="Aurora Cluster (Create from Snapshot)"
    local TMP_SNAPSHOT_ARN="${1}"
    local AWS_OPERATION_TIMEOUT="${2}"
    local TMP_AWS_REGION="${3}"

    local RETURNVAL=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    verify_array_key_values "CLUSTER_VARIABLES_REQUIRED[@]" "DATABASE"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "${FUNCTION_DESCRIPTION}: Failed to load required variables [Aurora Cluster]"
        return $RETURNVAL
    fi

    if(is_empty "${TMP_SNAPSHOT_ARN}"); then
        log_error "${FUNCTION_DESCRIPTION}: Snapshot ARN not specified"
        return $E_BAD_ARGS
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    local ARG_DATABASE_CLUSTER_IDENTIFIER="--db-cluster-identifier ${DATABASE_CLUSTERID}"
    local ARG_DATABASE_NAME="--database-name ${DATABASE_DATABASENAME}"
    local ARG_SNAPSHOT_IDENTIFIER="--snapshot-identifier ${TMP_SNAPSHOT_ARN}"
    local ARG_ENGINE="--engine ${DATABASE_ENGINE}"
    local ARG_ENGINE_VERSION="--engine-version ${DATABASE_ENGINEVERSION}"
    local ARG_VPC_SECURITY_GROUP_IDS="--vpc-security-group-ids ${DATABASE_DATABASESECURITYGROUPID}"
    local ARG_DATABASE_SUBNET_GROUP_NAME="--db-subnet-group-name ${DATABASE_DATABASESUBNETGROUPID}"
    local ARG_PORT="--port ${DATABASE_PORT}"
    local ARG_CLUSTER_PARAMETER_GROUP_NAME="--db-cluster-parameter-group-name ${DATABASE_CLUSTERPARAMETERGROUPNAME}"

    local ARG_MASTER_USERNAME="--master-username ${DATABASE_MASTERUSERNAME}"
    local ARG_MASTER_USER_PASSWORD="--master-user-password ${DATABASE_MASTERPASSWORD}"
    if [ ${DATABASE_BACKUPRETENTIONLIMIT} -gt 0 ]; then
        local ARG_BACKUP_RETENTION_PERIOD="--backup-retention-period ${DATABASE_BACKUPRETENTIONLIMIT}"
        local ARG_PREFERRED_BACKUP_WINDOW="--preferred-backup-window ${DATABASE_BACKUPWINDOW}"
        local ARG_COPY_TAGS_TO_SNAPSHOT="--copy-tags-to-snapshot"
    else
        local ARG_BACKUP_RETENTION_PERIOD=""
        local ARG_PREFERRED_BACKUP_WINDOW=""
        local ARG_COPY_TAGS_TO_SNAPSHOT=""
    fi
    local ARG_PREFERRED_MAINTENANCE_WINDOW="--preferred-maintenance-window ${DATABASE_PREFERREDMAINTENANCEWINDOW}"
    
    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"
    generate_temp_file FILE_TAGS_JSON "tags json"

    log_notice "${FUNCTION_DESCRIPTION}: started (database_id: [${DATABASE_CLUSTERID}])"
    log "- Snapshot ARN:     [${TMP_SNAPSHOT_ARN}]"

    log "${FUNCTION_DESCRIPTION}: Generating Tags JSON"
cat > ${FILE_TAGS_JSON} << ZZEOF
[
  { "Key": "Organization", "Value": "${DATABASE_ORGANIZATIONNAME}"},
  { "Key": "Project", "Value": "${DATABASE_PROJECTNAME}"},
  { "Key": "Function", "Value": "${DATABASE_FUNCTIONNAME}"},
  { "Key": "Environment", "Value": "${DATABASE_ENVIRONMENT}"},
  { "Key": "Owner", "Value": "${DATABASE_OWNER}"},
  { "Key": "Contact", "Value": "${DATABASE_CONTACT}"},
  { "Key": "DNS_Write", "Value": "${DATABASE_DNSDATABASERECORDWRITE}"},
  { "Key": "DNS_Read", "Value": "${DATABASE_DNSDATABASERECORDREAD}"}
]
ZZEOF

    RUN=1
    COUNTER=0
    RETRY_ENABLED=no
    HAS_ERROR=no
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

        log "${FUNCTION_DESCRIPTION}: Creating Cluster (Attempt::${COUNTER} of ${AWS_RDS_DEFAULT_RETRY_COUNT})"
        $(which aws) ${AWS_REGION_STRING} rds restore-db-cluster-from-snapshot \
            ${ARG_DATABASE_CLUSTER_IDENTIFIER} \
            ${ARG_DATABASE_NAME} \
            ${ARG_SNAPSHOT_IDENTIFIER} \
            ${ARG_ENGINE} \
            ${ARG_ENGINE_VERSION} \
            ${ARG_VPC_SECURITY_GROUP_IDS} \
            ${ARG_DATABASE_SUBNET_GROUP_NAME} \
            ${ARG_PORT} \
            ${ARG_CLUSTER_PARAMETER_GROUP_NAME} \
            --tags file://${FILE_TAGS_JSON}>${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
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

    call_sleep 30 "allowing Aurora Cluster time to warmup"
    line_break

    log "- Waiting for Aurora Cluster to become available"
    rds_poll_cluster_status "${DATABASE_CLUSTERID}" yes "${AWS_OPERATION_TIMEOUT}" "" "${TMP_AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "- Aurora Cluster failed to become available, cannot continue"
        return $RETURNVAL
    else
        log "- Aurora Cluster is now available"
    fi

    RUN=1
    COUNTER=0
    RETRY_ENABLED=no
    HAS_ERROR=no
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

        log "${FUNCTION_DESCRIPTION}: Updating Cluster (Attempt::${COUNTER} of ${AWS_RDS_DEFAULT_RETRY_COUNT})"
        $(which aws) ${AWS_REGION_STRING} rds modify-db-cluster \
            ${ARG_DATABASE_CLUSTER_IDENTIFIER} \
            ${ARG_MASTER_USER_PASSWORD} \
            ${ARG_BACKUP_RETENTION_PERIOD} \
            ${ARG_PREFERRED_BACKUP_WINDOW} \
            ${ARG_COPY_TAGS_TO_SNAPSHOT} \
            ${ARG_PREFERRED_MAINTENANCE_WINDOW} \
            --apply-immediately >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
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
            log "${FUNCTION_DESCRIPTION}: successfully sent update request"
            RUN=0
        fi
    done

    call_sleep 60 "allowing Aurora Cluster time to warmup"
    line_break

    log "- Waiting for Aurora Cluster to become available"
    rds_poll_cluster_status "${DATABASE_CLUSTERID}" yes "${AWS_OPERATION_TIMEOUT}" "" "${TMP_AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "- Aurora Cluster failed to become available, cannot continue"
        return $RETURNVAL
    else
        log "- Aurora Cluster is now available"
    fi

    log "${FUNCTION_DESCRIPTION}: gathering endpoints"
    rds_get_cluster_endpoint_read DATABASE_ENDPOINT_READ "${DATABASE_CLUSTERID}" "${TMP_AWS_REGION}"
    rds_get_cluster_endpoint_write DATABASE_ENDPOINT_WRITE "${DATABASE_CLUSTERID}" "${TMP_AWS_REGION}"

    verify_array_key_values "DATABASE_ENDPOINT_VARIABLES[@]" "DATABASE"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "${FUNCTION_DESCRIPTION}: Failed to load required variables [Cluster Endpoints]"
        return $RETURNVAL
    fi

    log "${FUNCTION_DESCRIPTION}: successfully created and configured cluster"
    line_break
    return 0
}

###------------------------------------------------------------------------------------------------
## FUNCTION: aurora_postgresql_create_database()
## - Creates Aurora PostgreSQL Database for Cluster
## - Does not perform polling
## - Arguments
##   - $1: Database ID
##   - $2: Region
function aurora_postgresql_create_database() {
    local FUNCTION_DESCRIPTION="Aurora PostgreSQL (Create Database)"
    local TMP_DATABASE_ID="${1}"
    local TMP_AWS_REGION="${2}"

    local RETURNVAL=""
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

    verify_array_key_values "DATABASE_VARIABLES_REQUIRED[@]" "DATABASE"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "${FUNCTION_DESCRIPTION}: Failed to load required variables [Aurora PostgreSQL Cluster]"
        return $RETURNVAL
    fi

    log_notice "${FUNCTION_DESCRIPTION}: started"
    log "- Database ID: [${TMP_DATABASE_ID}]"
    log "- Cluster ID:  [${DATABASE_CLUSTERID}]"
    log "- Region:      [${TMP_AWS_REGION}]"

    local ARG_DATABASE_INSTANCE_IDENTIFIER="--db-instance-identifier ${TMP_DATABASE_ID}"
    local ARG_CLUSTER_INSTANCE_IDENTIFIER="--db-cluster-identifier ${DATABASE_CLUSTERID}"
    local ARG_ENGINE="--engine ${DATABASE_ENGINE}"
    local ARG_DATABASE_INSTANCE_CLASS="--db-instance-class ${DATABASE_DATABASEINSTANCECLASS}"
    local ARG_DATABASE_PARAMETER_GROUP_NAME="--db-parameter-group-name ${DATABASE_PARAMETERGROUPNAME}"
    if option_enabled DATABASE_AUTOMINORVERSIONUPGRADE; then
        local ARG_MINOR_VERSION_UPGRADE="--auto-minor-version-upgrade"
    else
        local ARG_MINOR_VERSION_UPGRADE="--no-auto-minor-version-upgrade"
    fi
    if option_enabled DATABASE_PERFORMANCEINSIGHTSENABLED; then
        local ARG_PERFORMANCE_INSIGHTS="--enable-performance-insights --performance-insights-retention-period ${DATABASE_PERFORMANCEINSIGHTSRETENTIONPERIOD}"
    else
        local ARG_PERFORMANCE_INSIGHTS="--no-enable-performance-insights"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"
    generate_temp_file FILE_TAGS_JSON "tags json"

    log "${FUNCTION_DESCRIPTION}: generating tags JSON"
cat > ${FILE_TAGS_JSON} << ZZEOF
[
  { "Key": "Organization", "Value": "${DATABASE_ORGANIZATIONNAME}"},
  { "Key": "Project", "Value": "${DATABASE_PROJECTNAME}"},
  { "Key": "Function", "Value": "${DATABASE_FUNCTIONNAME}"},
  { "Key": "Environment", "Value": "${DATABASE_ENVIRONMENT}"},
  { "Key": "Owner", "Value": "${DATABASE_OWNER}"},
  { "Key": "Contact", "Value": "${DATABASE_CONTACT}"},
  { "Key": "Cluster", "Value": "${DATABASE_CLUSTERID}"}
]
ZZEOF

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

        log "${FUNCTION_DESCRIPTION}: creating database (Attempt::${COUNTER} of ${AWS_RDS_DEFAULT_RETRY_COUNT})"
        $(which aws) ${AWS_REGION_STRING} rds create-db-instance \
            ${ARG_DATABASE_INSTANCE_IDENTIFIER} \
            ${ARG_CLUSTER_INSTANCE_IDENTIFIER} \
            ${ARG_ENGINE} \
            ${ARG_DATABASE_INSTANCE_CLASS} \
            ${ARG_DATABASE_PARAMETER_GROUP_NAME} \
            ${ARG_MINOR_VERSION_UPGRADE} \
            ${ARG_PERFORMANCE_INSIGHTS} \
            --tags file://${FILE_TAGS_JSON}>${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
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

    call_sleep 5 "allowing database time to warmup"
    return 0
}

###------------------------------------------------------------------------------------------------
## FUNCTION: aurora_postgresql_create_from_snapshot()
## - Creates Aurora PostgreSQL Database from specified Snapshot
## - Arguments
##   - $1: Snapshot ARN
##   - $2: Verification Timeout <in minutes, defaults to 30 minutes>
##   - $3: Region
function aurora_postgresql_create_from_snapshot() {
    local FUNCTION_DESCRIPTION="Aurora PostgreSQL (Create from Snapshot)"
    local TMP_SNAPSHOT_ARN="${1}"
    local AWS_OPERATION_TIMEOUT="${2}"
    local TMP_AWS_REGION="${3}"

    local RETURNVAL=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    local TMP_CURRENT_ENGINE_VERSION=""
    local TMP_CURRENT_PARAMETER_GROUP=""

    if(is_empty "${TMP_SNAPSHOT_ARN}"); then
        log_error "${FUNCTION_DESCRIPTION}: Snapshot ARN not specified"
        return $E_BAD_ARGS
    fi

    verify_array_key_values "DATABASE_VARIABLES[@]" "DATABASE"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "${FUNCTION_DESCRIPTION}: Failed to load required variables [Aurora PostgreSQL]"
        return $RETURNVAL
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    local ARG_DATABASE_INSTANCE_IDENTIFIER="--db-instance-identifier ${DATABASE_DBID}"
    local ARG_DATABASE_SNAPSHOT_IDENTIFIER="--db-snapshot-identifier ${TMP_SNAPSHOT_ARN}"
    local ARG_ENGINE="--engine postgres"
    local ARG_DATABASE_INSTANCE_CLASS="--db-instance-class ${DATABASE_DATABASEINSTANCECLASS}"
    local ARG_DATABASE_SUBNET_GROUP_NAME="--db-subnet-group-name ${DATABASE_DATABASESUBNETGROUPID}"
    local ARG_PORT="--port ${DATABASE_PORT}"
    if option_enabled DATABASE_MULTIAZ; then
        local ARG_MULTI_AZ="--multi-az"
    else
        local ARG_MULTI_AZ="--no-multi-az"
    fi
    local ARG_STORAGE_TYPE="--storage-type ${DATABASE_DBSTORAGETYPE}"
    if [ ${DATABASE_IOPS} -gt 0 ]; then
        local ARG_IOPS="--iops ${DATABASE_IOPS}"
    else
        local ARG_IOPS=""
    fi
    if [[ "ZZ_$(to_upper "${DATABASE_SUBNETTYPE}")" == "ZZ_PUBLIC" ]]; then
        local ARG_ACCESSIBILITY="--publicly-accessible"
    else
        local ARG_ACCESSIBILITY="--no-publicly-accessible"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"
    generate_temp_file FILE_TAGS_JSON "tags json"

    log_notice "${FUNCTION_DESCRIPTION}: started (database_id: [${DATABASE_DBID}] / snapshot_arn: [${TMP_SNAPSHOT_ARN}])"

    log "${FUNCTION_DESCRIPTION}: Generating Tags JSON"
cat > ${FILE_TAGS_JSON} << ZZEOF
[
  { "Key": "Organization", "Value": "${DATABASE_ORGANIZATIONNAME}"},
  { "Key": "Project", "Value": "${DATABASE_PROJECTNAME}"},
  { "Key": "Function", "Value": "${DATABASE_FUNCTIONNAME}"},
  { "Key": "Environment", "Value": "${DATABASE_ENVIRONMENT}"},
  { "Key": "Owner", "Value": "${DATABASE_OWNER}"},
  { "Key": "Contact", "Value": "${DATABASE_CONTACT}"},
  { "Key": "DNS", "Value": "${DATABASE_POSTGRESQLDNSRECORD}"}
]
ZZEOF

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

        log "${FUNCTION_DESCRIPTION}: creating database from snapshot (Attempt::${COUNTER} of ${AWS_RDS_DEFAULT_RETRY_COUNT})"
        $(which aws) ${AWS_REGION_STRING} rds restore-db-instance-from-db-snapshot \
            ${ARG_DATABASE_INSTANCE_IDENTIFIER} \
            ${ARG_DATABASE_SNAPSHOT_IDENTIFIER} \
            ${ARG_ENGINE} \
            ${ARG_DATABASE_INSTANCE_CLASS} \
            ${ARG_DATABASE_SUBNET_GROUP_NAME} \
            ${ARG_PORT} \
            ${ARG_MULTI_AZ} \
            ${ARG_STORAGE_TYPE} \
            ${ARG_IOPS} \
            ${ARG_ACCESSIBILITY} \
            --tags file://${FILE_TAGS_JSON}>${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
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

    call_sleep 60 "allowing Aurora PostgreSQL Database time to warmup"
    line_break

    log "- Waiting for Aurora PostgreSQL Database to become available"
    rds_poll_status "${DATABASE_DBID}" yes "${AWS_OPERATION_TIMEOUT}" "" "${TMP_AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "- Aurora PostgreSQL Database failed to become available, cannot continue"
        return $RETURNVAL
    else
        log "- Aurora PostgreSQL Database is now available"
    fi
    line_break

    log_notice "${FUNCTION_DESCRIPTION}: Checking Engine Version"
    rds_get_engine_version TMP_CURRENT_ENGINE_VERSION "${DATABASE_DBID}" "${TMP_AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "- Aurora PostgreSQL Database failed to retrieve engine version, cannot continue"
        return $RETURNVAL
    fi
    if [[ "ZZ_${TMP_CURRENT_ENGINE_VERSION}" != "ZZ_${DATABASE_ENGINEVERSION}" ]]; then
        log "- Engine Version is not on specified version, requires modification"
        log "- Configuring database to allow Engine Version modification"
        rds_set_maintenance_policy "${DATABASE_DBID}" "${DATABASE_PREFERREDMAINTENANCEWINDOW}" "${DATABASE_AUTOMINORVERSIONUPGRADE}" yes yes "${TMP_AWS_REGION}"
        RETURNVAL="$?"
        if [ ${RETURNVAL} -ne 0 ]; then
            log_error "- failed to set maintenance policy, cannot continue"
            return $RETURNVAL
        fi
        log "- Configuring Engine Version"
        rds_set_engine_version "${DATABASE_DBID}" "${DATABASE_ENGINEVERSION}" "${DATABASE_PARAMETERGROUPNAME}" yes "${TMP_AWS_REGION}"
        RETURNVAL="$?"
        if [ ${RETURNVAL} -ne 0 ]; then
            log_error "- failed to set engine version, cannot continue"
            return $RETURNVAL
        fi
    else
        log "- Engine Version is on specified version, no need to modify"
    fi
    line_break

    log_notice "${FUNCTION_DESCRIPTION}: Checking Parameter Group"
    rds_get_parameter_group TMP_CURRENT_PARAMETER_GROUP "${DATABASE_DBID}" "${TMP_AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "- Aurora PostgreSQL Database failed to retrieve parameter group, cannot continue"
        return $RETURNVAL
    fi
    if [[ "ZZ_${TMP_CURRENT_PARAMETER_GROUP}" != "ZZ_${DATABASE_PARAMETERGROUPNAME}" ]]; then
        log "- Parameter Group is not on specified parameter group, requires modification"
        log "- Configuring Parameter Group"
        rds_set_parameter_group "${DATABASE_DBID}" "${DATABASE_PARAMETERGROUPNAME}" no "${TMP_AWS_REGION}"
        RETURNVAL="$?"
        if [ ${RETURNVAL} -ne 0 ]; then
            log_error "- failed to set parameter group, cannot continue"
            return $RETURNVAL
        fi
        call_sleep 15 "allowing Aurora PostgreSQL Database time to apply logic"
        log "- Waiting for Aurora PostgreSQL Database to become available"
        rds_poll_status "${DATABASE_DBID}" yes "${AWS_OPERATION_TIMEOUT}" "" "${TMP_AWS_REGION}"
        RETURNVAL="$?"
        if [ ${RETURNVAL} -ne 0 ]; then
            log_error "- Aurora PostgreSQL Database failed to become available, cannot continue"
            return $RETURNVAL
        else
            log "- Aurora PostgreSQL Database is now available"
        fi
    else
        log "- Parameter Group is on specified parameter group, no need to modify"
    fi
    line_break

    log_notice "${FUNCTION_DESCRIPTION}: Setting Security Group"
    rds_set_security_group "${DATABASE_DBID}" "${DATABASE_DATABASESECURITYGROUPID}" yes "${TMP_AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "- failed to set security group, cannot continue"
        return $RETURNVAL
    fi
    call_sleep 15 "allowing Aurora PostgreSQL Database time to apply logic"
    log "- Waiting for Aurora PostgreSQL Database to become available"
    rds_poll_status "${DATABASE_DBID}" yes "${AWS_OPERATION_TIMEOUT}" "" "${TMP_AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "- Aurora PostgreSQL Database failed to become available, cannot continue"
        return $RETURNVAL
    else
        log "- Aurora PostgreSQL Database is now available"
    fi
    line_break

    log_notice "${FUNCTION_DESCRIPTION}: Setting Maintenance Policy"
    rds_set_maintenance_policy "${DATABASE_DBID}" "${DATABASE_PREFERREDMAINTENANCEWINDOW}" "${DATABASE_AUTOMINORVERSIONUPGRADE}" no no "${TMP_AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "- failed to set maintenance policy, cannot continue"
        return $RETURNVAL
    fi
    call_sleep 15 "allowing Aurora PostgreSQL Database time to apply logic"
    log "- Waiting for Aurora PostgreSQL Database to become available"
    rds_poll_status "${DATABASE_DBID}" yes "${AWS_OPERATION_TIMEOUT}" "" "${TMP_AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "- Aurora PostgreSQL Database failed to become available, cannot continue"
        return $RETURNVAL
    else
        log "- Aurora PostgreSQL Database is now available"
    fi
    line_break

    log_notice "${FUNCTION_DESCRIPTION}: Setting Backup Policy"
    rds_set_backup_policy "${DATABASE_DBID}" "${DATABASE_BACKUPRETENTIONLIMIT}" "${DATABASE_BACKUPWINDOW}" no "${TMP_AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "- failed to set backup policy, cannot continue"
        return $RETURNVAL
    fi
    call_sleep 15 "allowing Aurora PostgreSQL Database time to apply logic"
    log "- Waiting for Aurora PostgreSQL Database to become available"
    rds_poll_status "${DATABASE_DBID}" yes "${AWS_OPERATION_TIMEOUT}" "" "${TMP_AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "- Aurora PostgreSQL Database failed to become available, cannot continue"
        return $RETURNVAL
    else
        log "- Aurora PostgreSQL Database is now available"
    fi
    line_break

    log_notice "${FUNCTION_DESCRIPTION}: Setting Password"
    rds_update_password "${DATABASE_DBID}" "${DATABASE_MASTERPASSWORD}" 30 "${TMP_AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "- failed to set password, cannot continue"
        return $RETURNVAL
    fi
    call_sleep 30 "allowing Aurora PostgreSQL Database time to apply logic"
    log "- Waiting for Aurora PostgreSQL Database to become available"
    rds_poll_status "${DATABASE_DBID}" yes "${AWS_OPERATION_TIMEOUT}" "" "${TMP_AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "- Aurora PostgreSQL Database failed to become available, cannot continue"
        return $RETURNVAL
    else
        log "- Aurora PostgreSQL Database is now available"
    fi

    log_notice "- Rebooting Database"
    rds_reboot "${DATABASE_DBID}" no 25 "${TMP_AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "- Aurora PostgreSQL Database failed to become available, cannot continue"
        return $RETURNVAL
    fi
    call_sleep 30 "allowing Aurora PostgreSQL Database time to apply logic"
    line_break

    log_notice "${FUNCTION_DESCRIPTION}: Checking if Database requires an additional reboot to finalize settings"
    if (rds_is_DATABASE_pending_reboot "${DATABASE_DBID}" "${TMP_AWS_REGION}"); then
        log_notice "- Aurora PostgreSQL Database requires reboot"
        rds_reboot "${DATABASE_DBID}" no 25 "${TMP_AWS_REGION}"
        RETURNVAL="$?"
        if [ ${RETURNVAL} -ne 0 ]; then
            log_error "- Aurora PostgreSQL Database failed to become available, cannot continue"
            return $RETURNVAL
        fi
    fi
    line_break

    log "- Waiting for Aurora PostgreSQL Database to become available"
    rds_poll_status "${DATABASE_DBID}" yes "${AWS_OPERATION_TIMEOUT}" "" "${TMP_AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "- Aurora PostgreSQL Database failed to become available, cannot continue"
        return $RETURNVAL
    else
        log "- Aurora PostgreSQL Database is now available"
    fi
    log "${FUNCTION_DESCRIPTION}: successfully created and configured database"
    line_break
    return 0
}

###------------------------------------------------------------------------------------------------
## FUNCTION: aurora_postgresql_execute_sql()
## - Attempts to connect to specified Aurora PostgreSQL Database in order to execute SQL file
## - Arguments
##   - $1: Database Endpoint
##   - $2: Username
##   - $3: Password
##   - $4: Database Name (optional, defaults to 'postgres')
##   - $5: Database Port (optional, if not specified, defaults to DATABASE_DEFAULT_PORT)
##   - $6: SQL File
function aurora_postgresql_execute_sql() {
    local FUNCTION_DESCRIPTION="Aurora PostgreSQL (Execute SQL)"
    local TMP_DATABASE_ENDPOINT="${1}"
    local TMP_DATABASE_USERNAME="${2}"
    local TMP_DATABASE_PASSWORD="${3}"
    local TMP_DATABASE_NAME="${4}"
    local TMP_DATABASE_PORT="${5}"
    local TMP_FILE_SQL="${6}"

    local RETURNVAL=""
    local FILE_DATABASE_OUTPUT=""

    if(is_empty "${TMP_DATABASE_ENDPOINT}"); then
        log_error "${FUNCTION_DESCRIPTION}: Database Endpoint not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_DATABASE_USERNAME}"); then
        log_error "${FUNCTION_DESCRIPTION}: Username not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_DATABASE_PASSWORD}"); then
        log_error "${FUNCTION_DESCRIPTION}: Password not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_FILE_SQL}"); then
        log_error "${FUNCTION_DESCRIPTION}: SQL file specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_DATABASE_NAME}"); then
        TMP_DATABASE_NAME="postgres"
    fi
    if(is_empty "${TMP_DATABASE_PORT}"); then
        TMP_DATABASE_PORT="${DATABASE_DEFAULT_PORT}"
    fi
    if [ ! -f "${TMP_FILE_SQL}" ]; then
        log_error "${FUNCTION_DESCRIPTION}: SQL file does not exist [${TMP_FILE_SQL}]"
        return $E_OBJECT_NOT_FOUND
    fi

    generate_temp_file FILE_DATABASE_OUTPUT "postgresql output"

    log "${FUNCTION_DESCRIPTION}: started"
    log "- Endpoint: [${TMP_DATABASE_ENDPOINT}]"
    log "- Username: [${TMP_DATABASE_USERNAME}]"
    log "- Password: [${TMP_DATABASE_PASSWORD//?/*}]"
    log "- Database: [${TMP_DATABASE_NAME}]"
    log "- SQL File: [${TMP_FILE_SQL}]"
    PGPASSWORD=${TMP_DATABASE_PASSWORD} $(which psql) -h${TMP_DATABASE_ENDPOINT} -p${TMP_DATABASE_PORT} -U${TMP_DATABASE_USERNAME} ${TMP_DATABASE_NAME} -f "${TMP_FILE_SQL}" >${FILE_DATABASE_OUTPUT} 2>&1
    RETURNVAL="$?"
    PGPASSWORD=""
    if [ ${RETURNVAL} -ne 0 ]; then
        log_add_from_file "${FILE_DATABASE_OUTPUT}" "${FUNCTION_DESCRIPTION}: postgresql output"
        log_error "${FUNCTION_DESCRIPTION}: failed to execute SQL file (psql_exit_code::${RETURNVAL}])"
        return $E_DATABASE_EXECUTION_FAILURE
    fi
    log_add_from_file "${FILE_DATABASE_OUTPUT}" "${FUNCTION_DESCRIPTION}: postgresql output"
    log "${FUNCTION_DESCRIPTION}: successfully executed SQL file"
    sleep 0.1
    return 0
}

###------------------------------------------------------------------------------------------------
## FUNCTION: aurora_postgresql_set_dns()
## - Sets DNS records (Read & Write) for Aurora PostgreSQL Database
## - Arguments
##   - $1: Region
function aurora_postgresql_set_dns() {
    local FUNCTION_DESCRIPTION="Aurora PostgreSQL (Set DNS)"
    local TMP_AWS_REGION="${1}"

    local RETURNVAL=""
    local TMP_KEY=""
    local TMP_VAR=""
    local KEY_MAX_LENGTH=0


    verify_array_key_values "DATABASE_DNS_VARIABLES[@]" "DATABASE"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "${FUNCTION_DESCRIPTION}: Failed to load required variables [DNS]"
        return $RETURNVAL
    fi

    verify_array_key_values "DATABASE_ENDPOINT_VARIABLES[@]" "DATABASE"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "${FUNCTION_DESCRIPTION}: Failed to load required variables [Endpoints]"
        return $RETURNVAL
    fi

    log_notice "${FUNCTION_DESCRIPTION}: started"

    log "${FUNCTION_DESCRIPTION}: setting DNS record (write)"
    log "- DNS Record:       [${DATABASE_DNSDATABASERECORDWRITE}]"
    log "- DNS Zone ID:      [${DATABASE_DNSDATABASEHOSTEDZONEID}]"
    log "- Endpoint Address: [${DATABASE_ENDPOINT_WRITE}]"
    route53_upsert_record "${DATABASE_DNSDATABASEHOSTEDZONEID}" "${DATABASE_DNSDATABASERECORDWRITE}" "CNAME" ${AWS_RDS_DNS_TTL} "${DATABASE_ENDPOINT_WRITE}" "" "" "${TMP_AWS_REGION}"
    RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then return ${RETURNVAL}; fi

    log "${FUNCTION_DESCRIPTION}: setting DNS record (read)"
    log "- DNS Record:       [${DATABASE_DNSDATABASERECORDREAD}]"
    log "- DNS Zone ID:      [${DATABASE_DNSDATABASEHOSTEDZONEID}]"
    log "- Endpoint Address: [${DATABASE_ENDPOINT_READ}]"
    route53_upsert_record "${DATABASE_DNSDATABASEHOSTEDZONEID}" "${DATABASE_DNSDATABASERECORDREAD}" "CNAME" ${AWS_RDS_DNS_TTL} "${DATABASE_ENDPOINT_READ}" "" "" "${TMP_AWS_REGION}"
    RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then return ${RETURNVAL}; fi

    return 0
}

###------------------------------------------------------------------------------------------------
## FUNCTION: aurora_postgresql_verify_connection()
## - Attempts to connect to specified Aurora PostgreSQL Database in order to verify if database is available and credentials are correct
## - Arguments
##   - $1: Database Endpoint
##   - $2: Username
##   - $3: Password
##   - $4: Database Name (optional, defaults to 'postgres')
##   - $5: Database Port (optional, if not specified, defaults to DATABASE_DEFAULT_PORT)
function aurora_postgresql_verify_connection() {
    local FUNCTION_DESCRIPTION="Aurora PostgreSQL (Verify Connection)"
    local TMP_DATABASE_ENDPOINT="${1}"
    local TMP_DATABASE_USERNAME="${2}"
    local TMP_DATABASE_PASSWORD="${3}"
    local TMP_DATABASE_NAME="${4}"
    local TMP_DATABASE_PORT="${5}"

    local RETURNVAL=""
    local FILE_DATABASE_OUTPUT=""

    if(is_empty "${TMP_DATABASE_ENDPOINT}"); then
        log_error "${FUNCTION_DESCRIPTION}: Database Endpoint not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_DATABASE_USERNAME}"); then
        log_error "${FUNCTION_DESCRIPTION}: Username not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_DATABASE_PASSWORD}"); then
        log_error "${FUNCTION_DESCRIPTION}: Password not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_DATABASE_NAME}"); then
        TMP_DATABASE_NAME="postgres"
    fi
    if(is_empty "${TMP_DATABASE_PORT}"); then
        TMP_DATABASE_PORT="${DATABASE_DEFAULT_PORT}"
    fi

    generate_temp_file FILE_DATABASE_OUTPUT "postgresql output"

    PGPASSWORD=${TMP_DATABASE_PASSWORD} $(which psql) -h${TMP_DATABASE_ENDPOINT} -p${TMP_DATABASE_PORT} -U${TMP_DATABASE_USERNAME} ${TMP_DATABASE_NAME}  -c '\l' >${FILE_DATABASE_OUTPUT} 2>&1
    RETURNVAL="$?"
    PGPASSWORD=""
    if [ ${RETURNVAL} -ne 0 ]; then
        log "${FUNCTION_DESCRIPTION}: Database [${TMP_DATABASE_ENDPOINT}] - [$(color_text "${LOG_COLOR_ERROR}" "FAILED")] (psql_exit_code::${RETURNVAL}])"
        log_add_from_file "${FILE_DATABASE_OUTPUT}" "${FUNCTION_DESCRIPTION}: postgresql output"
        return $E_DATABASE_CONNECTION_FAILURE
    fi

    log "${FUNCTION_DESCRIPTION}: Database [${TMP_DATABASE_ENDPOINT}] - [$(color_text "${LOG_COLOR_SUCCESS}" "OK")]"
    return 0
}
