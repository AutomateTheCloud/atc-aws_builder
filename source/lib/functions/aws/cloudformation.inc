###------------------------------------------------------------------------------------------------
# script:  cloudformation.inc
# purpose: Collection of functions related to AWS (CloudFormation)
# version: 1.0.0
#
# function list:
#          - cloudformation_deploy
#          - cloudformation_get_outputs
#          - cloudformation_get_outputs_silent
#          - cloudformation_get_stack_attribute
#          - cloudformation_poll_status
#          - cloudformation_stack_exists
#          - cloudformation_validate_template
#          - display_variables_details
#          - load_variables_details_from_file_keyvalue
#          - load_variables_details_from_file
#          - sam_package
###------------------------------------------------------------------------------------------------
REQUIRED_EXECUTABLES+=('aws')

###------------------------------------------------------------------------------------------------
# Variables
AWS_CLOUDFORMATION_DEFAULT_RETRY_COUNT=3
AWS_CLOUDFORMATION_DEFAULT_RETRY_TIMER_MAX_SEC=30
AWS_CLOUDFORMATION_DEFAULT_VERIFICATION_TIMEOUT=45
AWS_CLOUDFORMATION_DEFAULT_VERIFICATION_SLEEP=15

EXTENSION_PROJECT="project"
EXTENSION_SECRETS="secrets"
EXTENSION_DETAILS="details"
EXTENSION_TEMPLATE="template.yaml"
EXTENSION_PARAMETERS="parameters.json"
EXTENSION_TAGS="tags.json"
EXTENSION_TARGETS="targets"

REF_DETAILS_VARIABLES_REQUIRED+=(
    'stack_name'
    'stack_description'
    'deployment_timeout'
    'account_name'
    'account_abbr'
    'account_number'
    'organization_name'
    'organization_abbr'
    'region'
    'environment'
    'project_name'
    'project_abbr'
    'function_name'
    'function_abbr'
    'owner'
    'contact'
)

DETAILS_STACK_NAME=""
DETAILS_STACK_DESCRIPTION=""
DETAILS_DEPLOYMENT_TIMEOUT=""
DETAILS_ACCOUNT_NAME=""
DETAILS_ACCOUNT_ABBR=""
DETAILS_ACCOUNT_NUMBER=""
DETAILS_ORGANIZATION_NAME=""
DETAILS_ORGANIZATION_ABBR=""
DETAILS_REGION=""
DETAILS_ENVIRONMENT=""
DETAILS_PROJECT_NAME=""
DETAILS_PROJECT_ABBR=""
DETAILS_FUNCTION_NAME=""
DETAILS_FUNCTION_ABBR=""
DETAILS_OWNER=""
DETAILS_CONTACT=""

DEFAULT_DEPLOYMENT_TIMEOUT=45

