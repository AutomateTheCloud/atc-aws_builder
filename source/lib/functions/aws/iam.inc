###------------------------------------------------------------------------------------------------
# script:  iam.inc
# purpose: Collection of functions related to IAM
# version: 1.0.0
#
# function list:
#          - attach_iam_policy_to_role
#          - create_iam_policy
#          - create_iam_role
#          - delete_iam_policy
#          - delete_iam_role
#          - detach_iam_policy_from_role
#          - return_iam_policy_arn
#          - return_iam_policy_attached_to_role
#          - return_iam_role_arn
###------------------------------------------------------------------------------------------------
REQUIRED_EXECUTABLES+=('aws')

###------------------------------------------------------------------------------------------------
# Variables
AWS_IAM_DEFAULT_RETRY_COUNT=3
AWS_IAM_DEFAULT_RETRY_TIMER_MAX_SEC=30

###------------------------------------------------------------------------------------------------
## FUNCTION: attach_iam_policy_to_role()
## - Create Lambda Function
## - Arguments
##   - $1: Policy Name
##   - $2: Role Name
##   - $3: Region
function attach_iam_policy_to_role() {
    local FUNCTION_DESCRIPTION="IAM (Attach Policy to Role)"
    local IAM_POLICY_NAME="${1}"
    local IAM_ROLE_NAME="${2}"
    local TMP_AWS_REGION="${3}"

    local RETURNVAL=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    local AWS_ACCOUNT_NUMBER=""

    if(is_empty "${IAM_POLICY_NAME}"); then
        log_error "${FUNCTION_DESCRIPTION}: Policy Name not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${IAM_ROLE_NAME}"); then
        log_error "${FUNCTION_DESCRIPTION}: Role Name not specified"
        return $E_BAD_ARGS
    fi

    aws_metadata_account_id AWS_ACCOUNT_NUMBER
    if(is_empty "${AWS_ACCOUNT_NUMBER}"); then
        log_error "${FUNCTION_DESCRIPTION}: Failed to retrieve AWS Account Number"
        return $E_OBJECT_NOT_FOUND
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    log "${FUNCTION_DESCRIPTION}: started"
    log "- AWS Account Number: [${AWS_ACCOUNT_NUMBER}]"
    log "- Policy Name:        [${IAM_POLICY_NAME}]"
    log "- Role Name:          [${IAM_ROLE_NAME}]"
    log "- Region:             [${TMP_AWS_REGION}]"
    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_IAM_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_IAM_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_IAM_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        log "${FUNCTION_DESCRIPTION}: Attaching IAM Policy to Role (Attempt::${COUNTER} of ${AWS_IAM_DEFAULT_RETRY_COUNT})"
        $(which aws) ${AWS_REGION_STRING} iam attach-role-policy --role-name "${IAM_ROLE_NAME}" --policy-arn arn:aws:iam::${AWS_ACCOUNT_NUMBER}:policy/${IAM_POLICY_NAME} >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
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
            log_success "${FUNCTION_DESCRIPTION}: successfully attached IAM Policy to Role"
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: create_iam_policy()
## - Create IAM Policy
## - Arguments
##   - $1: Policy Name
##   - $2: Policy Document File
##   - $3: Region
function create_iam_policy() {
    local FUNCTION_DESCRIPTION="IAM (Create Policy)"
    local IAM_POLICY_NAME="${1}"
    local FILE_POLICY_DOCUMENT="${2}"
    local TMP_AWS_REGION="${3}"
    local RETURNVAL=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    if(is_empty "${IAM_POLICY_NAME}"); then
        log_error "${FUNCTION_DESCRIPTION}: Policy Name not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${FILE_POLICY_DOCUMENT}"); then
        log_error "${FUNCTION_DESCRIPTION}: Policy Document File not specified"
        return $E_BAD_ARGS
    fi
    if [ ! -f "${FILE_POLICY_DOCUMENT}" ]; then
        log_error "${FUNCTION_DESCRIPTION}: Policy Document File not found [${FILE_POLICY_DOCUMENT}]"
        return $E_OBJECT_NOT_FOUND
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    log "${FUNCTION_DESCRIPTION}: started"
    log "- Policy Name:        [${IAM_POLICY_NAME}]"
    log "- Policy Document:    [${FILE_POLICY_DOCUMENT}]"
    log "- Region:             [${TMP_AWS_REGION}]"
    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_IAM_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_IAM_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_IAM_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        log "${FUNCTION_DESCRIPTION}: Creating Policy (Attempt::${COUNTER} of ${AWS_IAM_DEFAULT_RETRY_COUNT})"
        $(which aws) ${AWS_REGION_STRING} iam create-policy --policy-name "${IAM_POLICY_NAME}" --policy-document file://${FILE_POLICY_DOCUMENT} >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
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
            log_success "${FUNCTION_DESCRIPTION}: successfully created Policy"
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: create_iam_role()
## - Create IAM Role
## - Arguments
##   - $1: Role Name
##   - $2: Trust Policy Document File
##   - $3: Region
function create_iam_role() {
    local FUNCTION_DESCRIPTION="IAM (Create Role)"
    local IAM_ROLE_NAME="${1}"
    local FILE_TRUST_POLICY_DOCUMENT="${2}"
    local TMP_AWS_REGION="${3}"

    local RETURNVAL=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    if(is_empty "${IAM_ROLE_NAME}"); then
        log_error "${FUNCTION_DESCRIPTION}: Role Name not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${FILE_TRUST_POLICY_DOCUMENT}"); then
        log_error "${FUNCTION_DESCRIPTION}: Trust Policy Document File not specified"
        return $E_BAD_ARGS
    fi
    if [ ! -f "${FILE_TRUST_POLICY_DOCUMENT}" ]; then
        log_error "${FUNCTION_DESCRIPTION}: Trust Policy Document File not found [${FILE_TRUST_POLICY_DOCUMENT}]"
        return $E_OBJECT_NOT_FOUND
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    log "${FUNCTION_DESCRIPTION}: started"
    log "- Role Name:             [${IAM_ROLE_NAME}]"
    log "- Trust Policy Document: [${FILE_TRUST_POLICY_DOCUMENT}]"
    log "- Region:                [${TMP_AWS_REGION}]"
    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_IAM_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_IAM_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_IAM_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        log "${FUNCTION_DESCRIPTION}: Creating Role (Attempt::${COUNTER} of ${AWS_IAM_DEFAULT_RETRY_COUNT})"
        $(which aws) ${AWS_REGION_STRING} iam create-role --role-name "${IAM_ROLE_NAME}" --assume-role-policy-document file://${FILE_TRUST_POLICY_DOCUMENT} >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
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
            log_success "${FUNCTION_DESCRIPTION}: successfully created Role"
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: delete_iam_policy()
## - Delete IAM Policy
## - Arguments
##   - $1: Policy ARN
##   - $2: Region
function delete_iam_policy() {
    local FUNCTION_DESCRIPTION="IAM (Delete Policy)"
    local IAM_POLICY_ARN="${1}"
    local TMP_AWS_REGION="${2}"

    local RETURNVAL=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    local AWS_ACCOUNT_NUMBER=""

    aws_metadata_account_id AWS_ACCOUNT_NUMBER
    if(is_empty "${AWS_ACCOUNT_NUMBER}"); then
        log_error "${FUNCTION_DESCRIPTION}: Failed to retrieve AWS Account Number"
        return $E_OBJECT_NOT_FOUND
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    log "${FUNCTION_DESCRIPTION}: started"
    log "- Policy ARN: [${IAM_POLICY_ARN}]"
    log "- Region:     [${TMP_AWS_REGION}]"
    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_IAM_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_IAM_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_IAM_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        log "${FUNCTION_DESCRIPTION}: Deleting Policy (Attempt::${COUNTER} of ${AWS_IAM_DEFAULT_RETRY_COUNT})"
        $(which aws) ${AWS_REGION_STRING} iam delete-policy --policy-arn "${IAM_POLICY_ARN}" >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
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
            log_success "${FUNCTION_DESCRIPTION}: successfully deleted Policy"
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: delete_iam_role()
## - Delete IAM Role
## - Arguments
##   - $1: Role Name
##   - $2: Region
function delete_iam_role() {
    local FUNCTION_DESCRIPTION="IAM (Delete Role)"
    local IAM_ROLE_NAME="${1}"
    local TMP_AWS_REGION="${2}"

    local RETURNVAL=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    if(is_empty "${IAM_ROLE_NAME}"); then
        log_error "${FUNCTION_DESCRIPTION}: Role Name not specified"
        return $E_BAD_ARGS
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    log "${FUNCTION_DESCRIPTION}: started"
    log "- Role Name: [${IAM_ROLE_NAME}]"
    log "- Region:    [${TMP_AWS_REGION}]"
    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_IAM_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_IAM_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_IAM_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        log "${FUNCTION_DESCRIPTION}: Deleting Role (Attempt::${COUNTER} of ${AWS_IAM_DEFAULT_RETRY_COUNT})"
        $(which aws) ${AWS_REGION_STRING} iam delete-role --role-name "${IAM_ROLE_NAME}" >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
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
            log_success "${FUNCTION_DESCRIPTION}: successfully deleted Role"
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: detach_iam_policy_from_role()
## - Detaches IAM Policy from Role
## - Arguments
##   - $1: Policy ARN
##   - $2: Role Name
##   - $3: Region
function detach_iam_policy_from_role() {
    local FUNCTION_DESCRIPTION="IAM (Detach Policy from Role)"
    local IAM_POLICY_ARN="${1}"
    local IAM_ROLE_NAME="${2}"
    local TMP_AWS_REGION="${3}"

    local RETURNVAL=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    local AWS_ACCOUNT_NUMBER=""

    if(is_empty "${IAM_POLICY_ARN}"); then
        log_error "${FUNCTION_DESCRIPTION}: Policy ARN not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${IAM_ROLE_NAME}"); then
        log_error "${FUNCTION_DESCRIPTION}: Role Name not specified"
        return $E_BAD_ARGS
    fi

    aws_metadata_account_id AWS_ACCOUNT_NUMBER
    if(is_empty "${AWS_ACCOUNT_NUMBER}"); then
        log_error "${FUNCTION_DESCRIPTION}: Failed to retrieve AWS Account Number"
        return $E_OBJECT_NOT_FOUND
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    log "${FUNCTION_DESCRIPTION}: started"
    log "- AWS Account Number: [${AWS_ACCOUNT_NUMBER}]"
    log "- Policy ARN:         [${IAM_POLICY_ARN}]"
    log "- Role Name:          [${IAM_ROLE_NAME}]"
    log "- Region:             [${TMP_AWS_REGION}]"
    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_IAM_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_IAM_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_IAM_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        log "${FUNCTION_DESCRIPTION}: Detaching IAM Policy from Role (Attempt::${COUNTER} of ${AWS_IAM_DEFAULT_RETRY_COUNT})"
        $(which aws) ${AWS_REGION_STRING} iam detach-role-policy --role-name "${IAM_ROLE_NAME}" --policy-arn ${IAM_POLICY_ARN} >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
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
            log_success "${FUNCTION_DESCRIPTION}: successfully detached IAM Policy from Role"
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: return_iam_policy_arn()
## - Returns IAM Policy ARN
## - Arguments
##   - $1: Variable name to pass info to
##   - $2: Policy Name
##   - $3: Region
function return_iam_policy_arn() {
    local __RESULTVAR=$1
    local IAM_POLICY_NAME="${2}"
    local TMP_AWS_REGION="${3}"

    local TMP_STRING=""
    local AWS_REGION_STRING=""

    local AWS_ACCOUNT_NUMBER=""

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    aws_metadata_account_id AWS_ACCOUNT_NUMBER
    TMP_STRING="$($(which aws) ${AWS_REGION_STRING} iam get-policy --policy-arn arn:aws:iam::${AWS_ACCOUNT_NUMBER}:policy/${IAM_POLICY_NAME} 2>/dev/null | grep "Arn" | sed -e 's/{//g' -e 's/}//g' | cut -d',' -f1 | awk -F ": " '{print $2}' | cut -d '"' -f2)"
    eval $__RESULTVAR="'${TMP_STRING}'"
}

###------------------------------------------------------------------------------------------------
## FUNCTION: return_iam_policy_attached_to_role()
## - Returns attached IAM Policy attached to Role
## - Arguments
##   - $1: Variable name to pass info to
##   - $2: Role Name
##   - $3: Region
function return_iam_policy_attached_to_role() {
    local __RESULTVAR=$1
    local IAM_ROLE_NAME="${2}"
    local TMP_AWS_REGION="${3}"

    local TMP_STRING=""
    local AWS_REGION_STRING=""

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    TMP_STRING="$($(which aws) ${AWS_REGION_STRING} iam list-attached-role-policies --role-name "${IAM_ROLE_NAME}" 2>/dev/null | grep "PolicyArn" | head -1 | sed -e 's/{//g' -e 's/}//g' | cut -d',' -f1 | awk -F ": " '{print $2}' | cut -d '"' -f2)"
    eval $__RESULTVAR="'${TMP_STRING}'"
}

###------------------------------------------------------------------------------------------------
## FUNCTION: return_iam_role_arn()
## - Returns IAM Role ARN
## - Arguments
##   - $1: Variable name to pass info to
##   - $2: Role Name
##   - $3: Region
function return_iam_role_arn() {
    local __RESULTVAR=$1
    local IAM_ROLE_NAME="${2}"
    local TMP_AWS_REGION="${3}"

    local TMP_STRING=""
    local AWS_REGION_STRING=""

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    TMP_STRING="$($(which aws) ${AWS_REGION_STRING} iam get-role --role-name "${IAM_ROLE_NAME}" 2>/dev/null | grep "Arn" | head -1 | sed -e 's/{//g' -e 's/}//g' | cut -d',' -f1 | awk -F ": " '{print $2}' | cut -d '"' -f2)"
    eval $__RESULTVAR="'${TMP_STRING}'"
}
