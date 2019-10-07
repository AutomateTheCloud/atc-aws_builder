###------------------------------------------------------------------------------------------------
# script:  rds_postgresql.inc
# purpose: Collection of functions related to AWS (RDS - PostgreSQL)
# version: 1.0.0
#
# function list:
#          - load_info_postgresql
#          - postgresql_create
#          - postgresql_create_from_snapshot
#          - postgresql_create_replica
#          - postgresql_execute_sql
#          - postgresql_set_dns
#          - postgresql_verify_connection
###------------------------------------------------------------------------------------------------
REQUIRED_EXECUTABLES+=('aws' 'psql')

###------------------------------------------------------------------------------------------------
# Variables
POSTGRESQL_DEFAULT_PORT="5432"

POSTGRESQL_VARIABLES+=(
    'PostgreSQLReferenceName'
    'PostgreSQLDBId'
    'DBInstanceClass'
    'EngineVersion'
    'ReplicaCount'
    'ParameterGroupName'
    'OptionGroupName'
    'DBName'

    'AllocatedStorage'
    'DBStorageType'
    'IOPS'
    'EncryptionEnabled'

    'SubnetType'
    'MultiAZ'
    'PostgreSQLPort'
    'DBSubnetGroupId'
    'PostgreSQLSecurityGroupId'

    'AutoMinorVersionUpgrade'
    'PreferredMaintenanceWindow'

    'BackupRetentionLimit'
    'BackupWindow'

    'DBSubnetGroupId'
    'DNSDatabaseDomain'
    'DNSDatabaseHostedZoneID'
    'PostgreSQLDNSRecord'
    'PostgreSQLReplicaDNSRecord'

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

POSTGRESQL_REPLICA_VARIABLES+=(
    'ReplicaDBInstanceClass'
    'ReplicaParameterGroupName'
    'ReplicaOptionGroupName'
    'ReplicaDBStorageType'
    'ReplicaIOPS'
)

POSTGRESQL_DNS_VARIABLES+=(
    'PostgreSQLDNSRecord'
    'PostgreSQLReplicaDNSRecord'
    'DNSDatabaseHostedZoneID'
)

RDS_REPLICA_DB_IDS=()

POSTGRESQL_POSTGRESQLREFERENCENAME=""
POSTGRESQL_DBINSTANCECLASS=""
POSTGRESQL_ENGINEVERSION=""
POSTGRESQL_PARAMETERGROUPNAME=""
POSTGRESQL_OPTIONGROUPNAME=""
POSTGRESQL_DBNAME=""
POSTGRESQL_ALLOCATEDSTORAGE=""
POSTGRESQL_DBSTORAGETYPE=""
POSTGRESQL_IOPS=""
POSTGRESQL_ENCRYPTIONENABLED=""
POSTGRESQL_SUBNETTYPE=""
POSTGRESQL_MULTIAZ=""
POSTGRESQL_POSTGRESQLPORT=""
POSTGRESQL_DBSUBNETGROUPID=""
POSTGRESQL_POSTGRESQLSECURITYGROUPID=""
POSTGRESQL_AUTOMINORVERSIONUPGRADE=""
POSTGRESQL_PREFERREDMAINTENANCEWINDOW=""
POSTGRESQL_BACKUPRETENTIONLIMIT=""
POSTGRESQL_BACKUPWINDOW=""
POSTGRESQL_DNSDATABASEDOMAIN=""
POSTGRESQL_DNSDATABASEHOSTEDZONEID=""
POSTGRESQL_DNSDATABASESUBDOMAIN=""
POSTGRESQL_POSTGRESQLDNSRECORD=""
POSTGRESQL_POSTGRESQLREPLICADNSRECORD=""
POSTGRESQL_ACCOUNTNAME=""
POSTGRESQL_ACCOUNTABBR=""
POSTGRESQL_ACCOUNTNUMBER=""
POSTGRESQL_ORGANIZATIONNAME=""
POSTGRESQL_ORGANIZATIONABBR=""
POSTGRESQL_ENVIRONMENT=""
POSTGRESQL_PROJECTNAME=""
POSTGRESQL_PROJECTABBR=""
POSTGRESQL_FUNCTIONNAME=""
POSTGRESQL_FUNCTIONABBR=""
POSTGRESQL_OWNER=""
POSTGRESQL_CONTACT=""
POSTGRESQL_REGION=""

POSTGRESQL_REPLICACOUNT=""
POSTGRESQL_REPLICADBINSTANCECLASS=""
POSTGRESQL_REPLICAPARAMETERGROUPNAME=""
POSTGRESQL_REPLICAOPTIONGROUPNAME=""
POSTGRESQL_REPLICADBSTORAGETYPE=""
POSTGRESQL_REPLICAIOPS=""

POSTGRESQL_DBENDPOINT=""
POSTGRESQL_DBREPLICAENDPOINT=""
POSTGRESQL_POSTGRESQLDBID=""
POSTGRESQL_MASTERUSERNAME=""
POSTGRESQL_MASTERPASSWORD=""

###------------------------------------------------------------------------------------------------
## FUNCTION: load_info_postgresql()
## - Loads PostgreSQL Information into memory from CloudFormation Outputs file
## - Arguments
##   - $1: CloudFormation Outputs File
##   - $2: Region
function load_info_postgresql() {
    local FUNCTION_DESCRIPTION="Load Info (PostgreSQL)"
    local TMP_FILE_CLOUDFORMATION_OUTPUTS="${1}"
    local TMP_AWS_REGION="${2}"

    local TMP_UUID=""
    local TMP_KEY=""
    local TMP_VAR=""
    local TMP_PARAMETER=""
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
    load_array_properties_from_file "POSTGRESQL_VARIABLES[@]" "${TMP_FILE_CLOUDFORMATION_OUTPUTS}" "POSTGRESQL"
    if(is_empty "${POSTGRESQL_IOPS}"); then POSTGRESQL_IOPS=0; fi
    if(is_empty "${POSTGRESQL_ENCRYPTIONENABLED}"); then POSTGRESQL_ENCRYPTIONENABLED=no; fi
    if(is_empty "${POSTGRESQL_REPLICACOUNT}"); then POSTGRESQL_REPLICACOUNT=0; fi

    verify_array_key_values "PARAMETER_VARIABLES[@]" "POSTGRESQL"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "${FUNCTION_DESCRIPTION}: Failed to load required variables [Parameter Related]"
        return $RETURNVAL
    fi

    for TMP_KEY in "${CREDENTIALS_VARIABLES[@]}"; do
        TMP_VAR="$(to_upper "POSTGRESQL_${TMP_KEY}")"
        TMP_PARAMETER="/secrets/${POSTGRESQL_PROJECTABBR}/${POSTGRESQL_FUNCTIONABBR}/${POSTGRESQL_ENVIRONMENT}/vars/${TMP_KEY}"
        parameter_get_silent ${TMP_VAR} "${TMP_PARAMETER}" yes "${TMP_AWS_REGION}"
    done
    verify_array_key_values "CREDENTIALS_VARIABLES[@]" "POSTGRESQL"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "${FUNCTION_DESCRIPTION}: Failed to load required variables [Credentials]"
        return $RETURNVAL
    fi

    generate_uuid TMP_UUID 2
    POSTGRESQL_POSTGRESQLDBID="${POSTGRESQL_POSTGRESQLREFERENCENAME}-${TMP_UUID}"

    verify_array_key_values "POSTGRESQL_VARIABLES[@]" "POSTGRESQL"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "${FUNCTION_DESCRIPTION}: Failed to load required variables [PostgreSQL]"
        return $RETURNVAL
    fi

    if [[ "ZZ_$(to_upper "${POSTGRESQL_DBSTORAGETYPE}")" == "ZZ_IO1" ]]; then
        if [ ${POSTGRESQL_IOPS} -eq 0 ]; then
            log_error "${FUNCTION_DESCRIPTION}: Storage Type [io1] selected, IOPS must be greater than Zero [PostgreSQL]"
            return $E_BAD_ARGS
        fi
    fi

    if(! is_int "${POSTGRESQL_REPLICACOUNT}"); then
        log_error "${FUNCTION_DESCRIPTION}: Replica Count is not an integer [${POSTGRESQL_REPLICACOUNT}]"
        return $E_BAD_ARGS
    fi

    if [ ${POSTGRESQL_REPLICACOUNT} -gt 0 ]; then
        load_array_properties_from_file "POSTGRESQL_REPLICA_VARIABLES[@]" "${TMP_FILE_CLOUDFORMATION_OUTPUTS}" "POSTGRESQL"
        if(is_empty "${POSTGRESQL_REPLICAIOPS}"); then POSTGRESQL_REPLICAIOPS=0; fi
        verify_array_key_values "POSTGRESQL_REPLICA_VARIABLES[@]" "POSTGRESQL"
        RETURNVAL="$?"
        if [ ${RETURNVAL} -ne 0 ]; then
            log_error "${FUNCTION_DESCRIPTION}: Failed to load required variables [PostgreSQL Replica]"
            return $RETURNVAL
        fi
        if [ ${POSTGRESQL_BACKUPRETENTIONLIMIT} -eq 0 ]; then
            log_error "${FUNCTION_DESCRIPTION}: In order to have replicas, BackupRetentionLimit must be greater than 0 "
            return $E_BAD_ARGS
        fi
        if [[ "ZZ_$(to_upper "${POSTGRESQL_REPLICADBSTORAGETYPE}")" == "ZZ_IO1" ]]; then
            if [ ${POSTGRESQL_REPLICAIOPS} -eq 0 ]; then
                log_error "${FUNCTION_DESCRIPTION}: Replica Storage Type [io1] selected, IOPS must be greater than Zero [PostgreSQL Replica]"
                return $E_BAD_ARGS
            fi
        fi
    fi

    line_break
    log_highlight "PostgreSQL Information"
    for TMP_KEY in "${POSTGRESQL_VARIABLES[@]}"; do
        if [ ${#TMP_KEY} -gt $KEY_MAX_LENGTH ]; then
            KEY_MAX_LENGTH=${#TMP_KEY}
        fi
    done

    if [ ${POSTGRESQL_REPLICACOUNT} -gt 0 ]; then
        for TMP_KEY in "${POSTGRESQL_REPLICA_VARIABLES[@]}"; do
            if [ ${#TMP_KEY} -gt $KEY_MAX_LENGTH ]; then
                KEY_MAX_LENGTH=${#TMP_KEY}
            fi
        done
    fi
    KEY_MAX_LENGTH=$((${KEY_MAX_LENGTH}+1))

    for TMP_KEY in "${POSTGRESQL_VARIABLES[@]}"; do
        TMP_VAR="$(to_upper "POSTGRESQL_${TMP_KEY}")"
        log "$(printf "%-1s %-${KEY_MAX_LENGTH}s %s" "-" "${TMP_KEY}:" "[${!TMP_VAR}]")"
    done
    if [ ${POSTGRESQL_REPLICACOUNT} -gt 0 ]; then
        for TMP_KEY in "${POSTGRESQL_REPLICA_VARIABLES[@]}"; do
            TMP_VAR="$(to_upper "POSTGRESQL_${TMP_KEY}")"
            log "$(printf "%-1s %-${KEY_MAX_LENGTH}s %s" "-" "${TMP_KEY}:" "[${!TMP_VAR}]")"
        done
    fi
    log "$(printf "%-1s %-${KEY_MAX_LENGTH}s %s" "-" "MasterUsername:" "[${POSTGRESQL_MASTERUSERNAME}]")"
    log "$(printf "%-1s %-${KEY_MAX_LENGTH}s %s" "-" "MasterPassword:" "[${POSTGRESQL_MASTERPASSWORD//?/*}]")"
    line_break
    return 0
}

###------------------------------------------------------------------------------------------------
## FUNCTION: postgresql_create()
## - Creates PostgreSQL Database
## - Arguments
##   - $1: Verification Timeout <in minutes, defaults to 30 minutes>
##   - $2: Region
function postgresql_create() {
    local FUNCTION_DESCRIPTION="PostgreSQL (Create)"
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

    verify_array_key_values "POSTGRESQL_VARIABLES[@]" "POSTGRESQL"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "${FUNCTION_DESCRIPTION}: Failed to load required variables [PostgreSQL]"
        return $RETURNVAL
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    local ARG_DB_INSTANCE_IDENTIFIER="--db-instance-identifier ${POSTGRESQL_POSTGRESQLDBID}"
    local ARG_DB_NAME="--db-name ${POSTGRESQL_DBNAME}"
    local ARG_MASTER_USERNAME="--master-username ${POSTGRESQL_MASTERUSERNAME}"
    local ARG_MASTER_USER_PASSWORD="--master-user-password ${POSTGRESQL_MASTERPASSWORD}"
    local ARG_ENGINE="--engine postgres"
    local ARG_ENGINE_VERSION="--engine-version ${POSTGRESQL_ENGINEVERSION}"
    local ARG_DB_INSTANCE_CLASS="--db-instance-class ${POSTGRESQL_DBINSTANCECLASS}"
    local ARG_VPC_SECURITY_GROUP_IDS="--vpc-security-group-ids ${POSTGRESQL_POSTGRESQLSECURITYGROUPID}"
    local ARG_DB_SUBNET_GROUP_NAME="--db-subnet-group-name ${POSTGRESQL_DBSUBNETGROUPID}"
    local ARG_PORT="--port ${POSTGRESQL_POSTGRESQLPORT}"
    local ARG_DB_PARAMETER_GROUP_NAME="--db-parameter-group-name ${POSTGRESQL_PARAMETERGROUPNAME}"
    local ARG_OPTION_GROUP_NAME="--option-group-name ${POSTGRESQL_OPTIONGROUPNAME}"
    local ARG_BACKUP_RETENTION_PERIOD="--backup-retention-period ${POSTGRESQL_BACKUPRETENTIONLIMIT}"
    if [ ${POSTGRESQL_BACKUPRETENTIONLIMIT} -gt 0 ]; then
        local ARG_PREFERRED_BACKUP_WINDOW="--preferred-backup-window ${POSTGRESQL_BACKUPWINDOW} --copy-tags-to-snapshot"
    else
        local ARG_PREFERRED_BACKUP_WINDOW="--copy-tags-to-snapshot"
    fi
    local ARG_PREFERRED_MAINTENANCE_WINDOW="--preferred-maintenance-window ${POSTGRESQL_PREFERREDMAINTENANCEWINDOW}"
    if option_enabled POSTGRESQL_AUTOMINORVERSIONUPGRADE; then
        local ARG_MINOR_VERSION_UPGRADE="--auto-minor-version-upgrade"
    else
        local ARG_MINOR_VERSION_UPGRADE="--no-auto-minor-version-upgrade"
    fi
    if option_enabled POSTGRESQL_MULTIAZ; then
        local ARG_MULTI_AZ="--multi-az"
    else
        local ARG_MULTI_AZ="--no-multi-az"
    fi
    local ARG_ALLOCATED_STORAGE="--allocated-storage ${POSTGRESQL_ALLOCATEDSTORAGE}"
    local ARG_STORAGE_TYPE="--storage-type ${POSTGRESQL_DBSTORAGETYPE}"
    if [ ${POSTGRESQL_IOPS} -gt 0 ]; then
        local ARG_IOPS="--iops ${POSTGRESQL_IOPS}"
    else
        local ARG_IOPS=""
    fi
    if option_enabled POSTGRESQL_ENCRYPTIONENABLED; then
        local ARG_ENCRYPTION="--storage-encrypted"
    else
        local ARG_ENCRYPTION="--no-storage-encrypted"
    fi
    if [[ "ZZ_$(to_upper "${POSTGRESQL_SUBNETTYPE}")" == "ZZ_PUBLIC" ]]; then
        local ARG_ACCESSIBILITY="--publicly-accessible"
    else
        local ARG_ACCESSIBILITY="--no-publicly-accessible"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"
    generate_temp_file FILE_TAGS_JSON "tags json"

    log_notice "${FUNCTION_DESCRIPTION}: started (database_id: [${POSTGRESQL_POSTGRESQLDBID}])"

    log "${FUNCTION_DESCRIPTION}: Generating Tags JSON"
cat > ${FILE_TAGS_JSON} << ZZEOF
[
  { "Key": "Organization", "Value": "${POSTGRESQL_ORGANIZATIONNAME}"},
  { "Key": "Project", "Value": "${POSTGRESQL_PROJECTNAME}"},
  { "Key": "Function", "Value": "${POSTGRESQL_FUNCTIONNAME}"},
  { "Key": "Environment", "Value": "${POSTGRESQL_ENVIRONMENT}"},
  { "Key": "Owner", "Value": "${POSTGRESQL_OWNER}"},
  { "Key": "Contact", "Value": "${POSTGRESQL_CONTACT}"},
  { "Key": "DNS", "Value": "${POSTGRESQL_POSTGRESQLDNSRECORD}"}
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

        log "${FUNCTION_DESCRIPTION}: Creating Database (Attempt::${COUNTER} of ${AWS_RDS_DEFAULT_RETRY_COUNT})"
        $(which aws) ${AWS_REGION_STRING} rds create-db-instance \
            ${ARG_DB_INSTANCE_IDENTIFIER} \
            ${ARG_DB_NAME} \
            ${ARG_MASTER_USERNAME} \
            ${ARG_MASTER_USER_PASSWORD} \
            ${ARG_ENGINE} \
            ${ARG_ENGINE_VERSION} \
            ${ARG_DB_INSTANCE_CLASS} \
            ${ARG_VPC_SECURITY_GROUP_IDS} \
            ${ARG_DB_SUBNET_GROUP_NAME} \
            ${ARG_PORT} \
            ${ARG_DB_PARAMETER_GROUP_NAME} \
            ${ARG_OPTION_GROUP_NAME} \
            ${ARG_BACKUP_RETENTION_PERIOD} \
            ${ARG_PREFERRED_BACKUP_WINDOW} \
            ${ARG_PREFERRED_MAINTENANCE_WINDOW} \
            ${ARG_MINOR_VERSION_UPGRADE} \
            ${ARG_MULTI_AZ} \
            ${ARG_ALLOCATED_STORAGE} \
            ${ARG_STORAGE_TYPE} \
            ${ARG_IOPS} \
            ${ARG_ENCRYPTION} \
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

    call_sleep 60 "allowing PostgreSQL Database time to warmup"
    line_break

    log "- Waiting for PostgreSQL Database to become available"
    rds_poll_status "${POSTGRESQL_POSTGRESQLDBID}" yes "${AWS_OPERATION_TIMEOUT}" "" "${TMP_AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "- PostgreSQL Database failed to become available, cannot continue"
        return $RETURNVAL
    else
        log "- PostgreSQL Database is now available"
    fi
    log "${FUNCTION_DESCRIPTION}: successfully created and configured database"
    line_break
    return 0
}

###------------------------------------------------------------------------------------------------
## FUNCTION: postgresql_create_from_snapshot()
## - Creates PostgreSQL Database from specified Snapshot
## - Arguments
##   - $1: Snapshot ARN
##   - $2: Verification Timeout <in minutes, defaults to 30 minutes>
##   - $3: Region
function postgresql_create_from_snapshot() {
    local FUNCTION_DESCRIPTION="PostgreSQL (Create from Snapshot)"
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

    verify_array_key_values "POSTGRESQL_VARIABLES[@]" "POSTGRESQL"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "${FUNCTION_DESCRIPTION}: Failed to load required variables [PostgreSQL]"
        return $RETURNVAL
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    local ARG_DB_INSTANCE_IDENTIFIER="--db-instance-identifier ${POSTGRESQL_POSTGRESQLDBID}"
    local ARG_DB_SNAPSHOT_IDENTIFIER="--db-snapshot-identifier ${TMP_SNAPSHOT_ARN}"
    local ARG_ENGINE="--engine postgres"
    local ARG_DB_INSTANCE_CLASS="--db-instance-class ${POSTGRESQL_DBINSTANCECLASS}"
    local ARG_DB_SUBNET_GROUP_NAME="--db-subnet-group-name ${POSTGRESQL_DBSUBNETGROUPID}"
    local ARG_PORT="--port ${POSTGRESQL_POSTGRESQLPORT}"
    local ARG_OPTION_GROUP_NAME="--option-group-name ${POSTGRESQL_OPTIONGROUPNAME}"
    if option_enabled POSTGRESQL_MULTIAZ; then
        local ARG_MULTI_AZ="--multi-az"
    else
        local ARG_MULTI_AZ="--no-multi-az"
    fi
    local ARG_STORAGE_TYPE="--storage-type ${POSTGRESQL_DBSTORAGETYPE}"
    if [ ${POSTGRESQL_IOPS} -gt 0 ]; then
        local ARG_IOPS="--iops ${POSTGRESQL_IOPS}"
    else
        local ARG_IOPS=""
    fi
    if [[ "ZZ_$(to_upper "${POSTGRESQL_SUBNETTYPE}")" == "ZZ_PUBLIC" ]]; then
        local ARG_ACCESSIBILITY="--publicly-accessible"
    else
        local ARG_ACCESSIBILITY="--no-publicly-accessible"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"
    generate_temp_file FILE_TAGS_JSON "tags json"

    log_notice "${FUNCTION_DESCRIPTION}: started (database_id: [${POSTGRESQL_POSTGRESQLDBID}] / snapshot_arn: [${TMP_SNAPSHOT_ARN}])"

    log "${FUNCTION_DESCRIPTION}: Generating Tags JSON"
cat > ${FILE_TAGS_JSON} << ZZEOF
[
  { "Key": "Organization", "Value": "${POSTGRESQL_ORGANIZATIONNAME}"},
  { "Key": "Project", "Value": "${POSTGRESQL_PROJECTNAME}"},
  { "Key": "Function", "Value": "${POSTGRESQL_FUNCTIONNAME}"},
  { "Key": "Environment", "Value": "${POSTGRESQL_ENVIRONMENT}"},
  { "Key": "Owner", "Value": "${POSTGRESQL_OWNER}"},
  { "Key": "Contact", "Value": "${POSTGRESQL_CONTACT}"},
  { "Key": "DNS", "Value": "${POSTGRESQL_POSTGRESQLDNSRECORD}"}
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
            ${ARG_DB_INSTANCE_IDENTIFIER} \
            ${ARG_DB_SNAPSHOT_IDENTIFIER} \
            ${ARG_ENGINE} \
            ${ARG_DB_INSTANCE_CLASS} \
            ${ARG_DB_SUBNET_GROUP_NAME} \
            ${ARG_PORT} \
            ${ARG_OPTION_GROUP_NAME} \
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

    call_sleep 60 "allowing PostgreSQL Database time to warmup"
    line_break

    log "- Waiting for PostgreSQL Database to become available"
    rds_poll_status "${POSTGRESQL_POSTGRESQLDBID}" yes "${AWS_OPERATION_TIMEOUT}" "" "${TMP_AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "- PostgreSQL Database failed to become available, cannot continue"
        return $RETURNVAL
    else
        log "- PostgreSQL Database is now available"
    fi
    line_break

    log_notice "${FUNCTION_DESCRIPTION}: Checking Engine Version"
    rds_get_engine_version TMP_CURRENT_ENGINE_VERSION "${POSTGRESQL_POSTGRESQLDBID}" "${TMP_AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "- PostgreSQL Database failed to retrieve engine version, cannot continue"
        return $RETURNVAL
    fi
    if [[ "ZZ_${TMP_CURRENT_ENGINE_VERSION}" != "ZZ_${POSTGRESQL_ENGINEVERSION}" ]]; then
        log "- Engine Version is not on specified version, requires modification"
        log "- Configuring database to allow Engine Version modification"
        rds_set_maintenance_policy "${POSTGRESQL_POSTGRESQLDBID}" "${POSTGRESQL_PREFERREDMAINTENANCEWINDOW}" "${POSTGRESQL_AUTOMINORVERSIONUPGRADE}" yes yes "${TMP_AWS_REGION}"
        RETURNVAL="$?"
        if [ ${RETURNVAL} -ne 0 ]; then
            log_error "- failed to set maintenance policy, cannot continue"
            return $RETURNVAL
        fi
        log "- Configuring Engine Version"
        rds_set_engine_version "${POSTGRESQL_POSTGRESQLDBID}" "${POSTGRESQL_ENGINEVERSION}" "${POSTGRESQL_PARAMETERGROUPNAME}" yes "${TMP_AWS_REGION}"
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
    rds_get_parameter_group TMP_CURRENT_PARAMETER_GROUP "${POSTGRESQL_POSTGRESQLDBID}" "${TMP_AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "- PostgreSQL Database failed to retrieve parameter group, cannot continue"
        return $RETURNVAL
    fi
    if [[ "ZZ_${TMP_CURRENT_PARAMETER_GROUP}" != "ZZ_${POSTGRESQL_PARAMETERGROUPNAME}" ]]; then
        log "- Parameter Group is not on specified parameter group, requires modification"
        log "- Configuring Parameter Group"
        rds_set_parameter_group "${POSTGRESQL_POSTGRESQLDBID}" "${POSTGRESQL_PARAMETERGROUPNAME}" no "${TMP_AWS_REGION}"
        RETURNVAL="$?"
        if [ ${RETURNVAL} -ne 0 ]; then
            log_error "- failed to set parameter group, cannot continue"
            return $RETURNVAL
        fi
        call_sleep 15 "allowing PostgreSQL Database time to apply logic"
        log "- Waiting for PostgreSQL Database to become available"
        rds_poll_status "${POSTGRESQL_POSTGRESQLDBID}" yes "${AWS_OPERATION_TIMEOUT}" "" "${TMP_AWS_REGION}"
        RETURNVAL="$?"
        if [ ${RETURNVAL} -ne 0 ]; then
            log_error "- PostgreSQL Database failed to become available, cannot continue"
            return $RETURNVAL
        else
            log "- PostgreSQL Database is now available"
        fi
    else
        log "- Parameter Group is on specified parameter group, no need to modify"
    fi
    line_break

    log_notice "${FUNCTION_DESCRIPTION}: Setting Security Group"
    rds_set_security_group "${POSTGRESQL_POSTGRESQLDBID}" "${POSTGRESQL_POSTGRESQLSECURITYGROUPID}" yes "${TMP_AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "- failed to set security group, cannot continue"
        return $RETURNVAL
    fi
    call_sleep 15 "allowing PostgreSQL Database time to apply logic"
    log "- Waiting for PostgreSQL Database to become available"
    rds_poll_status "${POSTGRESQL_POSTGRESQLDBID}" yes "${AWS_OPERATION_TIMEOUT}" "" "${TMP_AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "- PostgreSQL Database failed to become available, cannot continue"
        return $RETURNVAL
    else
        log "- PostgreSQL Database is now available"
    fi
    line_break

    log_notice "${FUNCTION_DESCRIPTION}: Setting Maintenance Policy"
    rds_set_maintenance_policy "${POSTGRESQL_POSTGRESQLDBID}" "${POSTGRESQL_PREFERREDMAINTENANCEWINDOW}" "${POSTGRESQL_AUTOMINORVERSIONUPGRADE}" no no "${TMP_AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "- failed to set maintenance policy, cannot continue"
        return $RETURNVAL
    fi
    call_sleep 15 "allowing PostgreSQL Database time to apply logic"
    log "- Waiting for PostgreSQL Database to become available"
    rds_poll_status "${POSTGRESQL_POSTGRESQLDBID}" yes "${AWS_OPERATION_TIMEOUT}" "" "${TMP_AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "- PostgreSQL Database failed to become available, cannot continue"
        return $RETURNVAL
    else
        log "- PostgreSQL Database is now available"
    fi
    line_break

    log_notice "${FUNCTION_DESCRIPTION}: Setting Backup Policy"
    rds_set_backup_policy "${POSTGRESQL_POSTGRESQLDBID}" "${POSTGRESQL_BACKUPRETENTIONLIMIT}" "${POSTGRESQL_BACKUPWINDOW}" no "${TMP_AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "- failed to set backup policy, cannot continue"
        return $RETURNVAL
    fi
    call_sleep 15 "allowing PostgreSQL Database time to apply logic"
    log "- Waiting for PostgreSQL Database to become available"
    rds_poll_status "${POSTGRESQL_POSTGRESQLDBID}" yes "${AWS_OPERATION_TIMEOUT}" "" "${TMP_AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "- PostgreSQL Database failed to become available, cannot continue"
        return $RETURNVAL
    else
        log "- PostgreSQL Database is now available"
    fi
    line_break

    log_notice "${FUNCTION_DESCRIPTION}: Setting Password"
    rds_update_password "${POSTGRESQL_POSTGRESQLDBID}" "${POSTGRESQL_MASTERPASSWORD}" 30 "${TMP_AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "- failed to set password, cannot continue"
        return $RETURNVAL
    fi
    call_sleep 30 "allowing PostgreSQL Database time to apply logic"
    log "- Waiting for PostgreSQL Database to become available"
    rds_poll_status "${POSTGRESQL_POSTGRESQLDBID}" yes "${AWS_OPERATION_TIMEOUT}" "" "${TMP_AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "- PostgreSQL Database failed to become available, cannot continue"
        return $RETURNVAL
    else
        log "- PostgreSQL Database is now available"
    fi

    log_notice "- Rebooting Database"
    rds_reboot "${POSTGRESQL_POSTGRESQLDBID}" no 25 "${TMP_AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "- PostgreSQL Database failed to become available, cannot continue"
        return $RETURNVAL
    fi
    call_sleep 30 "allowing PostgreSQL Database time to apply logic"
    line_break

    log_notice "${FUNCTION_DESCRIPTION}: Checking if Database requires an additional reboot to finalize settings"
    if (rds_is_db_pending_reboot "${POSTGRESQL_POSTGRESQLDBID}" "${TMP_AWS_REGION}"); then
        log_notice "- PostgreSQL Database requires reboot"
        rds_reboot "${POSTGRESQL_POSTGRESQLDBID}" no 25 "${TMP_AWS_REGION}"
        RETURNVAL="$?"
        if [ ${RETURNVAL} -ne 0 ]; then
            log_error "- PostgreSQL Database failed to become available, cannot continue"
            return $RETURNVAL
        fi
    fi
    line_break

    log "- Waiting for PostgreSQL Database to become available"
    rds_poll_status "${POSTGRESQL_POSTGRESQLDBID}" yes "${AWS_OPERATION_TIMEOUT}" "" "${TMP_AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "- PostgreSQL Database failed to become available, cannot continue"
        return $RETURNVAL
    else
        log "- PostgreSQL Database is now available"
    fi
    log "${FUNCTION_DESCRIPTION}: successfully created and configured database"
    line_break
    return 0
}

###------------------------------------------------------------------------------------------------
## FUNCTION: postgresql_create_replica()
## - Creates PostgreSQL Replica Database
## - Arguments
##   - $1: Iteration
##   - $2: Verification Timeout <in minutes, defaults to 30 minutes>
##   - $3: Region
function postgresql_create_replica() {
    local FUNCTION_DESCRIPTION="PostgreSQL (Create Replica)"
    local TMP_ITERATION="${1}"
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

    local TMP_REPLICA_ID=""
    local TMP_CURRENT_PARAMETER_GROUP=""

    if(is_empty "${TMP_ITERATION}"); then
        log_error "${FUNCTION_DESCRIPTION}: Iteration not specified"
        return $E_BAD_ARGS
    fi
    TMP_ITERATION="$(printf "%02d\n" "${TMP_ITERATION}")"

    verify_array_key_values "POSTGRESQL_VARIABLES[@]" "POSTGRESQL"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "${FUNCTION_DESCRIPTION}: Failed to load required variables [PostgreSQL]"
        return $RETURNVAL
    fi
    verify_array_key_values "POSTGRESQL_REPLICA_VARIABLES[@]" "POSTGRESQL"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "${FUNCTION_DESCRIPTION}: Failed to load required variables [PostgreSQL Replica]"
        return $RETURNVAL
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi
    TMP_REPLICA_ID="${POSTGRESQL_POSTGRESQLDBID}-replica-${TMP_ITERATION}"

    local ARG_DB_INSTANCE_IDENTIFIER="--db-instance-identifier ${TMP_REPLICA_ID}"
    local ARG_SOURCE_DB_INSTANCE_IDENTIFIER="--source-db-instance-identifier ${POSTGRESQL_POSTGRESQLDBID}"
    local ARG_DB_INSTANCE_CLASS="--db-instance-class ${POSTGRESQL_REPLICADBINSTANCECLASS}"
    local ARG_PORT="--port ${POSTGRESQL_POSTGRESQLPORT}"
    local ARG_OPTION_GROUP_NAME="--option-group-name ${POSTGRESQL_OPTIONGROUPNAME}"
    if option_enabled POSTGRESQL_AUTOMINORVERSIONUPGRADE; then
        local ARG_MINOR_VERSION_UPGRADE="--auto-minor-version-upgrade"
    else
        local ARG_MINOR_VERSION_UPGRADE="--no-auto-minor-version-upgrade"
    fi
    local ARG_STORAGE_TYPE="--storage-type ${POSTGRESQL_REPLICADBSTORAGETYPE}"
    if [ ${POSTGRESQL_REPLICAIOPS} -gt 0 ]; then
        local ARG_IOPS="--iops ${POSTGRESQL_REPLICAIOPS}"
    else
        local ARG_IOPS=""
    fi
    if [[ "ZZ_$(to_upper "${POSTGRESQL_SUBNETTYPE}")" == "ZZ_PUBLIC" ]]; then
        local ARG_ACCESSIBILITY="--publicly-accessible"
    else
        local ARG_ACCESSIBILITY="--no-publicly-accessible"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"
    generate_temp_file FILE_TAGS_JSON "tags json"

    log "${FUNCTION_DESCRIPTION}: started (replica_id: [${TMP_REPLICA_ID}] / database_id: [${POSTGRESQL_POSTGRESQLDBID}])"

    log "${FUNCTION_DESCRIPTION}: Generating Tags JSON"
cat > ${FILE_TAGS_JSON} << ZZEOF
[
  { "Key": "Organization", "Value": "${POSTGRESQL_ORGANIZATIONNAME}"},
  { "Key": "Project", "Value": "${POSTGRESQL_PROJECTNAME}"},
  { "Key": "Function", "Value": "${POSTGRESQL_FUNCTIONNAME}"},
  { "Key": "Environment", "Value": "${POSTGRESQL_ENVIRONMENT}"},
  { "Key": "Owner", "Value": "${POSTGRESQL_OWNER}"},
  { "Key": "Contact", "Value": "${POSTGRESQL_CONTACT}"},
  { "Key": "DNS", "Value": "${POSTGRESQL_POSTGRESQLREPLICADNSRECORD}"}
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

        log "${FUNCTION_DESCRIPTION}: Creating Replica Database (Attempt::${COUNTER} of ${AWS_RDS_DEFAULT_RETRY_COUNT})"
        $(which aws) ${AWS_REGION_STRING} rds create-db-instance-read-replica \
            ${ARG_DB_INSTANCE_IDENTIFIER} \
            ${ARG_SOURCE_DB_INSTANCE_IDENTIFIER} \
            ${ARG_DB_INSTANCE_CLASS} \
            ${ARG_PORT} \
            ${ARG_OPTION_GROUP_NAME} \
            ${ARG_MINOR_VERSION_UPGRADE} \
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
            log "${FUNCTION_DESCRIPTION}: Successfully sent create request"
            RUN=0
        fi
    done

    call_sleep 60 "allowing PostgreSQL Replica Database time to warmup"
    line_break

    log_notice "Waiting for PostgreSQL Replica Database to become available"
    rds_poll_status "${TMP_REPLICA_ID}" yes "${AWS_OPERATION_TIMEOUT}" "" "${TMP_AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "- PostgreSQL Database failed to become available, cannot continue"
        return $RETURNVAL
    else
        log "- PostgreSQL Database is now available"
    fi
    line_break

    log "- Waiting for PostgreSQL Database to become available"
    rds_poll_status "${POSTGRESQL_POSTGRESQLDBID}" yes "${AWS_OPERATION_TIMEOUT}" "" "${TMP_AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "- PostgreSQL Database failed to become available, cannot continue"
        return $RETURNVAL
    else
        log "- PostgreSQL Database is now available"
    fi
    line_break

    rds_get_parameter_group TMP_CURRENT_PARAMETER_GROUP "${TMP_REPLICA_ID}" "${TMP_AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "- PostgreSQL Database failed to retrieve parameter group, cannot continue"
        return $RETURNVAL
    fi
    if [[ "ZZ_${TMP_CURRENT_PARAMETER_GROUP}" != "ZZ_${POSTGRESQL_REPLICAPARAMETERGROUPNAME}" ]]; then
        rds_set_parameter_group "${TMP_REPLICA_ID}" "${POSTGRESQL_REPLICAPARAMETERGROUPNAME}" no "${TMP_AWS_REGION}"
        RETURNVAL="$?"
        if [ ${RETURNVAL} -ne 0 ]; then
            log_error "- failed to set parameter group, cannot continue"
            return $RETURNVAL
        fi
        call_sleep 30 "allowing PostgreSQL Replica Database time to apply logic"
        line_break
    fi

    if (rds_is_db_pending_reboot "${TMP_REPLICA_ID}" "${TMP_AWS_REGION}"); then
        log_notice "- PostgreSQL Database Replica requires additional reboot"
        rds_reboot "${TMP_REPLICA_ID}" no 25 "${TMP_AWS_REGION}"
        RETURNVAL="$?"
        if [ ${RETURNVAL} -ne 0 ]; then
            log_error "- PostgreSQL Database failed to become available, cannot continue"
            return $RETURNVAL
        fi
    fi
    log "${FUNCTION_DESCRIPTION}: successfully created and configured replica database"
    line_break
    return 0
}

###------------------------------------------------------------------------------------------------
## FUNCTION: postgresql_execute_sql()
## - Attempts to connect to specified PostgreSQL Database in order to execute SQL file
## - Arguments
##   - $1: Database Endpoint
##   - $2: Username
##   - $3: Password
##   - $4: Database Name (optional, defaults to 'postgres')
##   - $5: Database Port (optional, if not specified, defaults to POSTGRESQL_DEFAULT_PORT)
##   - $6: SQL File
function postgresql_execute_sql() {
    local FUNCTION_DESCRIPTION="PostgreSQL (Execute SQL)"
    local TMP_DATABASE_ENDPOINT="${1}"
    local TMP_DB_USERNAME="${2}"
    local TMP_DB_PASSWORD="${3}"
    local TMP_DATABASE_NAME="${4}"
    local TMP_DATABASE_PORT="${5}"
    local TMP_FILE_SQL="${6}"

    local RETURNVAL=""
    local FILE_POSTGRESQL_OUTPUT=""

    if(is_empty "${TMP_DATABASE_ENDPOINT}"); then
        log_error "${FUNCTION_DESCRIPTION}: Database Endpoint not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_DB_USERNAME}"); then
        log_error "${FUNCTION_DESCRIPTION}: Username not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_DB_PASSWORD}"); then
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
        TMP_DATABASE_PORT="${POSTGRESQL_DEFAULT_PORT}"
    fi
    if [ ! -f "${TMP_FILE_SQL}" ]; then
        log_error "${FUNCTION_DESCRIPTION}: SQL file does not exist [${TMP_FILE_SQL}]"
        return $E_OBJECT_NOT_FOUND
    fi

    generate_temp_file FILE_POSTGRESQL_OUTPUT "postgresql output"

    log "${FUNCTION_DESCRIPTION}: started"
    log "- Endpoint: [${TMP_DATABASE_ENDPOINT}]"
    log "- Username: [${TMP_DB_USERNAME}]"
    log "- Password: [${TMP_DB_PASSWORD//?/*}]"
    log "- Database: [${TMP_DATABASE_NAME}]"
    log "- SQL File: [${TMP_FILE_SQL}]"
    PGPASSWORD=${TMP_DB_PASSWORD} $(which psql) -h${TMP_DATABASE_ENDPOINT} -p${TMP_DATABASE_PORT} -U${TMP_DB_USERNAME} ${TMP_DATABASE_NAME} -f "${TMP_FILE_SQL}" >${FILE_POSTGRESQL_OUTPUT} 2>&1
    RETURNVAL="$?"
    PGPASSWORD=""
    if [ ${RETURNVAL} -ne 0 ]; then
        log_add_from_file "${FILE_POSTGRESQL_OUTPUT}" "${FUNCTION_DESCRIPTION}: postgresql output"
        log_error "${FUNCTION_DESCRIPTION}: failed to execute SQL file (psql_exit_code::${RETURNVAL}])"
        return $E_POSTGRESQL_EXECUTION_FAILURE
    fi
    log_add_from_file "${FILE_POSTGRESQL_OUTPUT}" "${FUNCTION_DESCRIPTION}: postgresql output"
    log "${FUNCTION_DESCRIPTION}: successfully executed SQL file"
    sleep 0.1
    return 0
}

###------------------------------------------------------------------------------------------------
## FUNCTION: postgresql_set_dns()
## - Sets DNS record for PostgreSQL Database
## - Arguments
##   - $1: Region
function postgresql_set_dns() {
    local FUNCTION_DESCRIPTION="PostgreSQL (Set DNS)"
    local TMP_AWS_REGION="${1}"

    local RETURNVAL=""
    local TMP_DB_ENDPOINT=""
    local TMP_KEY=""
    local TMP_VAR=""
    local KEY_MAX_LENGTH=0

    local TMP_ARRAY_REPLICA_IDS=()

    verify_array_key_values "POSTGRESQL_DNS_VARIABLES[@]" "POSTGRESQL"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "${FUNCTION_DESCRIPTION}: Failed to load required variables [PostgreSQL DNS]"
        return $RETURNVAL
    fi
    line_break
    log_highlight "DNS Information"
    for TMP_KEY in "${POSTGRESQL_DNS_VARIABLES[@]}"; do
        if [ ${#TMP_KEY} -gt $KEY_MAX_LENGTH ]; then
            KEY_MAX_LENGTH=${#TMP_KEY}
        fi
    done
    KEY_MAX_LENGTH=$((${KEY_MAX_LENGTH}+1))

    for TMP_KEY in "${POSTGRESQL_DNS_VARIABLES[@]}"; do
        TMP_VAR="$(to_upper "POSTGRESQL_${TMP_KEY}")"
        log "$(printf "%-1s %-${KEY_MAX_LENGTH}s %s" "-" "${TMP_KEY}:" "[${!TMP_VAR}]")"
    done
    line_break

    log_notice "${FUNCTION_DESCRIPTION}: Setting Primary DNS Record"
    rds_get_endpoint TMP_DB_ENDPOINT "${POSTGRESQL_POSTGRESQLDBID}" "${TMP_AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "- Failed to retrieve Endpoint Address, cannot continue"
        return $RETURNVAL
    fi
    log "- Endpoint Address: [${TMP_DB_ENDPOINT}]"
    route53_upsert_record "${POSTGRESQL_DNSDATABASEHOSTEDZONEID}" "${POSTGRESQL_POSTGRESQLDNSRECORD}" "CNAME" ${AWS_RDS_DNS_TTL} "${TMP_DB_ENDPOINT}" "" "" "${TMP_AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "${FUNCTION_DESCRIPTION}: Failed to set Primary DNS Record"
        return $RETURNVAL
    fi

    log "${FUNCTION_DESCRIPTION}: Deleting Replica DNS records before setting, as it may have multiple weighted records"
    route53_delete_record "${POSTGRESQL_DNSDATABASEHOSTEDZONEID}" "${POSTGRESQL_POSTGRESQLREPLICADNSRECORD}" "${TMP_AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "${FUNCTION_DESCRIPTION}: Failed to remove Replica DNS records"
        return $RETURNVAL
    fi
    log_notice "${FUNCTION_DESCRIPTION}: Setting Replica DNS Record"

    rds_get_database_replica_ids TMP_ARRAY_REPLICA_IDS "${POSTGRESQL_POSTGRESQLDBID}" "${TMP_AWS_REGION}"
    if [ ${#TMP_ARRAY_REPLICA_IDS[@]} -gt 0 ]; then
        for TMP_KEY in "${TMP_ARRAY_REPLICA_IDS[@]}"; do
            if(! is_empty "${TMP_KEY}"); then
                log "- Replica Detected [${TMP_KEY}]"
                rds_get_endpoint TMP_DB_ENDPOINT "${TMP_KEY}" "${TMP_AWS_REGION}"
                RETURNVAL="$?"
                if [ ${RETURNVAL} -ne 0 ]; then
                    log_error "- Failed to retrieve Endpoint Address for Replica [${TMP_KEY}], cannot continue"
                    return $RETURNVAL
                fi
                route53_upsert_record "${POSTGRESQL_DNSDATABASEHOSTEDZONEID}" "${POSTGRESQL_POSTGRESQLREPLICADNSRECORD}" "CNAME" ${AWS_RDS_DNS_TTL} "${TMP_DB_ENDPOINT}" "1" "${TMP_KEY}" "${TMP_AWS_REGION}"
                RETURNVAL="$?"
                if [ ${RETURNVAL} -ne 0 ]; then
                    log_error "${FUNCTION_DESCRIPTION}: Failed to set Replica DNS Record [${TMP_KEY}]"
                    return $RETURNVAL
                fi
            fi
        done
    else
        log "- No Replicas found, setting Replica DNS Record to point to primary"
        route53_upsert_record "${POSTGRESQL_DNSDATABASEHOSTEDZONEID}" "${POSTGRESQL_POSTGRESQLREPLICADNSRECORD}" "CNAME" ${AWS_RDS_DNS_TTL} "${TMP_DB_ENDPOINT}" "" "" "${TMP_AWS_REGION}"
        RETURNVAL="$?"
        if [ ${RETURNVAL} -ne 0 ]; then
            log_error "${FUNCTION_DESCRIPTION}: Failed to set Replica DNS Record"
            return $RETURNVAL
        fi
    fi

    return 0
}

###------------------------------------------------------------------------------------------------
## FUNCTION: postgresql_verify_connection()
## - Attempts to connect to specified PostgreSQL Database in order to verify if database is available and credentials are correct
## - Arguments
##   - $1: Database Endpoint
##   - $2: Username
##   - $3: Password
##   - $4: Database Name (optional, defaults to 'postgres')
##   - $5: Database Port (optional, if not specified, defaults to POSTGRESQL_DEFAULT_PORT)
function postgresql_verify_connection() {
    local FUNCTION_DESCRIPTION="PostgreSQL (Verify Connection)"
    local TMP_DATABASE_ENDPOINT="${1}"
    local TMP_DB_USERNAME="${2}"
    local TMP_DB_PASSWORD="${3}"
    local TMP_DATABASE_NAME="${4}"
    local TMP_DATABASE_PORT="${5}"

    local RETURNVAL=""
    local FILE_POSTGRESQL_OUTPUT=""

    if(is_empty "${TMP_DATABASE_ENDPOINT}"); then
        log_error "${FUNCTION_DESCRIPTION}: Database Endpoint not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_DB_USERNAME}"); then
        log_error "${FUNCTION_DESCRIPTION}: Username not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_DB_PASSWORD}"); then
        log_error "${FUNCTION_DESCRIPTION}: Password not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_DATABASE_NAME}"); then
        TMP_DATABASE_NAME="postgres"
    fi
    if(is_empty "${TMP_DATABASE_PORT}"); then
        TMP_DATABASE_PORT="${POSTGRESQL_DEFAULT_PORT}"
    fi

    generate_temp_file FILE_POSTGRESQL_OUTPUT "postgresql output"

    PGPASSWORD=${TMP_DB_PASSWORD} $(which psql) -h${TMP_DATABASE_ENDPOINT} -p${TMP_DATABASE_PORT} -U${TMP_DB_USERNAME} ${TMP_DATABASE_NAME}  -c '\l' >${FILE_POSTGRESQL_OUTPUT} 2>&1
    RETURNVAL="$?"
    PGPASSWORD=""
    if [ ${RETURNVAL} -ne 0 ]; then
        log "${FUNCTION_DESCRIPTION}: Database [${TMP_DATABASE_ENDPOINT}] - [$(color_text "${LOG_COLOR_ERROR}" "FAILED")] (psql_exit_code::${RETURNVAL}])"
        log_add_from_file "${FILE_POSTGRESQL_OUTPUT}" "${FUNCTION_DESCRIPTION}: postgresql output"
        return $E_DATABASE_CONNECTION_FAILURE
    fi

    log "${FUNCTION_DESCRIPTION}: Database [${TMP_DATABASE_ENDPOINT}] - [$(color_text "${LOG_COLOR_SUCCESS}" "OK")]"
    return 0
}