###------------------------------------------------------------------------------------------------
## FUNCTION: cloudformation_deploy()
## - Deploys CloudFormation Template to AWS
## - Arguments
##   - $1: Variable name to pass StackId to
##   - $2: Stack Name
##   - $3: Template file
##   - $4: Parameters file
##   - $5: Tags file
##   - $6: S3 Bucket (Temp)
##   - $7: Region
function cloudformation_deploy() {
    local FUNCTION_DESCRIPTION="CloudFormation (Deploy)"
    local __RESULTVAR=$1
    local TMP_STACK_NAME="${2}"
    local TMP_FILE_TEMPLATE="${3}"
    local TMP_FILE_PARAMETERS="${4}"
    local TMP_FILE_TAGS="${5}"
    local TMP_S3_BUCKET_TEMP="${6}"
    local TMP_AWS_REGION="${7}"

    local RETURNVAL=""
    local RETURN_CODE=0
    local TMP_STRING=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local CF_DEPLOYMENT_METHOD=""
    local CF_TEMPLATE_URL=""
    local AWS_TEMP_FILE=""
    local TMP_BUCKET_REGION=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    if(is_empty "${__RESULTVAR}"); then
        log_error "${FUNCTION_DESCRIPTION}: Result Variable not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_STACK_NAME}"); then
        log_error "${FUNCTION_DESCRIPTION}: Stack Name not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_FILE_TEMPLATE}"); then
        log_error "${FUNCTION_DESCRIPTION}: Template file not specified"
        return $E_BAD_ARGS
    fi
    if [ ! -f "${TMP_FILE_TEMPLATE}" ]; then
        log_error "- ${FUNCTION_DESCRIPTION}: Template file does not exist [${TMP_FILE_TEMPLATE}]"
        return $E_OBJECT_NOT_FOUND
    fi
    if(is_empty "${TMP_FILE_PARAMETERS}"); then
        log_error "${FUNCTION_DESCRIPTION}: Parameters file not specified"
        return $E_BAD_ARGS
    fi
    if [ ! -f "${TMP_FILE_PARAMETERS}" ]; then
        log_error "- ${FUNCTION_DESCRIPTION}: Parameters file does not exist [${TMP_FILE_PARAMETERS}]"
        return $E_OBJECT_NOT_FOUND
    fi
    if(is_empty "${TMP_FILE_TAGS}"); then
        log_error "${FUNCTION_DESCRIPTION}: Tags file not specified"
        return $E_BAD_ARGS
    fi
    if [ ! -f "${TMP_FILE_TAGS}" ]; then
        log_error "- ${FUNCTION_DESCRIPTION}: Tags file does not exist [${TMP_FILE_TAGS}]"
        return $E_OBJECT_NOT_FOUND
    fi
    if(is_empty "${TMP_S3_BUCKET_TEMP}"); then
        log_error "${FUNCTION_DESCRIPTION}: S3 Bucket (Temp) not specified"
        return $E_BAD_ARGS
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    log_notice "${FUNCTION_DESCRIPTION}: started"
    line_break
    log "- Stack Name:        [${TMP_STACK_NAME}]"
    log "- File (Template):   [${TMP_FILE_TEMPLATE}]"
    log "- File (Parameters): [${TMP_FILE_PARAMETERS}]"
    log "- File (Tags):       [${TMP_FILE_TAGS}]"
    log "- S3 Bucket (Temp):  [s3://${TMP_S3_BUCKET_TEMP}]"
    log "- Region:            [${TMP_AWS_REGION}]"
    line_break

    log_notice "${FUNCTION_DESCRIPTION}: determining deployment method"
    if(cloudformation_stack_exists "${TMP_STACK_NAME}" "${TMP_AWS_REGION}"); then
        log "- Stack currently exists, [update-stack] method selected"
        CF_DEPLOYMENT_METHOD="update-stack"
    else
        log "- Stack does not currently exist, [create-stack] method selected"
        CF_DEPLOYMENT_METHOD="create-stack"
    fi

    log_notice "${FUNCTION_DESCRIPTION}: uploading CloudFormation Template to S3"
    AWS_TEMP_FILE="cfn/$(generate_uuid).tmp"
    s3_cp_upload "${TMP_S3_BUCKET_TEMP}" "${AWS_TEMP_FILE}" "${TMP_FILE_TEMPLATE}" yes yes "${TMP_AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "${FUNCTION_DESCRIPTION}: Failed to upload CloudFormation Template to S3, aborting operation"
        return ${RETURNVAL}
    fi

    # The following is to address the fact that Template URL's require https:// addresses, and they handle us-east-1 as simple s3. rather than s3-us-east-1
    TMP_BUCKET_REGION="$($(which aws) ${AWS_REGION_STRING} s3api get-bucket-location --bucket ${TMP_S3_BUCKET_TEMP} --query "LocationConstraint" --output text 2>/dev/null)"
    if [[ "ZZ_$(to_upper "${TMP_BUCKET_REGION}")" == "ZZ_NONE" ]]; then
        CF_TEMPLATE_URL="https://s3.amazonaws.com/${TMP_S3_BUCKET_TEMP}/${AWS_TEMP_FILE}"
    else
        CF_TEMPLATE_URL="https://s3-${TMP_BUCKET_REGION}.amazonaws.com/${TMP_S3_BUCKET_TEMP}/${AWS_TEMP_FILE}"
    fi

    log_notice "${FUNCTION_DESCRIPTION}: deploying stack"
    $(which aws) ${AWS_REGION_STRING} cloudformation ${CF_DEPLOYMENT_METHOD} --stack-name ${TMP_STACK_NAME} --template-url ${CF_TEMPLATE_URL} --parameters file://${TMP_FILE_PARAMETERS} --capabilities CAPABILITY_NAMED_IAM CAPABILITY_AUTO_EXPAND --tags file://${TMP_FILE_TAGS} >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
    RETURNVAL="$?"
    $(which sed) -i "s/\o15/_AWS_BAD_IGNORE\\n/g" "${AWS_FILE_RESPONSE}"
    $(which sed) -i '/_AWS_BAD_IGNORE/d' "${AWS_FILE_RESPONSE}"
    if [ ${RETURNVAL} -ne 0 ]; then
        if grep -q "No updates are to be performed" "${AWS_FILE_ERROR}"; then
            log_warning "${FUNCTION_DESCRIPTION}: No updates are to be performed, skipping deployment"
            line_break
        else
            log_add_from_file "${AWS_FILE_ERROR}" "${FUNCTION_DESCRIPTION}: Data containing error" 200000
            log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Data" 200000
            log_error "${FUNCTION_DESCRIPTION}: CloudFormation Deployment [${CF_DEPLOYMENT_METHOD}] failed"
            RETURN_CODE=$E_AWS_FAILURE
        fi
    else
        TMP_STRING="$(cat ${AWS_FILE_RESPONSE} | grep '"StackId":' | sed -e 's/{//g' -e 's/}//g' | cut -d',' -f1 | awk -F ": " '{print $2}' | cut -d '"' -f2)"
        log "- successfully initiated CloudFormation deployment [StackId::${TMP_STRING}]"
        line_break
        eval $__RESULTVAR="'${TMP_STRING}'"
    fi

    log "${FUNCTION_DESCRIPTION}: removing template from S3 for cleanup"
    s3_delete "${TMP_S3_BUCKET_TEMP}" "${AWS_TEMP_FILE}" "${TMP_AWS_REGION}"

    return $RETURN_CODE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: cloudformation_get_outputs()
