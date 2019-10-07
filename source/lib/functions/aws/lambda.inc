###------------------------------------------------------------------------------------------------
# script:  lambda.inc
# purpose: Collection of functions related to Lambda
# version: 1.0.0
#
# function list:
#          - create_lambda_function
#          - delete_lambda_function
#          - return_lambda_arn
###------------------------------------------------------------------------------------------------
REQUIRED_EXECUTABLES+=('aws')

###------------------------------------------------------------------------------------------------
# Variables
AWS_LAMBDA_DEFAULT_RETRY_COUNT=3
AWS_LAMBDA_DEFAULT_RETRY_TIMER_MAX_SEC=30

LAMBDA_FILE_BASE_LAMBDA_JSON="lambda.json"
LAMBDA_FILE_BASE_POLICY_JSON="policy.json"
LAMBDA_FILE_BASE_TRUST_POLICY_JSON="trust_policy.json"

###------------------------------------------------------------------------------------------------
## FUNCTION: create_lambda_function()
## - Creates Lambda Function
## - Arguments
##   - $1: Lambda Function Name
##   - $2: Lambda json file
##   - $3: Region
function create_lambda_function() {
    local FUNCTION_DESCRIPTION="Lambda (Create Function)"
    local LAMBDA_FUNCTION_NAME="${1}"
    local FILE_LAMBDA_JSON="${2}"
    local TMP_AWS_REGION="${3}"

    local RETURNVAL=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    if(is_empty "${LAMBDA_FUNCTION_NAME}"); then
        log_error "${FUNCTION_DESCRIPTION}: Lambda Function Name not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${FILE_LAMBDA_JSON}"); then
        log_error "${FUNCTION_DESCRIPTION}: Lambda File JSON not specified"
        return $E_BAD_ARGS
    fi
    if [ ! -f "${FILE_LAMBDA_JSON}" ]; then
        log_error "${FUNCTION_DESCRIPTION}: Lambda File JSON not found [${FILE_LAMBDA_JSON}]"
        return $E_OBJECT_NOT_FOUND
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    log "${FUNCTION_DESCRIPTION}: started"
    log "- Lambda Function:  [${LAMBDA_FUNCTION_NAME}]"
    log "- Lambda File JSON: [${FILE_LAMBDA_JSON}]"
    log "- Region:           [${TMP_AWS_REGION}]"
    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_LAMBDA_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_LAMBDA_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_LAMBDA_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        log "${FUNCTION_DESCRIPTION}: Creating Lambda Function (Attempt::${COUNTER} of ${AWS_LAMBDA_DEFAULT_RETRY_COUNT})"
        $(which aws) ${AWS_REGION_STRING} lambda create-function --function-name "${LAMBDA_FUNCTION_NAME}" --cli-input-json file://${FILE_LAMBDA_JSON} >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
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
            log_success "${FUNCTION_DESCRIPTION}: Successfully created Lambda Function"
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: delete_lambda_function()
## - Deletes Lambda Function
## - Arguments
##   - $1: Lambda Function Name
##   - $2: Region
function delete_lambda_function() {
    local FUNCTION_DESCRIPTION="Lambda (Delete Function)"
    local LAMBDA_FUNCTION_NAME="${1}"
    local TMP_AWS_REGION="${2}"

    local RETURNVAL=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    if(is_empty "${LAMBDA_FUNCTION_NAME}"); then
        log_error "${FUNCTION_DESCRIPTION}: Lambda Function Name not specified"
        return $E_BAD_ARGS
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    log "${FUNCTION_DESCRIPTION}: started"
    log "- Lambda Function:  [${LAMBDA_FUNCTION_NAME}]"
    log "- Region:           [${TMP_AWS_REGION}]"
    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_LAMBDA_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_LAMBDA_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_LAMBDA_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        log "${FUNCTION_DESCRIPTION}: Deleting Lambda Function (Attempt::${COUNTER} of ${AWS_LAMBDA_DEFAULT_RETRY_COUNT})"
        $(which aws) ${AWS_REGION_STRING} lambda delete-function --function-name "${LAMBDA_FUNCTION_NAME}" >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
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
            log_success "${FUNCTION_DESCRIPTION}: Successfully deleted Lambda Function"
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: return_lambda_arn()
## - Returns Lambda Function ARN
## - Arguments
##   - $1: Variable name to pass info to
##   - $2: Lambda Function Name
##   - $3: Region
function return_lambda_arn() {
    local __RESULTVAR=$1
    local LAMBDA_NAME="${2}"
    local TMP_AWS_REGION="${3}"

    local TMP_STRING=""
    local AWS_REGION_STRING=""

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    TMP_STRING="$($(which aws) ${AWS_REGION_STRING} lambda get-function --function-name ${LAMBDA_NAME} 2>/dev/null | grep "FunctionArn" | sed -e 's/{//g' -e 's/}//g' | cut -d',' -f1 | awk -F ": " '{print $2}' | cut -d '"' -f2)"
    eval $__RESULTVAR="'${TMP_STRING}'"
}
