###------------------------------------------------------------------------------------------------
# script:  vpc.inc
# purpose: Collection of functions related to AWS (VPC)
# version: 1.0.0
#
# function list:
#          - vpc_peering_connection_enable_dns
###------------------------------------------------------------------------------------------------
REQUIRED_EXECUTABLES+=('aws')

###------------------------------------------------------------------------------------------------
# Variables
AWS_VPC_DEFAULT_RETRY_COUNT=3
AWS_VPC_DEFAULT_RETRY_TIMER_MAX_SEC=30

###------------------------------------------------------------------------------------------------
## FUNCTION: vpc_peering_connection_enable_dns()
## - Enables DNS Resolution between VPCs for VPC Peering Connection
## - Arguments
##   - $1: Resource ID
##   - $2: Region
function vpc_peering_connection_enable_dns() {
    local FUNCTION_DESCRIPTION="VPC Peering Connection (Enable DNS)"
    local TMP_RESOURCE_ID="${1}"
    local TMP_AWS_REGION="${2}"

    local RETURNVAL=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    if(is_empty "${TMP_RESOURCE_ID}"); then
        log_error "${FUNCTION_DESCRIPTION}: VPC Peering Connection ID not specified"
        return $E_BAD_ARGS
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    log_notice "${FUNCTION_DESCRIPTION}: started"
    log "- VPC Peering Connection ID: [${TMP_RESOURCE_ID}]"

    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_VPC_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_VPC_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_VPC_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        log "${FUNCTION_DESCRIPTION}: Attempting to modify resource (Attempt::${COUNTER} of ${AWS_VPC_DEFAULT_RETRY_COUNT})"
        $(which aws) ${AWS_REGION_STRING} ec2 modify-vpc-peering-connection-options --vpc-peering-connection-id ${TMP_RESOURCE_ID} --requester-peering-connection-options AllowDnsResolutionFromRemoteVpc=true --accepter-peering-connection-options AllowDnsResolutionFromRemoteVpc=true >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
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
            log "${FUNCTION_DESCRIPTION}: successfully modified resource"
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}