## - Retrieves outputs from CloudFormation Stack and dumps them into a file in KEY=VALUE format
## - Arguments
##   - $1: File to dump data to
##   - $2: CloudFormation Stack Name
##   - $3: Region
function cloudformation_get_outputs() {
    local FUNCTION_DESCRIPTION="CloudFormation (Get Outputs)"
    local FILE_CLOUDFORMATION_OUTPUTS="${1}"
    local CLOUDFORMATION_STACK_NAME="${2}"
    local TMP_AWS_REGION="${3}"

    local RETURNVAL=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    if(is_empty "${FILE_CLOUDFORMATION_OUTPUTS}"); then
        log_error "${FUNCTION_DESCRIPTION}: CloudFormation Outputs file not specified"
        return $E_BAD_ARGS
    fi
    if [ ! -f "${FILE_CLOUDFORMATION_OUTPUTS}" ]; then
        log_error "${FUNCTION_DESCRIPTION}: CloudFormation Outputs file does not exist [${FILE_CLOUDFORMATION_OUTPUTS}]"
        return $E_OBJECT_NOT_FOUND
    fi
    if(is_empty "${CLOUDFORMATION_STACK_NAME}"); then
        log_error "${FUNCTION_DESCRIPTION}: CloudFormation Stack Name not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_AWS_REGION}"); then
        log_error "${FUNCTION_DESCRIPTION}: AWS Region not specified"
        return $E_BAD_ARGS
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    log "${FUNCTION_DESCRIPTION}: started"
    log "- CloudFormation Stack: [${CLOUDFORMATION_STACK_NAME}]"
    log "- Outputs File:         [${FILE_CLOUDFORMATION_OUTPUTS}]"
    log "- Region:               [${TMP_AWS_REGION}]"
    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_CLOUDFORMATION_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_CLOUDFORMATION_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_CLOUDFORMATION_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        log "${FUNCTION_DESCRIPTION}: Retrieving outputs (cloudformation_stack: [${CLOUDFORMATION_STACK_NAME}]) (Attempt::${COUNTER} of ${AWS_CLOUDFORMATION_DEFAULT_RETRY_COUNT})"
        $(which aws) ${AWS_REGION_STRING} cloudformation describe-stacks --stack-name "${CLOUDFORMATION_STACK_NAME}" --query 'Stacks[*].Outputs[*].{Key:OutputKey,Value:OutputValue}' --output text >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
        RETURNVAL="$?"
        # Fix Response File
        $(which sed) -i "s/\o15/_AWS_BAD_IGNORE\\n/g" "${AWS_FILE_RESPONSE}"
        $(which sed) -i '/_AWS_BAD_IGNORE/d' "${AWS_FILE_RESPONSE}"
        if [ ${RETURNVAL} -ne 0 ]; then
            log_add_from_file "${AWS_FILE_ERROR}" "${FUNCTION_DESCRIPTION}: Data containing error"
            log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Data"
            log_error "${FUNCTION_DESCRIPTION}: operation failed (AWS_CLOUDFORMATION_exit_code::${RETURNVAL}])"
            RETRY_ENABLED=yes
            # Reset Files
            > ${AWS_FILE_RESPONSE}
            > ${AWS_FILE_ERROR}
        else
            cat ${AWS_FILE_RESPONSE} | awk -F'\t' '{print $1"="$2}' | sort > ${FILE_CLOUDFORMATION_OUTPUTS}
            log_add_from_file "${FILE_CLOUDFORMATION_OUTPUTS}" "${FUNCTION_DESCRIPTION}: Outputs"
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: cloudformation_get_outputs_silent()
## - Retrieves outputs from CloudFormation Stack and dumps them into a file in KEY=VALUE format
## - Suppresses Log output, unless there is an error
## - Arguments
##   - $1: File to dump data to
##   - $2: CloudFormation Stack Name
##   - $3: Region
function cloudformation_get_outputs_silent() {
    local FUNCTION_DESCRIPTION="CloudFormation (Get Outputs)"
    local FILE_CLOUDFORMATION_OUTPUTS="${1}"
    local CLOUDFORMATION_STACK_NAME="${2}"
    local TMP_AWS_REGION="${3}"

    local RETURNVAL=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    if(is_empty "${FILE_CLOUDFORMATION_OUTPUTS}"); then
        log_error "${FUNCTION_DESCRIPTION}: CloudFormation Outputs file not specified"
        return $E_BAD_ARGS
    fi
    if [ ! -f "${FILE_CLOUDFORMATION_OUTPUTS}" ]; then
        log_error "${FUNCTION_DESCRIPTION}: CloudFormation Outputs file does not exist [${FILE_CLOUDFORMATION_OUTPUTS}]"
        return $E_OBJECT_NOT_FOUND
    fi
    if(is_empty "${CLOUDFORMATION_STACK_NAME}"); then
        log_error "${FUNCTION_DESCRIPTION}: CloudFormation Stack Name not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_AWS_REGION}"); then
        log_error "${FUNCTION_DESCRIPTION}: AWS Region not specified"
        return $E_BAD_ARGS
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_CLOUDFORMATION_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_CLOUDFORMATION_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_CLOUDFORMATION_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        $(which aws) ${AWS_REGION_STRING} cloudformation describe-stacks --stack-name "${CLOUDFORMATION_STACK_NAME}" --query 'Stacks[*].Outputs[*].{Key:OutputKey,Value:OutputValue}' --output text >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
        RETURNVAL="$?"
        # Fix Response File
        $(which sed) -i "s/\o15/_AWS_BAD_IGNORE\\n/g" "${AWS_FILE_RESPONSE}"
        $(which sed) -i '/_AWS_BAD_IGNORE/d' "${AWS_FILE_RESPONSE}"
        if [ ${RETURNVAL} -ne 0 ]; then
            log_add_from_file "${AWS_FILE_ERROR}" "${FUNCTION_DESCRIPTION}: Data containing error"
            log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Data"
            log_error "${FUNCTION_DESCRIPTION}: operation failed (AWS_CLOUDFORMATION_exit_code::${RETURNVAL}])"
            RETRY_ENABLED=yes
            # Reset Files
            > ${AWS_FILE_RESPONSE}
            > ${AWS_FILE_ERROR}
        else
            cat ${AWS_FILE_RESPONSE} | awk -F'\t' '{print $1"="$2}' | sort > ${FILE_CLOUDFORMATION_OUTPUTS}
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: cloudformation_get_stack_attribute()
## - Retrieves Stack Attribute for specified CloudFormation stack
## - Arguments
##   - $1: Variable name to pass info to
##   - $2: Stack Name
##   - $3: Attribute
##   - $4: Fail with error on STACK_DOES_NOT_EXIST (yes/no)
##   - $5: Capitalize Attribute Value (yes/no)
##   - $6: Region
function cloudformation_get_stack_attribute() {
    local FUNCTION_DESCRIPTION="CloudFormation (Get Stack Attribute)"
    local __RESULTVAR=$1
    local TMP_STACK_NAME="${2}"
    local TMP_ATTRIBUTE="${3}"
    local ERROR_ON_NOT_FOUND="${4}"
    local CAPITALIZE_ATTRIBUTE_VALUE="${5}"
    local TMP_AWS_REGION="${6}"

    local RETURNVAL=""
    local TMP_STRING=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    if(is_empty "${TMP_STACK_NAME}"); then
        log_error "${FUNCTION_DESCRIPTION}: Stack Name not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_ATTRIBUTE}"); then
        log_error "${FUNCTION_DESCRIPTION}: Attribute not specified"
        return $E_BAD_ARGS
    fi

    if(is_empty "${ERROR_ON_NOT_FOUND}"); then
        ERROR_ON_NOT_FOUND=yes
    fi
    if(is_empty "${CAPITALIZE_ATTRIBUTE_VALUE}"); then
        CAPITALIZE_ATTRIBUTE_VALUE=no
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_CLOUDFORMATION_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_CLOUDFORMATION_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_CLOUDFORMATION_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no
        $(which aws) ${AWS_REGION_STRING} cloudformation describe-stacks --stack-name "${TMP_STACK_NAME}" >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
        RETURNVAL="$?"
        # Fix Response File
        $(which sed) -i "s/\o15/_AWS_BAD_IGNORE\\n/g" "${AWS_FILE_RESPONSE}"
        $(which sed) -i '/_AWS_BAD_IGNORE/d' "${AWS_FILE_RESPONSE}"
        if [ ${RETURNVAL} -ne 0 ]; then
            if grep -q "does not exist" "${AWS_FILE_ERROR}"; then
                if option_enabled ERROR_ON_NOT_FOUND; then
                    TMP_STRING="NOT_FOUND"
                    eval $__RESULTVAR="'$(echo "$(trim "${TMP_STRING}")" | sed -e 's/^ *//g' -e 's/ *$//g')'"
                    log_error "${FUNCTION_DESCRIPTION}: Stack not found [${TMP_STACK_NAME}]"
                    return $E_OBJECT_NOT_FOUND
                else
                    TMP_STRING="NOT_FOUND"
                    eval $__RESULTVAR="'$(echo "$(trim "${TMP_STRING}")" | sed -e 's/^ *//g' -e 's/ *$//g')'"
                    return 0
                fi
            fi
            log_add_from_file "${AWS_FILE_ERROR}" "${FUNCTION_DESCRIPTION}: Data containing error"
            log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Data"
            log_error "${FUNCTION_DESCRIPTION}: operation failed (aws_cli_exit_code::${RETURNVAL}])"
            RETRY_ENABLED=yes
            # Reset Files
            > ${AWS_FILE_RESPONSE}
            > ${AWS_FILE_ERROR}
        else
            if option_enabled CAPITALIZE_ATTRIBUTE_VALUE; then
                TMP_STRING="$(cat ${AWS_FILE_RESPONSE} | grep "\"${TMP_ATTRIBUTE}\":" | sed -e 's/{//g' -e 's/}//g' | cut -d',' -f1 | awk -F ": " '{print $2}' | cut -d '"' -f2 | tr '[:lower:]' '[:upper:]')"
            else
                TMP_STRING="$(cat ${AWS_FILE_RESPONSE} | grep "\"${TMP_ATTRIBUTE}\":" | sed -e 's/{//g' -e 's/}//g' | cut -d',' -f1 | awk -F ": " '{print $2}' | cut -d '"' -f2)"
            fi
            eval $__RESULTVAR="'$(echo "$(trim "${TMP_STRING}")" | sed -e 's/^ *//g' -e 's/ *$//g')'"
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: cloudformation_poll_status()
## - Polls CloudFormation Stack and waits for a stable state
## - Arguments
##   - $1: Stack Name
##   - $2: Fail with error on STACK_DOES_NOT_EXIST (yes/no)
##   - $3: Verification Timeout <in minutes, defaults to 45 minutes>
##   - $4: Verification Time between polling <in seconds, defaults to 30 seconds)
##   - $5: Region
function cloudformation_poll_status() {
    local FUNCTION_DESCRIPTION="CloudFormation (Poll Status)"
    local TMP_STACK_NAME="${1}"
    local ERROR_ON_NOT_FOUND="${2}"
    local TMP_AWS_CLOUDFORMATION_VERIFICATION_TIMEOUT="${3}"
    local TMP_AWS_CLOUDFORMATION_VERIFICATION_SLEEP="${4}"
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

    local AWS_CLOUDFORMATION_VERIFICATION_MAX_ATTEMPTS=""
    local POLLING_PROGRESS=1
    local POLLING_STATUS=""
    local TMP_STACK_STATUS=""
    local STATUS_DESCRIPTION=""
    local TMP_LAST_STATUS=""

    if(is_empty "${TMP_STACK_NAME}"); then
        log_error "${FUNCTION_DESCRIPTION}: Stack Name ID not specified"
        return $E_BAD_ARGS
    fi

    if(is_empty "${ERROR_ON_NOT_FOUND}"); then
        ERROR_ON_NOT_FOUND=yes
    fi

    if(is_empty "${TMP_AWS_CLOUDFORMATION_VERIFICATION_TIMEOUT}"); then
        TMP_AWS_CLOUDFORMATION_VERIFICATION_TIMEOUT=${AWS_CLOUDFORMATION_DEFAULT_VERIFICATION_TIMEOUT}
    fi

    if(is_empty "${TMP_AWS_CLOUDFORMATION_VERIFICATION_SLEEP}"); then
        TMP_AWS_CLOUDFORMATION_VERIFICATION_SLEEP=${AWS_CLOUDFORMATION_DEFAULT_VERIFICATION_SLEEP}
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    AWS_CLOUDFORMATION_VERIFICATION_MAX_ATTEMPTS=$((TMP_AWS_CLOUDFORMATION_VERIFICATION_TIMEOUT * 60 / TMP_AWS_CLOUDFORMATION_VERIFICATION_SLEEP))

    log "${FUNCTION_DESCRIPTION}: started (stack_name: [${TMP_STACK_NAME}] / region: [${TMP_AWS_REGION}] / timeout: [${TMP_AWS_CLOUDFORMATION_VERIFICATION_TIMEOUT}] / attempts: [${AWS_CLOUDFORMATION_VERIFICATION_MAX_ATTEMPTS}])"
    while [ ${POLLING_PROGRESS} == 1 ]; do
        if [[ "ZZ_${TMP_STACK_STATUS}" != "ZZ_${TMP_LAST_STATUS}" ]]; then
            if(! is_empty "${TMP_LAST_STATUS}"); then
                log "-- Status Change Detected, resetting counter [${TMP_LAST_STATUS} => ${TMP_STACK_STATUS}]"
                COUNTER=0
            fi
            TMP_LAST_STATUS="${TMP_STACK_STATUS}"
        fi
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_CLOUDFORMATION_VERIFICATION_MAX_ATTEMPTS} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_CLOUDFORMATION_VERIFICATION_MAX_ATTEMPTS}] exceeded, aborting tracking operation"
            return $E_CLOUDFORMATION_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep ${TMP_AWS_CLOUDFORMATION_VERIFICATION_SLEEP}
        fi
        log "- Polling Status [${TMP_STACK_NAME}]  (Attempt::${COUNTER} of ${AWS_CLOUDFORMATION_VERIFICATION_MAX_ATTEMPTS})"
        cloudformation_get_stack_attribute TMP_STACK_STATUS "${TMP_STACK_NAME}" "StackStatus" ${ERROR_ON_NOT_FOUND} yes "${TMP_AWS_REGION}"
        RETURNVAL="$?"
        if [ ${RETURNVAL} -ne 0 ]; then
            return $RETURNVAL
        fi

        case $TMP_STACK_STATUS in
            "CREATE_COMPLETE")
                STATUS_DESCRIPTION="Successful creation of one or more stacks."
                POLLING_PROGRESS=0
                POLLING_STATUS=0
                ;;
            "CREATE_IN_PROGRESS")
                STATUS_DESCRIPTION="Ongoing creation of one or more stacks."
                POLLING_PROGRESS=1
                ;;
            "CREATE_FAILED")
                STATUS_DESCRIPTION="Unsuccessful creation of one or more stacks. View the stack events to see any associated error messages. Possible reasons for a failed creation include insufficient permissions to work with all resources in the stack, parameter values rejected by an AWS service, or a timeout during resource creation."
                POLLING_PROGRESS=0
                POLLING_STATUS=$E_CLOUDFORMATION_FAILURE
                ;;
            "DELETE_COMPLETE")
                STATUS_DESCRIPTION="Successful deletion of one or more stacks. Deleted stacks are retained and viewable for 90 days."
                POLLING_PROGRESS=0
                POLLING_STATUS=0
                ;;
            "DELETE_FAILED")
                STATUS_DESCRIPTION="Unsuccessful deletion of one or more stacks. Because the delete failed, you might have some resources that are still running; however, you cannot work with or update the stack. Delete the stack again or view the stack events to see any associated error messages."
                POLLING_PROGRESS=0
                POLLING_STATUS=$E_CLOUDFORMATION_FAILURE
                ;;
            "DELETE_IN_PROGRESS")
                STATUS_DESCRIPTION="Ongoing removal of one or more stacks."
                POLLING_PROGRESS=1
                ;;
            "REVIEW_IN_PROGRESS")
                STATUS_DESCRIPTION="Ongoing creation of one or more stacks with an expected StackId but without any templates or resources."
                POLLING_PROGRESS=1
                ;;
            "ROLLBACK_COMPLETE")
                STATUS_DESCRIPTION="Successful removal of one or more stacks after a failed stack creation or after an explicitly canceled stack creation. Any resources that were created during the create stack action are deleted. This status exists only after a failed stack creation. It signifies that all operations from the partially created stack have been appropriately cleaned up."
                POLLING_PROGRESS=0
                POLLING_STATUS=$E_CLOUDFORMATION_FAILURE
                ;;
            "ROLLBACK_FAILED")
                STATUS_DESCRIPTION="Unsuccessful removal of one or more stacks after a failed stack creation or after an explicitly canceled stack creation. Delete the stack or view the stack events to see any associated error messages."
                POLLING_PROGRESS=0
                POLLING_STATUS=$E_CLOUDFORMATION_FAILURE
                ;;
            "ROLLBACK_IN_PROGRESS")
                STATUS_DESCRIPTION="Ongoing removal of one or more stacks after a failed stack creation or after an explicitly cancelled stack creation."
                POLLING_PROGRESS=1
                ;;
            "UPDATE_COMPLETE")
                STATUS_DESCRIPTION="Successful update of one or more stacks."
                POLLING_PROGRESS=0
                POLLING_STATUS=0
                ;;
            "UPDATE_COMPLETE_CLEANUP_IN_PROGRESS")
                STATUS_DESCRIPTION="Ongoing removal of old resources for one or more stacks after a successful stack update. For stack updates that require resources to be replaced, AWS CloudFormation creates the new resources first and then deletes the old resources to help reduce any interruptions with your stack. In this state, the stack has been updated and is usable, but AWS CloudFormation is still deleting the old resources."
                POLLING_PROGRESS=1
                ;;
            "UPDATE_IN_PROGRESS")
                STATUS_DESCRIPTION="Ongoing update of one or more stacks."
                POLLING_PROGRESS=1
                ;;
            "UPDATE_ROLLBACK_COMPLETE")
                STATUS_DESCRIPTION="Successful return of one or more stacks to a previous working state after a failed stack update."
                POLLING_PROGRESS=0
                POLLING_STATUS=$E_CLOUDFORMATION_FAILURE
                ;;
            "UPDATE_ROLLBACK_COMPLETE_CLEANUP_IN_PROGRESS")
                STATUS_DESCRIPTION="Ongoing removal of new resources for one or more stacks after a failed stack update. In this state, the stack has been rolled back to its previous working state and is usable, but AWS CloudFormation is still deleting any new resources it created during the stack update."
                POLLING_PROGRESS=1
                ;;
            "UPDATE_ROLLBACK_FAILED")
                STATUS_DESCRIPTION="Unsuccessful return of one or more stacks to a previous working state after a failed stack update. When in this state, you can delete the stack or continue rollback. You might need to fix errors before your stack can return to a working state. Or, you can contact customer support to restore the stack to a usable state."
                POLLING_PROGRESS=0
                POLLING_STATUS=$E_CLOUDFORMATION_FAILURE
                ;;
            "UPDATE_ROLLBACK_IN_PROGRESS")
                STATUS_DESCRIPTION="Ongoing return of one or more stacks to the previous working state after failed stack update."
                POLLING_PROGRESS=1
                ;;
            "NOT_FOUND")
                STATUS_DESCRIPTION="Stack not found."
                POLLING_PROGRESS=0
                POLLING_STATUS=0
                ;;
            *)
                STATUS_DESCRIPTION="n/a"
                POLLING_PROGRESS=1
                ;;
            ?)
                STATUS_DESCRIPTION="n/a"
                POLLING_PROGRESS=1
                ;;
        esac
        RETRY_ENABLED=yes
        log "- Status: [${TMP_STACK_STATUS}]"
        log "- Description: [${STATUS_DESCRIPTION}]"
    done

    return $POLLING_STATUS
}

