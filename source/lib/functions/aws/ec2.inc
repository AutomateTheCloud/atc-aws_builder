###------------------------------------------------------------------------------------------------
# script:  ec2.inc
# purpose: Collection of functions related to EC2
# version: 1.0.0
#
# function list:
#          - ec2_get_availability_zone
#          - ec2_get_ipaddress_private
#          - ec2_harden_default_security_group
#          - ec2_import_keypair
#          - ec2_instance_connect_send_ssh_key
#          - ec2_source_destination_check_disable
#          - ec2_tag_resource
#          - ec2_tag_resource_from_file
#          - ec2_vpc_route_update
###------------------------------------------------------------------------------------------------
REQUIRED_EXECUTABLES+=('aws')

###------------------------------------------------------------------------------------------------
# Variables
AWS_EC2_DEFAULT_RETRY_COUNT=3
AWS_EC2_DEFAULT_RETRY_TIMER_MAX_SEC=30

###------------------------------------------------------------------------------------------------
## FUNCTION: ec2_get_availability_zone()
## - Returns Availability Zone of specified Instance
## - Arguments
##   - $1: Variable name to pass info to
##   - $2: Instance ID
##   - $3: Region
function ec2_get_availability_zone() {
    local FUNCTION_DESCRIPTION="EC2 (Get Availability Zone)"
    local TMP_STRING=""
    local __RESULTVAR=$1
    local TMP_INSTANCE_ID=$2
    local TMP_AWS_REGION=$3

    local TMP_FILE_AWS_ERROR=""

    if(is_empty "${TMP_INSTANCE_ID}"); then
        log_error "${FUNCTION_DESCRIPTION}: Instance ID not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_AWS_REGION}"); then
        log_error "${FUNCTION_DESCRIPTION}: AWS Region not specified"
        return $E_BAD_ARGS
    fi

    generate_temp_file TMP_FILE_AWS_ERROR "aws error log"

    TMP_STRING="$($(which aws) --region ${TMP_AWS_REGION} ec2 describe-instances --instance-ids "${TMP_INSTANCE_ID}" --query 'Reservations[*].Instances[*].Placement.AvailabilityZone' --output text 2>${TMP_FILE_AWS_ERROR})"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_warning "${FUNCTION_DESCRIPTION}: Failed to retrieve Availability Zone"
        log_add_from_file "${TMP_FILE_AWS_ERROR}" "aws_cli error file"
        return $E_AWS_FAILURE
    fi
    eval $__RESULTVAR="'${TMP_STRING}'"
}

###------------------------------------------------------------------------------------------------
## FUNCTION: ec2_get_ipaddress_private()
## - Returns Private IP Address of specified Instance
## - Arguments
##   - $1: Variable name to pass info to
##   - $2: Instance ID
##   - $3: Region
function ec2_get_ipaddress_private() {
    local FUNCTION_DESCRIPTION="EC2 (Get Private IP Address)"
    local TMP_STRING=""
    local __RESULTVAR=$1
    local TMP_INSTANCE_ID=$2
    local TMP_AWS_REGION=$3

    local TMP_FILE_AWS_ERROR=""

    if(is_empty "${TMP_INSTANCE_ID}"); then
        log_error "${FUNCTION_DESCRIPTION}: Instance ID not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_AWS_REGION}"); then
        log_error "${FUNCTION_DESCRIPTION}: AWS Region not specified"
        return $E_BAD_ARGS
    fi

    generate_temp_file TMP_FILE_AWS_ERROR "aws error log"

    TMP_STRING="$($(which aws) --region ${TMP_AWS_REGION} ec2 describe-instances --instance-ids "${TMP_INSTANCE_ID}" --query 'Reservations[*].Instances[*].PrivateIpAddress' --output text 2>${TMP_FILE_AWS_ERROR})"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_warning "${FUNCTION_DESCRIPTION}: Failed to retrieve Availability Zone"
        log_add_from_file "${TMP_FILE_AWS_ERROR}" "aws_cli error file"
        return $E_AWS_FAILURE
    fi
    eval $__RESULTVAR="'${TMP_STRING}'"
}

