###------------------------------------------------------------------------------------------------
# script:  loadbalancer.inc
# purpose: Collection of functions related to AWS (LoadBalancer)
# version: 1.0.0
#
# function list:
#          - elbv2_deregister_target
#          - elbv2_register_target
###------------------------------------------------------------------------------------------------
REQUIRED_EXECUTABLES+=('aws')

###------------------------------------------------------------------------------------------------
# Variables
AWS_LOADBALANCER_DEFAULT_RETRY_COUNT=3
AWS_LOADBALANCER_DEFAULT_RETRY_TIMER_MAX_SEC=30

###------------------------------------------------------------------------------------------------
## FUNCTION: elbv2_deregister_target()
## - Deregisters Instance from LoadBalancer Target Group (ELB v2)
## - Arguments
##   - $1: Target Group ARN
##   - $2: Instance ID
##   - $3: Region
function elbv2_deregister_target() {
    local FUNCTION_DESCRIPTION="ELB v2 (Deregister Target)"
    local TARGET_GROUP_ARN="${1}"
    local INSTANCE_ID="${2}"
    local TMP_AWS_REGION="${3}"

    local RETURNVAL=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    if(is_empty "${TARGET_GROUP_ARN}"); then
        log_error "${FUNCTION_DESCRIPTION}: Target Group ARN Name not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${INSTANCE_ID}"); then
        log_error "${FUNCTION_DESCRIPTION}: Instance ID not specified"
        return $E_BAD_ARGS
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    log_notice "${FUNCTION_DESCRIPTION}: started"
    log "- Target Group ARN: [${TARGET_GROUP_ARN}]"
    log "- Instance ID:      [${INSTANCE_ID}]"
    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_LOADBALANCER_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_LOADBALANCER_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_LOADBALANCER_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        log "${FUNCTION_DESCRIPTION}: Attempting to deregister target (Attempt::${COUNTER} of ${AWS_LOADBALANCER_DEFAULT_RETRY_COUNT})"
        $(which aws) ${AWS_REGION_STRING} elbv2 deregister-targets --target-group-arn ${TARGET_GROUP_ARN} --targets Id=${INSTANCE_ID} >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
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
            log "${FUNCTION_DESCRIPTION}: successfully deregistered target"
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: elbv2_register_target()
## - Registers Instance to LoadBalancer Target Group (ELB v2)
## - Arguments
##   - $1: Target Group ARN
##   - $2: Instance ID
##   - $3: Region
function elbv2_register_target() {
    local FUNCTION_DESCRIPTION="ELB v2 (Register Target)"
    local TARGET_GROUP_ARN="${1}"
    local INSTANCE_ID="${2}"
    local TMP_AWS_REGION="${3}"

    local RETURNVAL=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    if(is_empty "${TARGET_GROUP_ARN}"); then
        log_error "${FUNCTION_DESCRIPTION}: Target Group ARN Name not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${INSTANCE_ID}"); then
        log_error "${FUNCTION_DESCRIPTION}: Instance ID not specified"
        return $E_BAD_ARGS
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    log_notice "${FUNCTION_DESCRIPTION}: started"
    log "- Target Group ARN: [${TARGET_GROUP_ARN}]"
    log "- Instance ID:      [${INSTANCE_ID}]"
    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_LOADBALANCER_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_LOADBALANCER_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_LOADBALANCER_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        log "${FUNCTION_DESCRIPTION}: Attempting to register target (Attempt::${COUNTER} of ${AWS_LOADBALANCER_DEFAULT_RETRY_COUNT})"
        $(which aws)${AWS_REGION_STRING} elbv2 register-targets --target-group-arn ${TARGET_GROUP_ARN} --targets Id=${INSTANCE_ID} >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
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
            log "${FUNCTION_DESCRIPTION}: successfully registered target"
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}