###------------------------------------------------------------------------------------------------
## FUNCTION: cloudformation_stack_exists()
## - Checks to see if CloudFormation Stack exists
## - Arguments
##   - $1: Stack Name
##   - $2: Region
function cloudformation_stack_exists() {
    local FUNCTION_DESCRIPTION="CloudFormation (Does Stack Exist)"
    local TMP_STACK_NAME="${1}"
    local TMP_AWS_REGION="${2}"

    local RETURNVAL=""
    local AWS_REGION_STRING=""

    if(is_empty "${TMP_STACK_NAME}"); then
        log_error "${FUNCTION_DESCRIPTION}: CloudFormation Stack Name not specified"
        return $E_BAD_ARGS
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    $(which aws) ${AWS_REGION_STRING} cloudformation describe-stacks --stack-name "${TMP_STACK_NAME}" --query 'Stacks[*].{StackId:StackId}' --output text >/dev/null 2>&1
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        return $E_AWS_RESOURCE_NOT_FOUND
    fi
    return 0
}

###------------------------------------------------------------------------------------------------
## FUNCTION: cloudformation_validate_template()
## - Validates specified CloudFormation Template using the AWS API call 'validate-template'
## - Arguments
##   - $1: CloudFormation Template file
##   - $2: S3 Bucket (temp)
##   - $3: Region
function cloudformation_validate_template() {
    local FUNCTION_DESCRIPTION="CloudFormation (Validate Template)"
    local TMP_FILE_CLOUDFORMATION_TEMPLATE="${1}"
    local TMP_S3_BUCKET_TEMP="${2}"
    local TMP_AWS_REGION="${3}"

    local RETURNVAL=""
    local RETURN_CODE=0
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local AWS_TEMP_FILE=""
    local CF_TEMPLATE_URL=""
    local TMP_BUCKET_REGION=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    if(is_empty "${TMP_FILE_CLOUDFORMATION_TEMPLATE}"); then
        log_error "${FUNCTION_DESCRIPTION}: CloudFormation Template file file not specified"
        return $E_BAD_ARGS
    fi
    if [ ! -f "${TMP_FILE_CLOUDFORMATION_TEMPLATE}" ]; then
        log_error "${FUNCTION_DESCRIPTION}: CloudFormation Template file does not exist [${TMP_FILE_CLOUDFORMATION_TEMPLATE}]"
        return $E_OBJECT_NOT_FOUND
    fi
    if(is_empty "${TMP_S3_BUCKET_TEMP}"); then
        log_error "${FUNCTION_DESCRIPTION}: S3 Bucket (temp) not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_AWS_REGION}"); then
        log_error "${FUNCTION_DESCRIPTION}: AWS Region not specified"
        return $E_BAD_ARGS
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    AWS_TEMP_FILE="cfn/$(generate_uuid).tmp"

    # The following is to address the fact that Template URL's require https:// addresses, and they handle us-east-1 as simple s3. rather than s3-us-east-1
    TMP_BUCKET_REGION="$($(which aws) ${AWS_REGION_STRING} s3api get-bucket-location --bucket ${TMP_S3_BUCKET_TEMP} --query "LocationConstraint" --output text 2>/dev/null)"
    if [[ "ZZ_$(to_upper "${TMP_BUCKET_REGION}")" == "ZZ_NONE" ]]; then
        CF_TEMPLATE_URL="https://s3.amazonaws.com/${TMP_S3_BUCKET_TEMP}/${AWS_TEMP_FILE}"
    else
        CF_TEMPLATE_URL="https://s3-${TMP_BUCKET_REGION}.amazonaws.com/${TMP_S3_BUCKET_TEMP}/${AWS_TEMP_FILE}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    log "${FUNCTION_DESCRIPTION}: started"
    log "- CloudFormation Template: [$(basename "${TMP_FILE_CLOUDFORMATION_TEMPLATE}")]"
    log "- S3 Bucket:               [${TMP_S3_BUCKET_TEMP}]"
    log "- Template URL:            [${CF_TEMPLATE_URL}]"
    log "- Region:                  [${TMP_AWS_REGION}]"

    log "${FUNCTION_DESCRIPTION}: uploading template to S3 for analysis"
    s3_cp_upload "${TMP_S3_BUCKET_TEMP}" "${AWS_TEMP_FILE}" "${TMP_FILE_CLOUDFORMATION_TEMPLATE}" yes yes "${TMP_AWS_REGION}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "- Failed to upload Template to S3, aborting operation"
        return $RETURNVAL
    fi

    $(which aws) ${AWS_REGION_STRING} cloudformation validate-template --template-url ${CF_TEMPLATE_URL} >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
    RETURNVAL="$?"
    # Fix Response File
    $(which sed) -i "s/\o15/_AWS_BAD_IGNORE\\n/g" "${AWS_FILE_RESPONSE}"
    $(which sed) -i '/_AWS_BAD_IGNORE/d' "${AWS_FILE_RESPONSE}"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_add_from_file "${TMP_FILE_CLOUDFORMATION_TEMPLATE}" "${FUNCTION_DESCRIPTION}: Template Output" 200000
        log_add_from_file "${AWS_FILE_ERROR}" "${FUNCTION_DESCRIPTION}: Data containing error" 200000
        log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Data" 200000
        log_error "${FUNCTION_DESCRIPTION}: Template failed validation (aws_cli_exit_code::${RETURNVAL}])"
        RETURN_CODE=$E_AWS_FAILURE
    else
        log "- [validated]"
    fi

    log "${FUNCTION_DESCRIPTION}: removing template from S3 for cleanup"
    s3_delete "${TMP_S3_BUCKET_TEMP}" "${AWS_TEMP_FILE}" "${TMP_AWS_REGION}"

    log "${FUNCTION_DESCRIPTION}: finished"
    return $RETURN_CODE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: display_variables_details()