###------------------------------------------------------------------------------------------------
## FUNCTION: ec2_harden_default_security_group()
## - Hardens Default Security Group by removing all rules, ensure all traffic is denied
## - Arguments
##   - $1: Security Group ID
##   - $2: Region
function ec2_harden_default_security_group() {
    local FUNCTION_DESCRIPTION="Harden Default Security Group (EC2)"
    local TMP_SECURITY_GROUP_ID="${1}"
    local TMP_AWS_REGION="${2}"

    local RETURNVAL=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    if(is_empty "${TMP_SECURITY_GROUP_ID}"); then
        log_error "${FUNCTION_DESCRIPTION}: Security Group ID not specified"
        return $E_BAD_ARGS
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    log_notice "${FUNCTION_DESCRIPTION}: started"
    log "- Security Group ID: [${TMP_SECURITY_GROUP_ID}]"
    log "- Region:            [${TMP_AWS_REGION}]"

    log "${FUNCTION_DESCRIPTION}: removing Ingress rules from Security Group"
    $(which aws) ${AWS_REGION_STRING} ec2 revoke-security-group-ingress --group-id ${TMP_SECURITY_GROUP_ID} --protocol "-1" --source-group ${TMP_SECURITY_GROUP_ID} >/dev/null 2>&1
    log "${FUNCTION_DESCRIPTION}: removing Egress rules from Security Group"
    $(which aws) ${AWS_REGION_STRING} ec2 revoke-security-group-egress --group-id ${TMP_SECURITY_GROUP_ID} --ip-permissions '[{"IpProtocol": "-1", "IpRanges": [{"CidrIp": "0.0.0.0/0"}]}]' >/dev/null 2>&1
    $(which aws) ${AWS_REGION_STRING} ec2 revoke-security-group-egress --group-id ${TMP_SECURITY_GROUP_ID} --ip-permissions '[{"IpProtocol": "-1", "Ipv6Ranges": [{"CidrIpv6": "::/0"}]}]' >/dev/null 2>&1

    return 0
}

###------------------------------------------------------------------------------------------------
## FUNCTION: ec2_import_keypair()
## - Uploads SSH Public Key for use with EC2 Instances
## - Arguments
##   - $1: Instance ID
##   - $2: Region
function ec2_import_keypair() {
    local FUNCTION_DESCRIPTION="EC2 Import Keypair"
    local TMP_KEYPAIR_NAME="${1}"
    local TMP_FILE_KEYPAIR_PUB="${2}"
    local TMP_AWS_REGION="${3}"

    local RETURNVAL=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    if(is_empty "${TMP_KEYPAIR_NAME}"); then
        log_error "${FUNCTION_DESCRIPTION}: Keypair Name not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_FILE_KEYPAIR_PUB}"); then
        log_error "${FUNCTION_DESCRIPTION}: Keypair Pub file not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_AWS_REGION}"); then
        log_error "${FUNCTION_DESCRIPTION}: AWS Region not specified"
        return $E_BAD_ARGS
    fi

    if [ ! -f "${TMP_FILE_KEYPAIR_PUB}" ]; then
        log_error "${FUNCTION_DESCRIPTION}: Keypair Pub file does not exist [${TMP_FILE_KEYPAIR_PUB}]"
        return $E_OBJECT_NOT_FOUND
    fi

    AWS_REGION_STRING="--region ${TMP_AWS_REGION}"

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    log_notice "${FUNCTION_DESCRIPTION}: started"
    log "- Keypair Name:     [${TMP_KEYPAIR_NAME}]"
    log "- Keypair Pub File: [${TMP_FILE_KEYPAIR_PUB}]"
    log "- Region:           [${TMP_AWS_REGION}]"

    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_EC2_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_EC2_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_EC2_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        log "${FUNCTION_DESCRIPTION}: Uploading SSH Public Key (Attempt::${COUNTER} of ${AWS_EC2_DEFAULT_RETRY_COUNT})"
        $(which aws) ${AWS_REGION_STRING} ec2 import-key-pair --public-key-material file://${TMP_FILE_KEYPAIR_PUB} --key-name "${TMP_KEYPAIR_NAME}" >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
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
            log "${FUNCTION_DESCRIPTION}: successfully uploaded SSH Public Key [${TMP_KEYPAIR_NAME}]"
            call_sleep 1
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: ec2_instance_connect_send_ssh_key()
## - EC2 Instance Connect - Sends SSH Key for specified user to specified EC2 Instance
## - Arguments
##   - $1: Instance ID
##   - $2: OS User
##   - $3: Region
##   - $4: Availability Zone
##   - $5: SSH Public Key
function ec2_instance_connect_send_ssh_key() {
    local FUNCTION_DESCRIPTION="EC2 Instance Connect (Send SSH Key)"
    local TMP_INSTANCE_ID=$1
    local TMP_OS_USER=$2
    local TMP_AWS_REGION=$3
    local TMP_AWS_AVAILABILITY_ZONE=$4
    local TMP_FILE_SSH_KEY_PUB=$5

    local TMP_FILE_AWS_OUTPUT=""

    if(is_empty "${TMP_INSTANCE_ID}"); then
        log_error "${FUNCTION_DESCRIPTION}: Instance ID not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_OS_USER}"); then
        log_error "${FUNCTION_DESCRIPTION}: OS User not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_AWS_REGION}"); then
        log_error "${FUNCTION_DESCRIPTION}: AWS Region not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_AWS_AVAILABILITY_ZONE}"); then
        log_error "${FUNCTION_DESCRIPTION}: AWS Availability Zone not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_FILE_SSH_KEY_PUB}"); then
        log_error "${FUNCTION_DESCRIPTION}: SSH Public Key file not specified"
        return $E_BAD_ARGS
    fi
    if [ ! -f "${TMP_FILE_SSH_KEY_PUB}" ]; then
        log_error "- ${FUNCTION_DESCRIPTION}: SSH Public Key file does not exist [${TMP_FILE_SSH_KEY_PUB}]"
        return $E_OBJECT_NOT_FOUND
    fi

    generate_temp_file TMP_FILE_AWS_OUTPUT "aws output log"

    log_notice "${FUNCTION_DESCRIPTION}: started"
    log "- Instance ID:       [${TMP_INSTANCE_ID}]"
    log "- OS User:           [${TMP_OS_USER}]"
    log "- Region:            [${TMP_AWS_REGION}]"
    log "- Availability Zone: [${TMP_AWS_AVAILABILITY_ZONE}]"
    log "- SSH Public Key:    [${TMP_FILE_SSH_KEY_PUB}]"


    $(which aws) --region ${TMP_AWS_REGION} ec2-instance-connect send-ssh-public-key --instance-id "${TMP_INSTANCE_ID}" --availability-zone "${TMP_AWS_AVAILABILITY_ZONE}" --instance-os-user "${TMP_OS_USER}" --ssh-public-key file://${TMP_FILE_SSH_KEY_PUB} > ${TMP_FILE_AWS_OUTPUT}
    RETURNVAL="$?"
    log_add_from_file "${TMP_FILE_AWS_OUTPUT}" "aws_cli output"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "${FUNCTION_DESCRIPTION}: Failed to Sends SSH Public Key"
        return $E_AWS_FAILURE
    fi
    return 0
}

