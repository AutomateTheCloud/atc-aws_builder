###------------------------------------------------------------------------------------------------
# script:  rds_mysql.inc
# purpose: Collection of functions related to AWS (RDS - MySQL)
# version: 1.0.0
#
# function list:
#          - load_info_mysql
#          - mysql_create
#          - mysql_create_from_snapshot
#          - mysql_create_replica
#          - mysql_execute_sql
#          - mysql_execute_sql_output_to_file
#          - mysql_set_dns
#          - mysql_verify_connection
###------------------------------------------------------------------------------------------------
REQUIRED_EXECUTABLES+=('aws' 'mysql')

###------------------------------------------------------------------------------------------------
# Variables
MYSQL_DEFAULT_PORT="3306"

MYSQL_VARIABLES+=(
    'MySQLReferenceName'
    'MySQLDBId'
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
    'MySQLPort'
    'DBSubnetGroupId'
    'MySQLSecurityGroupId'

    'AutoMinorVersionUpgrade'
    'PreferredMaintenanceWindow'

    'BackupRetentionLimit'
    'BackupWindow'

    'DNSDatabaseDomain'
    'DNSDatabaseHostedZoneID'
    'DNSDatabaseSubdomain'
    'MySQLDNSRecord'
    'MySQLReplicaDNSRecord'

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

MYSQL_REPLICA_VARIABLES+=(
    'ReplicaDBInstanceClass'
    'ReplicaParameterGroupName'
    'ReplicaOptionGroupName'
    'ReplicaDBStorageType'
    'ReplicaIOPS'
)

MYSQL_DNS_VARIABLES+=(
    'MySQLDNSRecord'
    'MySQLReplicaDNSRecord'
    'DNSDatabaseHostedZoneID'
)

RDS_REPLICA_DB_IDS=()

MYSQL_MYSQLREFERENCENAME=""
MYSQL_DBINSTANCECLASS=""
MYSQL_ENGINEVERSION=""
MYSQL_PARAMETERGROUPNAME=""
MYSQL_OPTIONGROUPNAME=""
MYSQL_DBNAME=""
MYSQL_ALLOCATEDSTORAGE=""
MYSQL_DBSTORAGETYPE=""
MYSQL_IOPS=""
MYSQL_ENCRYPTIONENABLED=""
MYSQL_SUBNETTYPE=""
MYSQL_MULTIAZ=""
MYSQL_MYSQLPORT=""
MYSQL_DBSUBNETGROUPID=""
MYSQL_MYSQLSECURITYGROUPID=""
MYSQL_AUTOMINORVERSIONUPGRADE=""
MYSQL_PREFERREDMAINTENANCEWINDOW=""
MYSQL_BACKUPRETENTIONLIMIT=""
MYSQL_BACKUPWINDOW=""
MYSQL_DNSDATABASEDOMAIN=""
MYSQL_DNSDATABASEHOSTEDZONEID=""
MYSQL_DNSDATABASESUBDOMAIN=""
MYSQL_MYSQLDNSRECORD=""
MYSQL_MYSQLREPLICADNSRECORD=""
MYSQL_ACCOUNTNAME=""
MYSQL_ACCOUNTABBR=""
MYSQL_ACCOUNTNUMBER=""
MYSQL_ORGANIZATIONNAME=""
MYSQL_ORGANIZATIONABBR=""
MYSQL_ENVIRONMENT=""
MYSQL_PROJECTNAME=""
MYSQL_PROJECTABBR=""
MYSQL_FUNCTIONNAME=""
MYSQL_FUNCTIONABBR=""
MYSQL_OWNER=""
MYSQL_CONTACT=""
MYSQL_REGION=""

MYSQL_REPLICACOUNT=""
MYSQL_REPLICADBINSTANCECLASS=""
MYSQL_REPLICAPARAMETERGROUPNAME=""
MYSQL_REPLICAOPTIONGROUPNAME=""
MYSQL_REPLICADBSTORAGETYPE=""
MYSQL_REPLICAIOPS=""

MYSQL_DBENDPOINT=""
MYSQL_DBREPLICAENDPOINT=""
MYSQL_MYSQLDBID=""
MYSQL_MASTERUSERNAME=""
MYSQL_MASTERPASSWORD=""

###------------------------------------------------------------------------------------------------
## FUNCTION: load_info_mysql()
## - Loads MySQL Information into memory from CloudFormation Outputs file
## - Arguments
##   - $1: CloudFormation Outputs File
##   - $2: Region
function load_info_mysql() {
    local FUNCTION_DESCRIPTION="Load Info (MySQL)"
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
    load_array_properties_from_file "MYSQL_VARIABLES[@]" "${TMP_FILE_CLOUDFORMATION_OUTPUTS}" "MYSQL"
    if(is_empty "${MYSQL_IOPS}"); then MYSQL_IOPS=0; fi
    if(is_empty "${MYSQL_ENCRYPTIONENABLED}"); then MYSQL_ENCRYPTIONENABLED=no; fi
    if(is_empty "${MYSQL_REPLICACOUNT}"); then MYSQL_REPLICACOUNT=0; fi

    verify_array_key_values "PARAMETER_VARIABLES[@]" "MYSQL"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "${FUNCTION_DESCRIPTION}: Failed to load required variables [Parameter Related]"
        return $RETURNVAL
    fi

    for TMP_KEY in "${CREDENTIALS_VARIABLES[@]}"; do
        TMP_VAR="$(to_upper "MYSQL_${TMP_KEY}")"
        TMP_PARAMETER="/secrets/${MYSQL_PROJECTABBR}/${MYSQL_FUNCTIONABBR}/${MYSQL_ENVIRONMENT}/vars/${TMP_KEY}"
        parameter_get_silent ${TMP_VAR} "${TMP_PARAMETER}" yes "${TMP_AWS_REGION}"
    done
    verify_array_key_values "CREDENTIALS_VARIABLES[@]" "MYSQL"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "${FUNCTION_DESCRIPTION}: Failed to load required variables [Credentials]"
        return $RETURNVAL
    fi

    generate_uuid TMP_UUID 2
    MYSQL_MYSQLDBID="${MYSQL_MYSQLREFERENCENAME}-${TMP_UUID}"

    verify_array_key_values "MYSQL_VARIABLES[@]" "MYSQL"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "${FUNCTION_DESCRIPTION}: Failed to load required variables [MySQL]"
        return $RETURNVAL
    fi

    if [[ "ZZ_$(to_upper "${MYSQL_DBSTORAGETYPE}")" == "ZZ_IO1" ]]; then
        if [ ${MYSQL_IOPS} -eq 0 ]; then
            log_error "${FUNCTION_DESCRIPTION}: Storage Type [io1] selected, IOPS must be greater than Zero [MySQL]"
            return $E_BAD_ARGS
        fi
    fi

    if(! is_int "${MYSQL_REPLICACOUNT}"); then
        log_error "${FUNCTION_DESCRIPTION}: Replica Count is not an integer [${MYSQL_REPLICACOUNT}]"
        return $E_BAD_ARGS
    fi

    if [ ${MYSQL_REPLICACOUNT} -gt 0 ]; then
        load_array_properties_from_file "MYSQL_REPLICA_VARIABLES[@]" "${TMP_FILE_CLOUDFORMATION_OUTPUTS}" "MYSQL"
        if(is_empty "${MYSQL_REPLICAIOPS}"); then MYSQL_REPLICAIOPS=0; fi
        verify_array_key_values "MYSQL_REPLICA_VARIABLES[@]" "MYSQL"
        RETURNVAL="$?"
        if [ ${RETURNVAL} -ne 0 ]; then
            log_error "${FUNCTION_DESCRIPTION}: Failed to load required variables [MySQL Replica]"
            return $RETURNVAL
        fi
        if [ ${MYSQL_BACKUPRETENTIONLIMIT} -eq 0 ]; then
            log_error "${FUNCTION_DESCRIPTION}: In order to have replicas, BackupRetentionLimit must be greater than 0 "
            return $E_BAD_ARGS
        fi
        if [[ "ZZ_$(to_upper "${MYSQL_REPLICADBSTORAGETYPE}")" == "ZZ_IO1" ]]; then
            if [ ${MYSQL_REPLICAIOPS} -eq 0 ]; then
                log_error "${FUNCTION_DESCRIPTION}: Replica Storage Type [io1] selected, IOPS must be greater than Zero [MySQL Replica]"
                return $E_BAD_ARGS
            fi
        fi
    fi

    line_break
    log_highlight "MySQL Information"
    for TMP_KEY in "${MYSQL_VARIABLES[@]}"; do
        if [ ${#TMP_KEY} -gt $KEY_MAX_LENGTH ]; then
            KEY_MAX_LENGTH=${#TMP_KEY}
        fi
    done

    if [ ${MYSQL_REPLICACOUNT} -gt 0 ]; then
        for TMP_KEY in "${MYSQL_REPLICA_VARIABLES[@]}"; do
            if [ ${#TMP_KEY} -gt $KEY_MAX_LENGTH ]; then
                KEY_MAX_LENGTH=${#TMP_KEY}
            fi
        done
    fi
    KEY_MAX_LENGTH=$((${KEY_MAX_LENGTH}+1))

    for TMP_KEY in "${MYSQL_VARIABLES[@]}"; do
        TMP_VAR="$(to_upper "MYSQL_${TMP_KEY}")"
        log "$(printf "%-1s %-${KEY_MAX_LENGTH}s %s" "-" "${TMP_KEY}:" "[${!TMP_VAR}]")"
    done
    if [ ${MYSQL_REPLICACOUNT} -gt 0 ]; then
        for TMP_KEY in "${MYSQL_REPLICA_VARIABLES[@]}"; do
            TMP_VAR="$(to_upper "MYSQL_${TMP_KEY}")"
            log "$(printf "%-1s %-${KEY_MAX_LENGTH}s %s" "-" "${TMP_KEY}:" "[${!TMP_VAR}]")"
        done
    fi
    log "$(printf "%-1s %-${KEY_MAX_LENGTH}s %s" "-" "MasterUsername:" "[${MYSQL_MASTERUSERNAME}]")"
    log "$(printf "%-1s %-${KEY_MAX_LENGTH}s %s" "-" "MasterPassword:" "[${MYSQL_MASTERPASSWORD//?/*}]")"
    line_break
    return 0
}

###------------------------------------------------------------------------------------------------
## FUNCTION: mysql_create()
## - Creates MySQL Database
## - Arguments
##   - $1: Verification Timeout <in minutes, defaults to 30 minutes>
##   - $2: Region
function mysql_create() {
    local FUNCTION_DESCRIPTION="MySQL (Create)"
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

    verify_array_key_values "MYSQL_VARIABLES[@]" "MYSQL"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "${FUNCTION_DESCRIPTION}: Failed to load required variables [MySQL]"
        return $RETURNVAL
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    local ARG_DB_INSTANCE_IDENTIFIER="--db-instance-identifier ${MYSQL_MYSQLDBID}"
    local ARG_DB_NAME="--db-name ${MYSQL_DBNAME}"
    local ARG_MASTER_USERNAME="--master-username ${MYSQL_MASTERUSERNAME}"
    local ARG_MASTER_USER_PASSWORD="--master-user-password ${MYSQL_MASTERPASSWORD}"
    local ARG_ENGINE="--engine mysql"
    local ARG_ENGINE_VERSION="--engine-version ${MYSQL_ENGINEVERSION}"
    local ARG_DB_INSTANCE_CLASS="--db-instance-class ${MYSQL_DBINSTANCECLASS}"
    local ARG_VPC_SECURITY_GROUP_IDS="--vpc-security-group-ids ${MYSQL_MYSQLSECURITYGROUPID}"
    local ARG_DB_SUBNET_GROUP_NAME="--db-subnet-group-name ${MYSQL_DBSUBNETGROUPID}"
    local ARG_PORT="--port ${MYSQL_MYSQLPORT}"
    local ARG_DB_PARAMETER_GROUP_NAME="--db-parameter-group-name ${MYSQL_PARAMETERGROUPNAME}"
    local ARG_OPTION_GROUP_NAME="--option-group-name ${MYSQL_OPTIONGROUPNAME}"
    local ARG_BACKUP_RETENTION_PERIOD="--backup-retention-period ${MYSQL_BACKUPRETENTIONLIMIT}"
    if [ ${MYSQL_BACKUPRETENTIONLIMIT} -gt 0 ]; then
        local ARG_PREFERRED_BACKUP_WINDOW="--preferred-backup-window ${MYSQL_BACKUPWINDOW} --copy-tags-to-snapshot"
    else
        local ARG_PREFERRED_BACKUP_WINDOW="--copy-tags-to-snapshot"
    fi
    local ARG_PREFERRED_MAINTENANCE_WINDOW="--preferred-maintenance-window ${MYSQL_PREFERREDMAINTENANCEWINDOW}"
    if option_enabled MYSQL_AUTOMINORVERSIONUPGRADE; then
        local ARG_MINOR_VERSION_UPGRADE="--auto-minor-version-upgrade"
    else
        local ARG_MINOR_VERSION_UPGRADE="--no-auto-minor-version-upgrade"
    fi
    if option_enabled MYSQL_MULTIAZ; then
        local ARG_MULTI_AZ="--multi-az"
    else
        local ARG_MULTI_AZ="--no-multi-az"
    fi
    local ARG_ALLOCATED_STORAGE="--allocated-storage ${MYSQL_ALLOCATEDSTORAGE}"
    local ARG_STORAGE_TYPE="--storage-type ${MYSQL_DBSTORAGETYPE}"
    if [ ${MYSQL_IOPS} -gt 0 ]; then
        local ARG_IOPS="--iops ${MYSQL_IOPS}"
    else
        local ARG_IOPS=""
    fi
    if option_enabled MYSQL_ENCRYPTIONENABLED; then
        local ARG_ENCRYPTION="--storage-encrypted"
    else
        local ARG_ENCRYPTION="--no-storage-encrypted"
    fi
    if [[ "ZZ_$(to_upper "${MYSQL_SUBNETTYPE}")" == "ZZ_PUBLIC" ]]; then
        local ARG_ACCESSIBILITY="--publicly-accessible"
    else
        local ARG_ACCESSIBILITY="--no-publicly-accessible"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"
    generate_temp_file FILE_TAGS_JSON "tags json"

    log_notice "${FUNCTION_DESCRIPTION}: started (database_id: [${MYSQL_MYSQLDBID}])"

    log "${FUNCTION_DESCRIPTION}: Generating Tags JSON"
cat > ${FILE_TAGS_JSON} << ZZEOF
[
  { "Key": "Organization", "Value": "${MYSQL_ORGANIZATIONNAME}"},
  { "Key": "Project", "Value": "${MYSQL_PROJECTNAME}"},
  { "Key": "Function", "Value": "${MYSQL_FUNCTIONNAME}"},
  { "Key": "Environment", "Value": "${MYSQL_ENVIRONMENT}"},
  { "Key": "Owner", "Value": "${MYSQL_OWNER}"},
  { "Key": "Contact", "Value": "${MYSQL_CONTACT}"},
  { "Key": "DNS", "Value": "${MYSQL_MYSQLDNSRECORD}"}
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

    call_sleep 60 "allowing MySQL Database time to warmup"
    line_break

    log "- Waiting for MySQL Database to become available"
    rds_poll_status "${MYSQL_MYSQLDBID}" yes "${AWS_OPERATION_TIMEOUT}" "" "${TMP_AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "- MySQL Database failed to become available, cannot continue"
        return $RETURNVAL
    else
        log "- MySQL Database is now available"
    fi
    log "${FUNCTION_DESCRIPTION}: successfully created and configured database"
    line_break
    return 0
}

###------------------------------------------------------------------------------------------------
## FUNCTION: mysql_create_from_snapshot()
## - Creates MySQL Database from specified Snapshot
## - Arguments
##   - $1: Snapshot ARN
##   - $2: Verification Timeout <in minutes, defaults to 30 minutes>
##   - $3: Region
function mysql_create_from_snapshot() {
    local FUNCTION_DESCRIPTION="MySQL (Create from Snapshot)"
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

    verify_array_key_values "MYSQL_VARIABLES[@]" "MYSQL"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "${FUNCTION_DESCRIPTION}: Failed to load required variables [MySQL]"
        return $RETURNVAL
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    local ARG_DB_INSTANCE_IDENTIFIER="--db-instance-identifier ${MYSQL_MYSQLDBID}"
    local ARG_DB_SNAPSHOT_IDENTIFIER="--db-snapshot-identifier ${TMP_SNAPSHOT_ARN}"
    local ARG_ENGINE="--engine mysql"
    local ARG_DB_INSTANCE_CLASS="--db-instance-class ${MYSQL_DBINSTANCECLASS}"
    local ARG_DB_SUBNET_GROUP_NAME="--db-subnet-group-name ${MYSQL_DBSUBNETGROUPID}"
    local ARG_PORT="--port ${MYSQL_MYSQLPORT}"
    local ARG_OPTION_GROUP_NAME="--option-group-name ${MYSQL_OPTIONGROUPNAME}"
    if option_enabled MYSQL_MULTIAZ; then
        local ARG_MULTI_AZ="--multi-az"
    else
        local ARG_MULTI_AZ="--no-multi-az"
    fi
    local ARG_STORAGE_TYPE="--storage-type ${MYSQL_DBSTORAGETYPE}"
    if [ ${MYSQL_IOPS} -gt 0 ]; then
        local ARG_IOPS="--iops ${MYSQL_IOPS}"
    else
        local ARG_IOPS=""
    fi
    if [[ "ZZ_$(to_upper "${MYSQL_SUBNETTYPE}")" == "ZZ_PUBLIC" ]]; then
        local ARG_ACCESSIBILITY="--publicly-accessible"
    else
        local ARG_ACCESSIBILITY="--no-publicly-accessible"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"
    generate_temp_file FILE_TAGS_JSON "tags json"

    log_notice "${FUNCTION_DESCRIPTION}: started (database_id: [${MYSQL_MYSQLDBID}] / snapshot_arn: [${TMP_SNAPSHOT_ARN}])"

    log "${FUNCTION_DESCRIPTION}: Generating Tags JSON"
cat > ${FILE_TAGS_JSON} << ZZEOF
[
  { "Key": "Organization", "Value": "${MYSQL_ORGANIZATIONNAME}"},
  { "Key": "Project", "Value": "${MYSQL_PROJECTNAME}"},
  { "Key": "Function", "Value": "${MYSQL_FUNCTIONNAME}"},
  { "Key": "Environment", "Value": "${MYSQL_ENVIRONMENT}"},
  { "Key": "Owner", "Value": "${MYSQL_OWNER}"},
  { "Key": "Contact", "Value": "${MYSQL_CONTACT}"},
  { "Key": "DNS", "Value": "${MYSQL_MYSQLDNSRECORD}"}
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

    call_sleep 60 "allowing MySQL Database time to warmup"
    line_break

    log "- Waiting for MySQL Database to become available"
    rds_poll_status "${MYSQL_MYSQLDBID}" yes "${AWS_OPERATION_TIMEOUT}" "" "${TMP_AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "- MySQL Database failed to become available, cannot continue"
        return $RETURNVAL
    else
        log "- MySQL Database is now available"
    fi
    line_break

    log_notice "${FUNCTION_DESCRIPTION}: Checking Engine Version"
    rds_get_engine_version TMP_CURRENT_ENGINE_VERSION "${MYSQL_MYSQLDBID}" "${TMP_AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "- MySQL Database failed to retrieve engine version, cannot continue"
        return $RETURNVAL
    fi
    if [[ "ZZ_${TMP_CURRENT_ENGINE_VERSION}" != "ZZ_${MYSQL_ENGINEVERSION}" ]]; then
        log "- Engine Version is not on specified version, requires modification"
        log "- Configuring database to allow Engine Version modification"
        rds_set_maintenance_policy "${MYSQL_MYSQLDBID}" "${MYSQL_PREFERREDMAINTENANCEWINDOW}" "${MYSQL_AUTOMINORVERSIONUPGRADE}" yes yes "${TMP_AWS_REGION}"
        RETURNVAL="$?"
        if [ ${RETURNVAL} -ne 0 ]; then
            log_error "- failed to set maintenance policy, cannot continue"
            return $RETURNVAL
        fi
        log "- Configuring Engine Version"
        rds_set_engine_version "${MYSQL_MYSQLDBID}" "${MYSQL_ENGINEVERSION}" "${MYSQL_PARAMETERGROUPNAME}" yes "${TMP_AWS_REGION}"
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
    rds_get_parameter_group TMP_CURRENT_PARAMETER_GROUP "${MYSQL_MYSQLDBID}" "${TMP_AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "- MySQL Database failed to retrieve parameter group, cannot continue"
        return $RETURNVAL
    fi
    if [[ "ZZ_${TMP_CURRENT_PARAMETER_GROUP}" != "ZZ_${MYSQL_PARAMETERGROUPNAME}" ]]; then
        log "- Parameter Group is not on specified parameter group, requires modification"
        log "- Configuring Parameter Group"
        rds_set_parameter_group "${MYSQL_MYSQLDBID}" "${MYSQL_PARAMETERGROUPNAME}" no "${TMP_AWS_REGION}"
        RETURNVAL="$?"
        if [ ${RETURNVAL} -ne 0 ]; then
            log_error "- failed to set parameter group, cannot continue"
            return $RETURNVAL
        fi
        call_sleep 15 "allowing MySQL Database time to apply logic"
        log "- Waiting for MySQL Database to become available"
        rds_poll_status "${MYSQL_MYSQLDBID}" yes "${AWS_OPERATION_TIMEOUT}" "" "${TMP_AWS_REGION}"
        RETURNVAL="$?"
        if [ ${RETURNVAL} -ne 0 ]; then
            log_error "- MySQL Database failed to become available, cannot continue"
            return $RETURNVAL
        else
            log "- MySQL Database is now available"
        fi
    else
        log "- Parameter Group is on specified parameter group, no need to modify"
    fi
    line_break

    log_notice "${FUNCTION_DESCRIPTION}: Setting Security Group"
    rds_set_security_group "${MYSQL_MYSQLDBID}" "${MYSQL_MYSQLSECURITYGROUPID}" yes "${TMP_AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "- failed to set security group, cannot continue"
        return $RETURNVAL
    fi
    call_sleep 15 "allowing MySQL Database time to apply logic"
    log "- Waiting for MySQL Database to become available"
    rds_poll_status "${MYSQL_MYSQLDBID}" yes "${AWS_OPERATION_TIMEOUT}" "" "${TMP_AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "- MySQL Database failed to become available, cannot continue"
        return $RETURNVAL
    else
        log "- MySQL Database is now available"
    fi
    line_break

    log_notice "${FUNCTION_DESCRIPTION}: Setting Maintenance Policy"
    rds_set_maintenance_policy "${MYSQL_MYSQLDBID}" "${MYSQL_PREFERREDMAINTENANCEWINDOW}" "${MYSQL_AUTOMINORVERSIONUPGRADE}" no no "${TMP_AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "- failed to set maintenance policy, cannot continue"
        return $RETURNVAL
    fi
    call_sleep 15 "allowing MySQL Database time to apply logic"
    log "- Waiting for MySQL Database to become available"
    rds_poll_status "${MYSQL_MYSQLDBID}" yes "${AWS_OPERATION_TIMEOUT}" "" "${TMP_AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "- MySQL Database failed to become available, cannot continue"
        return $RETURNVAL
    else
        log "- MySQL Database is now available"
    fi
    line_break

    log_notice "${FUNCTION_DESCRIPTION}: Setting Backup Policy"
    rds_set_backup_policy "${MYSQL_MYSQLDBID}" "${MYSQL_BACKUPRETENTIONLIMIT}" "${MYSQL_BACKUPWINDOW}" no "${TMP_AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "- failed to set backup policy, cannot continue"
        return $RETURNVAL
    fi
    call_sleep 15 "allowing MySQL Database time to apply logic"
    log "- Waiting for MySQL Database to become available"
    rds_poll_status "${MYSQL_MYSQLDBID}" yes "${AWS_OPERATION_TIMEOUT}" "" "${TMP_AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "- MySQL Database failed to become available, cannot continue"
        return $RETURNVAL
    else
        log "- MySQL Database is now available"
    fi
    line_break

    log_notice "${FUNCTION_DESCRIPTION}: Setting Password"
    rds_update_password "${MYSQL_MYSQLDBID}" "${MYSQL_MASTERPASSWORD}" 30 "${TMP_AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "- failed to set password, cannot continue"
        return $RETURNVAL
    fi
    call_sleep 30 "allowing MySQL Database time to apply logic"
    log "- Waiting for MySQL Database to become available"
    rds_poll_status "${MYSQL_MYSQLDBID}" yes "${AWS_OPERATION_TIMEOUT}" "" "${TMP_AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "- MySQL Database failed to become available, cannot continue"
        return $RETURNVAL
    else
        log "- MySQL Database is now available"
    fi

    log_notice "- Rebooting Database"
    rds_reboot "${MYSQL_MYSQLDBID}" no 25 "${TMP_AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "- MySQL Database failed to become available, cannot continue"
        return $RETURNVAL
    fi
    call_sleep 30 "allowing MySQL Database time to apply logic"
    line_break

    log_notice "${FUNCTION_DESCRIPTION}: Checking if Database requires an additional reboot to finalize settings"
    if (rds_is_db_pending_reboot "${MYSQL_MYSQLDBID}" "${TMP_AWS_REGION}"); then
        log_notice "- MySQL Database requires reboot"
        rds_reboot "${MYSQL_MYSQLDBID}" no 25 "${TMP_AWS_REGION}"
        RETURNVAL="$?"
        if [ ${RETURNVAL} -ne 0 ]; then
            log_error "- MySQL Database failed to become available, cannot continue"
            return $RETURNVAL
        fi
    fi
    line_break

    log "- Waiting for MySQL Database to become available"
    rds_poll_status "${MYSQL_MYSQLDBID}" yes "${AWS_OPERATION_TIMEOUT}" "" "${TMP_AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "- MySQL Database failed to become available, cannot continue"
        return $RETURNVAL
    else
        log "- MySQL Database is now available"
    fi
    log "${FUNCTION_DESCRIPTION}: successfully created and configured database"
    line_break
    return 0
}

###------------------------------------------------------------------------------------------------
## FUNCTION: mysql_create_replica()
## - Creates MySQL Replica Database
## - Arguments
##   - $1: Iteration
##   - $2: Verification Timeout <in minutes, defaults to 30 minutes>
##   - $3: Region
function mysql_create_replica() {
    local FUNCTION_DESCRIPTION="MySQL (Create Replica)"
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

    verify_array_key_values "MYSQL_VARIABLES[@]" "MYSQL"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "${FUNCTION_DESCRIPTION}: Failed to load required variables [MySQL]"
        return $RETURNVAL
    fi
    verify_array_key_values "MYSQL_REPLICA_VARIABLES[@]" "MYSQL"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "${FUNCTION_DESCRIPTION}: Failed to load required variables [MySQL Replica]"
        return $RETURNVAL
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi
    TMP_REPLICA_ID="${MYSQL_MYSQLDBID}-replica-${TMP_ITERATION}"

    local ARG_DB_INSTANCE_IDENTIFIER="--db-instance-identifier ${TMP_REPLICA_ID}"
    local ARG_SOURCE_DB_INSTANCE_IDENTIFIER="--source-db-instance-identifier ${MYSQL_MYSQLDBID}"
    local ARG_DB_INSTANCE_CLASS="--db-instance-class ${MYSQL_REPLICADBINSTANCECLASS}"
    local ARG_PORT="--port ${MYSQL_MYSQLPORT}"
    local ARG_OPTION_GROUP_NAME="--option-group-name ${MYSQL_OPTIONGROUPNAME}"
    if option_enabled MYSQL_AUTOMINORVERSIONUPGRADE; then
        local ARG_MINOR_VERSION_UPGRADE="--auto-minor-version-upgrade"
    else
        local ARG_MINOR_VERSION_UPGRADE="--no-auto-minor-version-upgrade"
    fi
    local ARG_STORAGE_TYPE="--storage-type ${MYSQL_REPLICADBSTORAGETYPE}"
    if [ ${MYSQL_REPLICAIOPS} -gt 0 ]; then
        local ARG_IOPS="--iops ${MYSQL_REPLICAIOPS}"
    else
        local ARG_IOPS=""
    fi
    if [[ "ZZ_$(to_upper "${MYSQL_SUBNETTYPE}")" == "ZZ_PUBLIC" ]]; then
        local ARG_ACCESSIBILITY="--publicly-accessible"
    else
        local ARG_ACCESSIBILITY="--no-publicly-accessible"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"
    generate_temp_file FILE_TAGS_JSON "tags json"

    log "${FUNCTION_DESCRIPTION}: started (replica_id: [${TMP_REPLICA_ID}] / database_id: [${MYSQL_MYSQLDBID}])"

    log "${FUNCTION_DESCRIPTION}: Generating Tags JSON"
cat > ${FILE_TAGS_JSON} << ZZEOF
[
  { "Key": "Organization", "Value": "${MYSQL_ORGANIZATIONNAME}"},
  { "Key": "Project", "Value": "${MYSQL_PROJECTNAME}"},
  { "Key": "Function", "Value": "${MYSQL_FUNCTIONNAME}"},
  { "Key": "Environment", "Value": "${MYSQL_ENVIRONMENT}"},
  { "Key": "Owner", "Value": "${MYSQL_OWNER}"},
  { "Key": "Contact", "Value": "${MYSQL_CONTACT}"},
  { "Key": "DNS", "Value": "${MYSQL_MYSQLREPLICADNSRECORD}"}
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

    call_sleep 60 "allowing MySQL Replica Database time to warmup"
    line_break

    log_notice "Waiting for MySQL Replica Database to become available"
    rds_poll_status "${TMP_REPLICA_ID}" yes "${AWS_OPERATION_TIMEOUT}" "" "${TMP_AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "- MySQL Database failed to become available, cannot continue"
        return $RETURNVAL
    else
        log "- MySQL Database is now available"
    fi
    line_break

    log "- Waiting for MySQL Database to become available"
    rds_poll_status "${MYSQL_MYSQLDBID}" yes "${AWS_OPERATION_TIMEOUT}" "" "${TMP_AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "- MySQL Database failed to become available, cannot continue"
        return $RETURNVAL
    else
        log "- MySQL Database is now available"
    fi
    line_break

    rds_get_parameter_group TMP_CURRENT_PARAMETER_GROUP "${TMP_REPLICA_ID}" "${TMP_AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "- MySQL Database failed to retrieve parameter group, cannot continue"
        return $RETURNVAL
    fi
    if [[ "ZZ_${TMP_CURRENT_PARAMETER_GROUP}" != "ZZ_${MYSQL_REPLICAPARAMETERGROUPNAME}" ]]; then
        rds_set_parameter_group "${TMP_REPLICA_ID}" "${MYSQL_REPLICAPARAMETERGROUPNAME}" no "${TMP_AWS_REGION}"
        RETURNVAL="$?"
        if [ ${RETURNVAL} -ne 0 ]; then
            log_error "- failed to set parameter group, cannot continue"
            return $RETURNVAL
        fi
        call_sleep 30 "allowing MySQL Replica Database time to apply logic"
        line_break
    fi

    if (rds_is_db_pending_reboot "${TMP_REPLICA_ID}" "${TMP_AWS_REGION}"); then
        log_notice "- MySQL Database Replica requires additional reboot"
        rds_reboot "${TMP_REPLICA_ID}" no 25 "${TMP_AWS_REGION}"
        RETURNVAL="$?"
        if [ ${RETURNVAL} -ne 0 ]; then
            log_error "- MySQL Database failed to become available, cannot continue"
            return $RETURNVAL
        fi
    fi
    log "${FUNCTION_DESCRIPTION}: successfully created and configured replica database"
    line_break
    return 0
}

###------------------------------------------------------------------------------------------------
## FUNCTION: mysql_execute_sql()
## - Attempts to connect to specified MySQL Database in order to execute SQL file
## - Arguments
##   - $1: Database Endpoint
##   - $2: Username
##   - $3: Password
##   - $4: Database Name (optional, if not specified, it ignores this option)
##   - $5: Database Port (optional, if not specified, defaults to MYSQL_DEFAULT_PORT)
##   - $6: SQL File
function mysql_execute_sql() {
    local FUNCTION_DESCRIPTION="MySQL (Execute SQL)"
    local TMP_DATABASE_ENDPOINT="${1}"
    local TMP_DB_USERNAME="${2}"
    local TMP_DB_PASSWORD="${3}"
    local TMP_DATABASE_NAME="${4}"
    local TMP_DATABASE_PORT="${5}"
    local TMP_FILE_SQL="${6}"

    local RETURNVAL=""
    local ARG_DATABASE=""
    local LOG_FILE_MYSQL_OUTPUT=""
    local LOG_FILE_MYSQL_ERROR=""

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
    if [ ! -f "${TMP_FILE_SQL}" ]; then
        log_error "${FUNCTION_DESCRIPTION}: SQL file does not exist [${TMP_FILE_SQL}]"
        return $E_OBJECT_NOT_FOUND
    fi

    if(! is_empty "${TMP_DATABASE_NAME}"); then
        ARG_DATABASE="-D ${TMP_DATABASE_NAME}"
    fi
    if(is_empty "${TMP_DATABASE_PORT}"); then
        TMP_DATABASE_PORT="${MYSQL_DEFAULT_PORT}"
    fi

    generate_temp_file LOG_FILE_MYSQL_OUTPUT "mysql output"
    generate_temp_file LOG_FILE_MYSQL_ERROR "mysql error"

    log "${FUNCTION_DESCRIPTION}: started"
    log "- Endpoint: [${TMP_DATABASE_ENDPOINT}:${TMP_DATABASE_PORT}]"
    log "- Username: [${TMP_DB_USERNAME}]"
    log "- Password: [${TMP_DB_PASSWORD//?/*}]"
    log "- Database: [${TMP_DATABASE_NAME}]"
    log "- SQL File: [${TMP_FILE_SQL}]"

    $(which mysql) -h "${TMP_DATABASE_ENDPOINT}" -P "${TMP_DATABASE_PORT}" ${ARG_DATABASE} -u "${TMP_DB_USERNAME}" -p"${TMP_DB_PASSWORD}" --reconnect < "${TMP_FILE_SQL}" > ${LOG_FILE_MYSQL_OUTPUT} 2>${LOG_FILE_MYSQL_ERROR}
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_add_from_file "${LOG_FILE_MYSQL_OUTPUT}" "${FUNCTION_DESCRIPTION}: mysql output"
        log_add_from_file "${LOG_FILE_MYSQL_ERROR}" "${FUNCTION_DESCRIPTION}: mysql error"
        log_error "${FUNCTION_DESCRIPTION}: failed to execute SQL file (mysql_exit_code::${RETURNVAL}])"
        return $E_MYSQL_EXECUTION_FAILURE
    fi
    log_add_from_file "${LOG_FILE_MYSQL_OUTPUT}" "${FUNCTION_DESCRIPTION}: mysql output"
    log "${FUNCTION_DESCRIPTION}: successfully executed SQL file"
    sleep 0.1
    return 0
}

###------------------------------------------------------------------------------------------------
## FUNCTION: mysql_execute_sql_output_to_file()
## - Attempts to connect to specified MySQL Database in order to execute SQL file and send its output to specified file
## - Arguments
##   - $1: Database Endpoint
##   - $2: Username
##   - $3: Password
##   - $4: Database Name (optional, if not specified, it ignores this option)
##   - $5: Database Port (optional, if not specified, defaults to MYSQL_DEFAULT_PORT)
##   - $6: SQL File
##   - $7: Output File
function mysql_execute_sql_output_to_file() {
    local FUNCTION_DESCRIPTION="MySQL (Execute SQL)"
    local TMP_DATABASE_ENDPOINT="${1}"
    local TMP_DB_USERNAME="${2}"
    local TMP_DB_PASSWORD="${3}"
    local TMP_DATABASE_NAME="${4}"
    local TMP_DATABASE_PORT="${5}"
    local TMP_FILE_SQL="${6}"
    local TMP_FILE_OUTPUT="${7}"

    local RETURNVAL=""
    local ARG_DATABASE=""
    local LOG_FILE_MYSQL_OUTPUT=""
    local LOG_FILE_MYSQL_ERROR=""

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
    if [ ! -f "${TMP_FILE_SQL}" ]; then
        log_error "${FUNCTION_DESCRIPTION}: SQL file does not exist [${TMP_FILE_SQL}]"
        return $E_OBJECT_NOT_FOUND
    fi

    if(! is_empty "${TMP_DATABASE_NAME}"); then
        ARG_DATABASE="-D ${TMP_DATABASE_NAME}"
    fi
    if(is_empty "${TMP_DATABASE_PORT}"); then
        TMP_DATABASE_PORT="${MYSQL_DEFAULT_PORT}"
    fi

    generate_temp_file LOG_FILE_MYSQL_OUTPUT "mysql output"
    generate_temp_file LOG_FILE_MYSQL_ERROR "mysql error"

    log "${FUNCTION_DESCRIPTION}: started"
    log "- Endpoint: [${TMP_DATABASE_ENDPOINT}:${TMP_DATABASE_PORT}]"
    log "- Username: [${TMP_DB_USERNAME}]"
    log "- Password: [${TMP_DB_PASSWORD//?/*}]"
    log "- Database: [${TMP_DATABASE_NAME}]"
    log "- SQL File: [${TMP_FILE_SQL}]"

    $(which mysql) -sN -h "${TMP_DATABASE_ENDPOINT}" -P "${TMP_DATABASE_PORT}" ${ARG_DATABASE} -u "${TMP_DB_USERNAME}" -p"${TMP_DB_PASSWORD}" --reconnect < "${TMP_FILE_SQL}" > ${LOG_FILE_MYSQL_OUTPUT} 2>${LOG_FILE_MYSQL_ERROR}
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_add_from_file "${LOG_FILE_MYSQL_OUTPUT}" "${FUNCTION_DESCRIPTION}: mysql output"
        log_add_from_file "${LOG_FILE_MYSQL_ERROR}" "${FUNCTION_DESCRIPTION}: mysql error"
        log_error "${FUNCTION_DESCRIPTION}: failed to execute SQL file (mysql_exit_code::${RETURNVAL}])"
        return $E_MYSQL_EXECUTION_FAILURE
    fi
    log "${FUNCTION_DESCRIPTION}: Dumping output to file [${TMP_FILE_OUTPUT}]"
    cat "${LOG_FILE_MYSQL_OUTPUT}" > "${TMP_FILE_OUTPUT}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "${FUNCTION_DESCRIPTION}: failed to create output file"
        return $E_OBJECT_FAILED_TO_CREATE
    fi
    log "${FUNCTION_DESCRIPTION}: successfully executed SQL file"
    sleep 0.1
    return 0
}

###------------------------------------------------------------------------------------------------
## FUNCTION: mysql_set_dns()
## - Sets DNS record for MySQL Database
## - Arguments
##   - $1: Region
function mysql_set_dns() {
    local FUNCTION_DESCRIPTION="MySQL (Set DNS)"
    local TMP_AWS_REGION="${1}"

    local RETURNVAL=""
    local TMP_DB_ENDPOINT=""
    local TMP_KEY=""
    local TMP_VAR=""
    local KEY_MAX_LENGTH=0

    local TMP_ARRAY_REPLICA_IDS=()

    verify_array_key_values "MYSQL_DNS_VARIABLES[@]" "MYSQL"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "${FUNCTION_DESCRIPTION}: Failed to load required variables [MySQL DNS]"
        return $RETURNVAL
    fi
    line_break
    log_highlight "DNS Information"
    for TMP_KEY in "${MYSQL_DNS_VARIABLES[@]}"; do
        if [ ${#TMP_KEY} -gt $KEY_MAX_LENGTH ]; then
            KEY_MAX_LENGTH=${#TMP_KEY}
        fi
    done
    KEY_MAX_LENGTH=$((${KEY_MAX_LENGTH}+1))

    for TMP_KEY in "${MYSQL_DNS_VARIABLES[@]}"; do
        TMP_VAR="$(to_upper "MYSQL_${TMP_KEY}")"
        log "$(printf "%-1s %-${KEY_MAX_LENGTH}s %s" "-" "${TMP_KEY}:" "[${!TMP_VAR}]")"
    done
    line_break

    log_notice "${FUNCTION_DESCRIPTION}: Setting Primary DNS Record"
    rds_get_endpoint TMP_DB_ENDPOINT "${MYSQL_MYSQLDBID}" "${TMP_AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "- Failed to retrieve Endpoint Address, cannot continue"
        return $RETURNVAL
    fi
    log "- Endpoint Address: [${TMP_DB_ENDPOINT}]"
    route53_upsert_record "${MYSQL_DNSDATABASEHOSTEDZONEID}" "${MYSQL_MYSQLDNSRECORD}" "CNAME" ${AWS_RDS_DNS_TTL} "${TMP_DB_ENDPOINT}" "" "" "${TMP_AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "${FUNCTION_DESCRIPTION}: Failed to set Primary DNS Record"
        return $RETURNVAL
    fi

    log "${FUNCTION_DESCRIPTION}: Deleting Replica DNS records before setting, as it may have multiple weighted records"
    route53_delete_record "${MYSQL_DNSDATABASEHOSTEDZONEID}" "${MYSQL_MYSQLREPLICADNSRECORD}" "${TMP_AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "${FUNCTION_DESCRIPTION}: Failed to remove Replica DNS records"
        return $RETURNVAL
    fi
    log_notice "${FUNCTION_DESCRIPTION}: Setting Replica DNS Record"

    rds_get_database_replica_ids TMP_ARRAY_REPLICA_IDS "${MYSQL_MYSQLDBID}" "${TMP_AWS_REGION}"
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
                route53_upsert_record "${MYSQL_DNSDATABASEHOSTEDZONEID}" "${MYSQL_MYSQLREPLICADNSRECORD}" "CNAME" ${AWS_RDS_DNS_TTL} "${TMP_DB_ENDPOINT}" "1" "${TMP_KEY}" "${TMP_AWS_REGION}"
                RETURNVAL="$?"
                if [ ${RETURNVAL} -ne 0 ]; then
                    log_error "${FUNCTION_DESCRIPTION}: Failed to set Replica DNS Record [${TMP_KEY}]"
                    return $RETURNVAL
                fi
            fi
        done
    else
        log "- No Replicas found, setting Replica DNS Record to point to primary"
        route53_upsert_record "${MYSQL_DNSDATABASEHOSTEDZONEID}" "${MYSQL_MYSQLREPLICADNSRECORD}" "CNAME" ${AWS_RDS_DNS_TTL} "${TMP_DB_ENDPOINT}" "" "" "${TMP_AWS_REGION}"
        RETURNVAL="$?"
        if [ ${RETURNVAL} -ne 0 ]; then
            log_error "${FUNCTION_DESCRIPTION}: Failed to set Replica DNS Record"
            return $RETURNVAL
        fi
    fi

    return 0
}

###------------------------------------------------------------------------------------------------
## FUNCTION: mysql_verify_connection()
## - Attempts to connect to specified MySQL Database in order to verify if database is available and credentials are correct
## - Arguments
##   - $1: Database Endpoint
##   - $2: Username
##   - $3: Password
##   - $4: Database Port (optional, if not specified, defaults to MYSQL_DEFAULT_PORT)
function mysql_verify_connection() {
    local FUNCTION_DESCRIPTION="MySQL (Verify Connection)"
    local TMP_DATABASE_ENDPOINT="${1}"
    local TMP_DB_USERNAME="${2}"
    local TMP_DB_PASSWORD="${3}"
    local TMP_DATABASE_PORT="${4}"

    local RETURNVAL=""
    local FILE_MYSQL_OUTPUT=""

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
    if(is_empty "${TMP_DATABASE_PORT}"); then
        TMP_DATABASE_PORT="${MYSQL_DEFAULT_PORT}"
    fi

    generate_temp_file FILE_MYSQL_OUTPUT "mysql output"

    $(which mysql) -h"${TMP_DATABASE_ENDPOINT}" -P"${TMP_DATABASE_PORT}" -u"${TMP_DB_USERNAME}" -p"${TMP_DB_PASSWORD}" --reconnect -e 'SHOW DATABASES;' >${FILE_MYSQL_OUTPUT} 2>&1
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log "${FUNCTION_DESCRIPTION}: Database [${TMP_DATABASE_ENDPOINT}] - [$(color_text "${LOG_COLOR_ERROR}" "FAILED")] (mysql_exit_code::${RETURNVAL}])"
        log_add_from_file "${FILE_MYSQL_OUTPUT}" "${FUNCTION_DESCRIPTION}: mysql output"
        return $E_DATABASE_CONNECTION_FAILURE
    fi
    log_add_from_file "${FILE_MYSQL_OUTPUT}" "${FUNCTION_DESCRIPTION}: mysql database stats"
    log "${FUNCTION_DESCRIPTION}: Database [${TMP_DATABASE_ENDPOINT}] - [$(color_text "${LOG_COLOR_SUCCESS}" "OK")]"
    return 0
}