## - Displays Variables: Details
function display_variables_details() {
    line_break
    log_highlight "Variables: Details"
    log "- stack_name:          [${DETAILS_STACK_NAME}]"
    log "- stack_description:   [${DETAILS_STACK_DESCRIPTION}]"
    log "- deployment_timeout:  [${DETAILS_DEPLOYMENT_TIMEOUT} minutes]"
    log "- account_name:        [${DETAILS_ACCOUNT_NAME}]"
    log "- account_abbr:        [${DETAILS_ACCOUNT_ABBR}]"
    log "- account_number:      [${DETAILS_ACCOUNT_NUMBER}]"
    log "- organization_name:   [${DETAILS_ORGANIZATION_NAME}]"
    log "- organization_abbr:   [${DETAILS_ORGANIZATION_ABBR}]"
    log "- region:              [${DETAILS_REGION}]"
    log "- environment:         [${DETAILS_ENVIRONMENT}]"
    log "- project_name:        [${DETAILS_PROJECT_NAME}]"
    log "- project_abbr:        [${DETAILS_PROJECT_ABBR}]"
    log "- function_name:       [${DETAILS_FUNCTION_NAME}]"
    log "- function_abbr:       [${DETAILS_FUNCTION_ABBR}]"
    log "- owner:               [${DETAILS_OWNER}]"
    log "- contact:             [${DETAILS_CONTACT}]"
    log "- s3_bucket_temp:      [${DETAILS_S3_BUCKET_TEMP}]"
    log "- s3_bucket_artifacts: [${DETAILS_S3_BUCKET_ARTIFACTS}]"
}

