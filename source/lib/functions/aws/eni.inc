###------------------------------------------------------------------------------------------------
# script:  eni.inc
# purpose: Collection of functions related to ENI
# version: 1.0.0
#
# function list:
#          - eni_attach
#          - eni_detach
#          - eni_return_attachment_id
#          - eni_return_status
###------------------------------------------------------------------------------------------------
REQUIRED_EXECUTABLES+=('aws')

###------------------------------------------------------------------------------------------------
# Variables
AWS_ENI_DEFAULT_RETRY_COUNT=3
AWS_ENI_DEFAULT_RETRY_TIMER_MAX_SEC=30

###------------------------------------------------------------------------------------------------
## FUNCTION: eni_attach()
## - Attach ENI
## - Arguments
##   - $1: ENI Name
##   - $2: Instance ID
##   - $3: Device ID (optional, defaults to 3)
##   - $4: Region
function eni_attach() {
    local FUNCTION_DESCRIPTION="ENI (Attach)"
    local ENI_NAME="${1}"
    local INSTANCE_ID="${2}"
    local DEVICE_ID="${3}"
    local TMP_AWS_REGION="${4}"

    local RETURNVAL=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    if(is_empty "${ENI_NAME}"); then
        log_error "${FUNCTION_DESCRIPTION}: ENI Name not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${INSTANCE_ID}"); then
        log_error "${FUNCTION_DESCRIPTION}: Instance ID not specified"
        return $E_BAD_ARGS
    fi

    if(is_empty "${DEVICE_ID}"); then
        DEVICE_ID=3
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    log_notice "${FUNCTION_DESCRIPTION}: started"
    log "- ENI Name:    [${ENI_NAME}]"
    log "- Instance ID: [${INSTANCE_ID}]"
    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_ENI_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_ENI_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_ENI_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        log "${FUNCTION_DESCRIPTION}: Attempting to attach ENI (Attempt::${COUNTER} of ${AWS_ENI_DEFAULT_RETRY_COUNT})"
        $(which aws) ${AWS_REGION_STRING} ec2 detach-network-interface --attachment-id ${ENI_ATTACHMENT_ID} --force >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
        call_sleep 5
        $(which aws) ${AWS_REGION_STRING} ec2 attach-network-interface --network-interface-id ${ENI_NAME} --instance-id ${INSTANCE_ID} --device-index ${DEVICE_ID} >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
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
            log "${FUNCTION_DESCRIPTION}: successfully attached ENI"
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: eni_detach()
## - Detach ENI
## - Arguments
##   - $1: ENI Attachment ID
##   - $2: Region
function eni_detach() {
    local FUNCTION_DESCRIPTION="ENI (Detach)"
    local ENI_ATTACHMENT_ID="${1}"
    local TMP_AWS_REGION="${2}"

    local RETURNVAL=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    if(is_empty "${ENI_ATTACHMENT_ID}"); then
        log_error "${FUNCTION_DESCRIPTION}: ENI Attachment ID not specified"
        return $E_BAD_ARGS
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    log_notice "${FUNCTION_DESCRIPTION}: started [eni_attachment_id::${ENI_ATTACHMENT_ID}]"
    log "- ENI Attachement ID: [${ENI_ATTACHMENT_ID}]"
    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_ENI_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_ENI_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_ENI_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        log "${FUNCTION_DESCRIPTION}: Attempting to detach ENI (Attempt::${COUNTER} of ${AWS_ENI_DEFAULT_RETRY_COUNT})"
        $(which aws) ${AWS_REGION_STRING} ec2 detach-network-interface --attachment-id ${ENI_ATTACHMENT_ID} --force >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
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
            log "${FUNCTION_DESCRIPTION}: successfully detached ENI"
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: eni_return_attachment_id()
## - Returns Attachment ID associated with ENI
## - Arguments
##   - $1: Variable name to pass info to
##   - $2: ENI Name
##   - $3: Region
function eni_return_attachment_id() {
    local __RESULTVAR=$1
    local AWS_ENI_NAME="${2}"
    local TMP_AWS_REGION="${3}"

    local TMP_STRING=""
    local AWS_REGION_STRING=""

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    TMP_STRING="$($(which aws) ${AWS_REGION_STRING} ec2 describe-network-interfaces --network-interface-ids ${AWS_ENI_NAME} 2>/dev/null | grep "AttachmentId" | sed -e 's/{//g' -e 's/}//g' | cut -d',' -f1 | awk -F ": " '{print $2}' | cut -d '"' -f2)"
    eval $__RESULTVAR="'${TMP_STRING}'"
}

###------------------------------------------------------------------------------------------------
## FUNCTION: eni_return_status()
## - Returns Status associated with ENI
## - Arguments
##   - $1: Variable name to pass info to
##   - $2: ENI Name
##   - $3: Region
function eni_return_status() {
    local __RESULTVAR=$1
    local AWS_ENI_NAME="${2}"
    local TMP_AWS_REGION="${3}"

    local TMP_STRING=""
    local AWS_REGION_STRING=""

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    TMP_STRING="$($(which aws) ${AWS_REGION_STRING} ec2 describe-network-interfaces --network-interface-ids ${AWS_ENI_NAME} 2>/dev/null | grep "Status" | head -1 | sed -e 's/{//g' -e 's/}//g' | cut -d',' -f1 | awk -F ": " '{print $2}' | cut -d '"' -f2)"
    eval $__RESULTVAR="'${TMP_STRING}'"
}