###------------------------------------------------------------------------------------------------
## FUNCTION: ec2_source_destination_check_disable()
## - Disables Source Destination Check on specified instance
## - Arguments
##   - $1: Instance ID
##   - $2: Region
function ec2_source_destination_check_disable() {
    local FUNCTION_DESCRIPTION="Disable Source Destination Check"
    local TMP_INSTANCE_ID="${1}"
    local TMP_AWS_REGION="${2}"

    local RETURNVAL=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    if(is_empty "${TMP_INSTANCE_ID}"); then
        log_error "${FUNCTION_DESCRIPTION}: Instance ID not specified"
        return $E_BAD_ARGS
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    log_notice "${FUNCTION_DESCRIPTION}: started"
    log "- Instance ID: [${TMP_INSTANCE_ID}]"
    log "- Region:      [${TMP_AWS_REGION}]"

    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_EC2_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_EC2_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_EC2_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        log "${FUNCTION_DESCRIPTION}: Attempting to disable Source Destination Check on instance (Attempt::${COUNTER} of ${AWS_EC2_DEFAULT_RETRY_COUNT})"
        $(which aws) ${AWS_REGION_STRING} ec2 modify-instance-attribute --instance-id ${TMP_INSTANCE_ID} --no-source-dest-check >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
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
            log "${FUNCTION_DESCRIPTION}: successfully disabled Source Destination Check on instance"
            call_sleep 1
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: ec2_tag_resource()
## - Tags Resource (EC2) with specified Key and Value
## - Arguments
##   - $1: Resource ID
##   - $2: Key
##   - $3: Value
##   - $4: Region
function ec2_tag_resource() {
    local FUNCTION_DESCRIPTION="Tag Resource (EC2)"
    local TMP_RESOURCE_ID="${1}"
    local TMP_KEY="${2}"
    local TMP_VALUE="${3}"
    local TMP_AWS_REGION="${4}"

    local TMP_TAG_JSON=""
    local RETURNVAL=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    if(is_empty "${TMP_RESOURCE_ID}"); then
        log_error "${FUNCTION_DESCRIPTION}: Resource ID not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_KEY}"); then
        log_error "${FUNCTION_DESCRIPTION}: Key not specified"
        return $E_BAD_ARGS
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    TMP_TAG_JSON="[{ \"Key\": \"${TMP_KEY}\", \"Value\": \"${TMP_VALUE}\" }]"
    log_notice "${FUNCTION_DESCRIPTION}: started"
    log "- Resource ID: [${TMP_RESOURCE_ID}]"
    log "- Key:         [${TMP_KEY}]"
    log "- Value:       [${TMP_VALUE}]"

    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_EC2_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_EC2_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_EC2_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        log "${FUNCTION_DESCRIPTION}: Attempting to tag resource (Attempt::${COUNTER} of ${AWS_EC2_DEFAULT_RETRY_COUNT})"
        $(which aws) ${AWS_REGION_STRING} ec2 create-tags --resources ${TMP_RESOURCE_ID} --tags "${TMP_TAG_JSON}" >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
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
            log "${FUNCTION_DESCRIPTION}: successfully tagged resource"
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###--------------------------------------------------------------------------------
## FUNCTION: ec2_tag_resource_from_file()
## - Tags Resource (EC2) with values from specified file
## - Arguments
##   - $1: Resource ID
##   - $2: Tags File
##   - $3: Region
function ec2_tag_resource_from_file() {
    local FUNCTION_DESCRIPTION="Tag Resource (EC2) from File"
    local TMP_RESOURCE_ID="${1}"
    local TMP_FILE_TAGS="${2}"
    local TMP_AWS_REGION="${3}"

    local RETURNVAL=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    if(is_empty "${TMP_RESOURCE_ID}"); then
        log_error "${FUNCTION_DESCRIPTION}: Resource ID not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_FILE_TAGS}"); then
        log_error "${FUNCTION_DESCRIPTION}: Tags file specified"
        return $E_BAD_ARGS
    fi
    if [ ! -f "${TMP_FILE_TAGS}" ]; then
        log_error "${FUNCTION_DESCRIPTION}: Tags file does not exist [${TMP_FILE_TAGS}]"
        return $E_OBJECT_NOT_FOUND
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    log "${FUNCTION_DESCRIPTION}: started"
    log "- Resource ID: [${TMP_RESOURCE_ID}]"
    log "- Tags File:   [${TMP_FILE_TAGS}]"

    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_EC2_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_EC2_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_EC2_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        log "${FUNCTION_DESCRIPTION}: Attempting to tag resource (Attempt::${COUNTER} of ${AWS_EC2_DEFAULT_RETRY_COUNT})"
        $(which aws) ${AWS_REGION_STRING} ec2 create-tags --resources ${TMP_RESOURCE_ID} --tags file://${TMP_FILE_TAGS} >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
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
            log "${FUNCTION_DESCRIPTION}: successfully tagged resource"
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: ec2_vpc_route_update()
## - Updates VPC Route Table to route through specified Instance
## - Arguments
##   - $1: Route Table ID
##   - $2: Instance ID
##   - $3: Region
function ec2_vpc_route_update() {
    local FUNCTION_DESCRIPTION="Update VPC Route"
    local TMP_ROUTE_TABLE_ID="${1}"
    local TMP_INSTANCE_ID="${2}"
    local TMP_AWS_REGION="${3}"

    local RETURNVAL=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    if(is_empty "${TMP_ROUTE_TABLE_ID}"); then
        log_error "${FUNCTION_DESCRIPTION}: Route Table ID not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_INSTANCE_ID}"); then
        log_error "${FUNCTION_DESCRIPTION}: Instance ID not specified"
        return $E_BAD_ARGS
    fi
    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    log_notice "${FUNCTION_DESCRIPTION}: started"
    log "- Route Table ID: [${TMP_ROUTE_TABLE_ID}]"
    log "- Instance ID:    [${TMP_INSTANCE_ID}]"
    log "- Region:         [${TMP_AWS_REGION}]"

    log "${FUNCTION_DESCRIPTION}: replacing route"
    $(which aws) ${AWS_REGION_STRING} ec2 replace-route --route-table-id "${TMP_ROUTE_TABLE_ID}" --instance-id "${TMP_INSTANCE_ID}" --destination-cidr-block "0.0.0.0/0" > ${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
    RETURNVAL="$?"
    # Fix Response File
    $(which sed) -i "s/\o15/_AWS_BAD_IGNORE\\n/g" "${AWS_FILE_RESPONSE}"
    $(which sed) -i '/_AWS_BAD_IGNORE/d' "${AWS_FILE_RESPONSE}"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_warning "- Failed to replace route, attempting to create instead"
        $(which aws) ${AWS_REGION_STRING} ec2 create-route --route-table-id "${TMP_ROUTE_TABLE_ID}" --instance-id "${TMP_INSTANCE_ID}" --destination-cidr-block "0.0.0.0/0" > ${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
        RETURNVAL="$?"
        # Fix Response File
        $(which sed) -i "s/\o15/_AWS_BAD_IGNORE\\n/g" "${AWS_FILE_RESPONSE}"
        $(which sed) -i '/_AWS_BAD_IGNORE/d' "${AWS_FILE_RESPONSE}"
        if [ ${RETURNVAL} -ne 0 ]; then
            log_add_from_file "${AWS_FILE_ERROR}" "${FUNCTION_DESCRIPTION}: Data containing error"
            log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Data"
            log_error "${FUNCTION_DESCRIPTION}: operation failed (aws_cli_exit_code::${RETURNVAL}])"
            return $E_AWS_FAILURE
        fi
    fi

    log "${FUNCTION_DESCRIPTION}: route successfully updated"
    return 0
}