###------------------------------------------------------------------------------------------------
## FUNCTION: load_variables_details_from_file_keyvalue()
## - Loads Variables: Details from KeyValue file
## - Arguments
##   - $1: Input file
function load_variables_details_from_file_keyvalue() {
    local FUNCTION_DESCRIPTION="Load Variables (Details)"
    local TMP_INPUT_FILE="${1}"
    if [ ! -f "${TMP_INPUT_FILE}" ]; then
        log_error "${FUNCTION_DESCRIPTION}: Input file not found [${TMP_INPUT_FILE}]"
        return $E_OBJECT_NOT_FOUND
    fi

    log_notice "${FUNCTION_DESCRIPTION}: loading"
    load_array_key_values_from_file "REF_DETAILS_VARIABLES_REQUIRED[@]" "${TMP_INPUT_FILE}" "DETAILS"
    load_array_key_values_from_file "REF_DETAILS_VARIABLES_OPTIONAL[@]" "${TMP_INPUT_FILE}" "DETAILS"
    if (is_empty "${DETAILS_DEPLOYMENT_TIMEOUT}"); then
        DETAILS_DEPLOYMENT_TIMEOUT=${DEFAULT_DEPLOYMENT_TIMEOUT}
    fi
    verify_array_key_values "REF_DETAILS_VARIABLES_REQUIRED[@]" "DETAILS"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "${FUNCTION_DESCRIPTION}: Failed to load required variables [Details]"
        return $RETURNVAL
    fi
}

###------------------------------------------------------------------------------------------------
## FUNCTION: load_variables_details_from_file()
## - Loads Variables: Details from Project file
## - Arguments
##   - $1: Input file
function load_variables_details_from_file() {
    local FUNCTION_DESCRIPTION="Load Variables (Details)"
    local TMP_INPUT_FILE="${1}"
    local TMP_STRING_ACCOUNT=""
    local TMP_STRING_ORGANIZATION=""
    local TMP_STRING_PROJECT=""
    local TMP_STRING_FUNCTION=""
    local TMP_STRING_OWNER=""

    if [ ! -f "${TMP_INPUT_FILE}" ]; then
        log_error "${FUNCTION_DESCRIPTION}: Input file not found [${TMP_INPUT_FILE}]"
        return $E_OBJECT_NOT_FOUND
    fi

    log_notice "${FUNCTION_DESCRIPTION}: loading"
    return_yaml_string TMP_STRING_ACCOUNT "Account" "${TMP_INPUT_FILE}"
    return_yaml_string TMP_STRING_ORGANIZATION "Organization" "${TMP_INPUT_FILE}"
    return_yaml_string TMP_STRING_PROJECT "Project" "${TMP_INPUT_FILE}"
    return_yaml_string TMP_STRING_FUNCTION "Function" "${TMP_INPUT_FILE}"
    return_yaml_string TMP_STRING_OWNER "Owner" "${TMP_INPUT_FILE}"

    return_yaml_string DETAILS_STACK_NAME "Name" "${TMP_INPUT_FILE}"
    return_yaml_string DETAILS_STACK_DESCRIPTION "Description" "${TMP_INPUT_FILE}"
    return_yaml_string DETAILS_DEPLOYMENT_TIMEOUT "DeploymentTimeout" "${TMP_INPUT_FILE}"
    if (is_empty "${DETAILS_DEPLOYMENT_TIMEOUT}"); then
        DETAILS_DEPLOYMENT_TIMEOUT=${DEFAULT_DEPLOYMENT_TIMEOUT}
    fi
    DETAILS_ACCOUNT_NAME="$(echo "${TMP_STRING_ACCOUNT}" | awk -F'|' '{print $1}' | sed -e 's/^ *//g' -e 's/ *$//g')"
    DETAILS_ACCOUNT_ABBR="$(echo "${TMP_STRING_ACCOUNT}" | awk -F'|' '{print $2}' | sed -e 's/^ *//g' -e 's/ *$//g')"
    DETAILS_ACCOUNT_NUMBER="$(echo "${TMP_STRING_ACCOUNT}" | awk -F'|' '{print $3}' | sed -e 's/^ *//g' -e 's/ *$//g')"
    DETAILS_ORGANIZATION_NAME="$(echo "${TMP_STRING_ORGANIZATION}" | awk -F'|' '{print $1}' | sed -e 's/^ *//g' -e 's/ *$//g')"
    DETAILS_ORGANIZATION_ABBR="$(echo "${TMP_STRING_ORGANIZATION}" | awk -F'|' '{print $2}' | sed -e 's/^ *//g' -e 's/ *$//g')"
    DETAILS_REGION="$(echo "${TMP_STRING_ORGANIZATION}" | awk -F'|' '{print $3}' | sed -e 's/^ *//g' -e 's/ *$//g')"
    DETAILS_ENVIRONMENT="$(echo "${TMP_STRING_ORGANIZATION}" | awk -F'|' '{print $4}' | sed -e 's/^ *//g' -e 's/ *$//g')"
    DETAILS_PROJECT_NAME="$(echo "${TMP_STRING_PROJECT}" | awk -F'|' '{print $1}' | sed -e 's/^ *//g' -e 's/ *$//g')"
    DETAILS_PROJECT_ABBR="$(echo "${TMP_STRING_PROJECT}" | awk -F'|' '{print $2}' | sed -e 's/^ *//g' -e 's/ *$//g')"
    DETAILS_FUNCTION_NAME="$(echo "${TMP_STRING_FUNCTION}" | awk -F'|' '{print $1}' | sed -e 's/^ *//g' -e 's/ *$//g')"
    DETAILS_FUNCTION_ABBR="$(echo "${TMP_STRING_FUNCTION}" | awk -F'|' '{print $2}' | sed -e 's/^ *//g' -e 's/ *$//g')"
    DETAILS_OWNER="$(echo "${TMP_STRING_OWNER}" | awk -F'|' '{print $1}' | sed -e 's/^ *//g' -e 's/ *$//g')"
    DETAILS_CONTACT="$(echo "${TMP_STRING_OWNER}" | awk -F'|' '{print $2}' | sed -e 's/^ *//g' -e 's/ *$//g')"
    DETAILS_S3_BUCKET_TEMP="${DETAILS_ACCOUNT_NUMBER}-temp-${DETAILS_REGION}"
    DETAILS_S3_BUCKET_ARTIFACTS="${DETAILS_ACCOUNT_NUMBER}-artifacts-${DETAILS_REGION}"

    verify_array_key_values "REF_DETAILS_VARIABLES_REQUIRED[@]" "DETAILS"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "${FUNCTION_DESCRIPTION}: Failed to load required variables [Details]"
        return $RETURNVAL
    fi
}

###------------------------------------------------------------------------------------------------
## FUNCTION: sam_package()
## - Packages SAM Template
## - Arguments
##   - $1: Input Template file
##   - $2: Output Template file
##   - $3: S3 Bucket (Artifacts)
##   - $4: S3 Prefix
##   - $5: Region
function sam_package() {
    local FUNCTION_DESCRIPTION="SAM (Package)"
    local TMP_FILE_TEMPLATE_INPUT="${1}"
    local TMP_FILE_TEMPLATE_OUTPUT="${2}"
    local TMP_S3_BUCKET_ARTIFACTS="${3}"
    local TMP_S3_PREFIX="${4}"
    local TMP_AWS_REGION="${5}"

    local RETURNVAL=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    if(is_empty "${TMP_FILE_TEMPLATE_INPUT}"); then
        log_error "${FUNCTION_DESCRIPTION}: Input Template file not specified"
        return $E_BAD_ARGS
    fi
    if [ ! -f "${TMP_FILE_TEMPLATE_INPUT}" ]; then
        log_error "- ${FUNCTION_DESCRIPTION}: Input Template file does not exist [${TMP_FILE_TEMPLATE_INPUT}]"
        return $E_OBJECT_NOT_FOUND
    fi
    if(is_empty "${TMP_FILE_TEMPLATE_OUTPUT}"); then
        log_error "${FUNCTION_DESCRIPTION}: Output Template file not specified"
        return $E_BAD_ARGS
    fi
    if ! $(which touch) ${TMP_FILE_TEMPLATE_OUTPUT} >/dev/null 2>&1; then
        log_error "${FUNCTION_DESCRIPTION}: Output Template file cannot be created [${TMP_FILE_TEMPLATE_OUTPUT}]"
        return $E_OBJECT_FAILED_TO_CREATE
    fi
    if(is_empty "${TMP_S3_BUCKET_ARTIFACTS}"); then
        log_error "${FUNCTION_DESCRIPTION}: S3 Bucket (Artifacts) not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_S3_PREFIX}"); then
        log_error "${FUNCTION_DESCRIPTION}: S3 Prefix not specified"
        return $E_BAD_ARGS
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    log_notice "${FUNCTION_DESCRIPTION}: started"
    line_break
    log "- Input Template:        [${TMP_FILE_TEMPLATE_INPUT}]"
    log "- Output Template:       [${TMP_FILE_TEMPLATE_OUTPUT}]"
    log "- S3 Bucket (Artifacts): [s3://${TMP_S3_BUCKET_ARTIFACTS}]"
    log "- S3 Prefix:             [${TMP_S3_PREFIX}]"
    log "- Region:                [${TMP_AWS_REGION}]"
    line_break

    log "${FUNCTION_DESCRIPTION}: Generating Package"
    $(which sam) package ${AWS_REGION_STRING} --s3-bucket "${TMP_S3_BUCKET_ARTIFACTS}" --s3-prefix "${TMP_S3_PREFIX}" --template-file "${TMP_FILE_TEMPLATE_INPUT}" --output-template-file "${TMP_FILE_TEMPLATE_OUTPUT}" >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_add_from_file "${AWS_FILE_ERROR}" "${FUNCTION_DESCRIPTION}: Data containing error" 200000
        log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Data" 200000
        log_error "${FUNCTION_DESCRIPTION}: SAM Package failed"
        return $E_AWS_FAILURE
    fi

    log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Data" 200000
    log "- successfully packaged and deployed SAM Artifact"

    return 0
}
