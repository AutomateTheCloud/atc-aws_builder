###------------------------------------------------------------------------------------------------
# script:  codedeploy.inc
# purpose: Collection of functions related to AWS (CodeDeploy)
# version: 1.0.0
#
# function list:
#          - codedeploy_create_bootstrap_deployment_group
#          - codedeploy_create_deployment
#          - codedeploy_delete_deployment_group
#          - codedeploy_get_current_revision_json
#          - codedeploy_get_deployed_revisions
#          - codedeploy_get_deployment
#          - codedeploy_get_deployment_revision_s3_bucket
#          - codedeploy_get_deployment_revision_s3_key
#          - codedeploy_get_service_role_arn
#          - codedeploy_register_revision
###------------------------------------------------------------------------------------------------
REQUIRED_EXECUTABLES+=('aws')

###------------------------------------------------------------------------------------------------
# Variables
AWS_CODEDEPLOY_DEFAULT_RETRY_COUNT=3
AWS_CODEDEPLOY_DEFAULT_RETRY_TIMER_MAX_SEC=30
AWS_CODEDEPLOY_DEFAULT_OPERATION_TIMEOUT=30
AWS_CODEDEPLOY_DEFAULT_VERIFICATION_TIMEOUT=30
AWS_CODEDEPLOY_DEFAULT_VERIFICATION_SLEEP=30

###------------------------------------------------------------------------------------------------
## FUNCTION: codedeploy_create_bootstrap_deployment_group()
## - Creates Bootstrap Deployment Group
## - Arguments
##   - $1: CodeDeploy Application Name to base Bootstrap Deployment on
##   - $2: CodeDeploy Deployment Group Name to base Bootstrap Deployment on
##   - $3: Bootstrap Deployment UUID
##   - $4: Region
function codedeploy_create_bootstrap_deployment_group() {
    local FUNCTION_DESCRIPTION="CodeDeploy (Create Bootstrap Deployment Group)"
    local TMP_CODEDEPLOY_APPLICATION_NAME="${1}"
    local TMP_CODEDEPLOY_DEPLOYMENT_GROUP="${2}"
    local TMP_BOOTSTRAP_CODEDEPLOY_DEPLOYMENT_GROUP="${3}"
    local TMP_AWS_REGION="${4}"

    local TMP_BOOTSTRAP_SERVICE_ROLE_ARN=""
    local TMP_INSTANCE_ID=""

    local RETURNVAL=""
    local TMP_STRING=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    if(is_empty "${TMP_CODEDEPLOY_APPLICATION_NAME}"); then
        log_error "${FUNCTION_DESCRIPTION}: CodeDeploy Application Name not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_CODEDEPLOY_DEPLOYMENT_GROUP}"); then
        log_error "${FUNCTION_DESCRIPTION}: CodeDeploy Group Name not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_BOOTSTRAP_CODEDEPLOY_DEPLOYMENT_GROUP}"); then
        log_error "${FUNCTION_DESCRIPTION}: Bootstrap Deployment Group not specified"
        return $E_BAD_ARGS
    fi
    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    codedeploy_get_service_role_arn TMP_BOOTSTRAP_SERVICE_ROLE_ARN "${TMP_CODEDEPLOY_APPLICATION_NAME}" "${TMP_CODEDEPLOY_DEPLOYMENT_GROUP}" "${TMP_AWS_REGION}"
    RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then return $RETURNVAL; fi

    line_break
    log "- CodeDeploy Application Name: [${TMP_CODEDEPLOY_APPLICATION_NAME}]"
    log "- CodeDeploy Deployment Group: [${TMP_CODEDEPLOY_DEPLOYMENT_GROUP}]"
    log "- Bootstrap Deployment Group:  [${TMP_BOOTSTRAP_CODEDEPLOY_DEPLOYMENT_GROUP}]"
    log "- Service Role ARN:            [${TMP_BOOTSTRAP_SERVICE_ROLE_ARN}]"
    log "- AWS Region:                  [${TMP_AWS_REGION}]"
    line_break

    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_CODEDEPLOY_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_CODEDEPLOY_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_CODEDEPLOY_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        log_notice "${FUNCTION_DESCRIPTION}: creating Bootstrap Deployment Group (Attempt::${COUNTER} of ${AWS_CODEDEPLOY_DEFAULT_RETRY_COUNT})"
        $(which aws) ${AWS_REGION_STRING} deploy create-deployment-group --application-name "${TMP_CODEDEPLOY_APPLICATION_NAME}" --deployment-group-name "${TMP_BOOTSTRAP_CODEDEPLOY_DEPLOYMENT_GROUP}" --deployment-config-name "CodeDeployDefault.AllAtOnce" --ec2-tag-filters Key=BootstrapCodeDeployDeploymentGroup,Value=${TMP_BOOTSTRAP_CODEDEPLOY_DEPLOYMENT_GROUP},Type=KEY_AND_VALUE --service-role-arn "${TMP_BOOTSTRAP_SERVICE_ROLE_ARN}" >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
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
            call_sleep 5 "allowing CodeDeploy time to warmup"
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: codedeploy_create_deployment()
## - Deploys specified Revision via CodeDeploy
## - Arguments
##   - $1: Variable name to pass info to
##   - $2: CodeDeploy Application Name
##   - $3: CodeDeploy Group Name
##   - $4: CodeDeploy Deployment Config
##   - $5: S3 Bucket
##   - $6: Revision Location
##   - $7: Deployment Description (optional)
##   - $8: Region
function codedeploy_create_deployment() {
    local FUNCTION_DESCRIPTION="CodeDeploy (Create Deployment)"
    local __RESULTVAR=$1
    local TMP_CODEDEPLOY_APPLICATION_NAME="${2}"
    local TMP_CODEDEPLOY_DEPLOYMENT_GROUP="${3}"
    local TMP_CODEDEPLOY_DEPLOYMENT_CONFIG="${4}"
    local TMP_S3_BUCKET="${5}"
    local TMP_REVISION_LOCATION="${6}"
    local TMP_DEPLOYMENT_DESCRIPTION="${7}"
    local TMP_AWS_REGION="${8}"

    local RETURNVAL=""
    local TMP_STRING=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    if(is_empty "${TMP_CODEDEPLOY_APPLICATION_NAME}"); then
        log_error "${FUNCTION_DESCRIPTION}: CodeDeploy Application Name not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_CODEDEPLOY_DEPLOYMENT_GROUP}"); then
        log_error "${FUNCTION_DESCRIPTION}: CodeDeploy Group Name not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_CODEDEPLOY_DEPLOYMENT_CONFIG}"); then
        log_error "${FUNCTION_DESCRIPTION}: CodeDeploy Deployment Config not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_S3_BUCKET}"); then
        log_error "${FUNCTION_DESCRIPTION}: S3 Bucket not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_REVISION_LOCATION}"); then
        log_error "${FUNCTION_DESCRIPTION}: Revision Location not specified"
        return $E_BAD_ARGS
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    log "${FUNCTION_DESCRIPTION}: started"
    log "- CodeDeploy Application Name: [${TMP_CODEDEPLOY_APPLICATION_NAME}]"
    log "- CodeDeploy Deployment Group: [${TMP_CODEDEPLOY_DEPLOYMENT_GROUP}]"
    log "- CodeDeploy Revision:         [${TMP_REVISION_LOCATION}]"
    log "- S3 Bucket:                   [${TMP_S3_BUCKET}]"
    log "- AWS Region:                  [${TMP_AWS_REGION}]"
    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_CODEDEPLOY_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_CODEDEPLOY_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_CODEDEPLOY_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        log "${FUNCTION_DESCRIPTION}: Deploying Revision (Attempt::${COUNTER} of ${AWS_CODEDEPLOY_DEFAULT_RETRY_COUNT})"
        $(which aws) ${AWS_REGION_STRING} deploy create-deployment --application-name "${TMP_CODEDEPLOY_APPLICATION_NAME}" --deployment-config-name "${TMP_CODEDEPLOY_DEPLOYMENT_CONFIG}" --deployment-group-name "${TMP_CODEDEPLOY_DEPLOYMENT_GROUP}" --description "${TMP_DEPLOYMENT_DESCRIPTION}" --s3-location bucket=${TMP_S3_BUCKET},bundleType=${TMP_REVISION_LOCATION##*.},key=${TMP_REVISION_LOCATION} >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
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
            TMP_STRING="$(cat ${AWS_FILE_RESPONSE} | grep '"deploymentId":' | sed -e 's/{//g' -e 's/}//g' | cut -d',' -f1 | awk -F ": " '{print $2}' | cut -d '"' -f2)"
            log "${FUNCTION_DESCRIPTION}: successfully started Revision Deployment [deploymentId::${TMP_STRING}]"
            eval $__RESULTVAR="'${TMP_STRING}'"
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: codedeploy_delete_deployment_group()
## - Deletes specified CodeDeploy Deployment Group
## - Arguments
##   - $1: CodeDeploy Application Name
##   - $2: CodeDeploy Group Name
##   - $3: Region
function codedeploy_delete_deployment_group() {
    local FUNCTION_DESCRIPTION="CodeDeploy (Delete Deployment Group)"
    local TMP_CODEDEPLOY_APPLICATION_NAME="${1}"
    local TMP_CODEDEPLOY_DEPLOYMENT_GROUP="${2}"
    local TMP_AWS_REGION="${3}"

    local RETURNVAL=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    if(is_empty "${TMP_CODEDEPLOY_APPLICATION_NAME}"); then
        log_error "${FUNCTION_DESCRIPTION}: CodeDeploy Application Name not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_CODEDEPLOY_DEPLOYMENT_GROUP}"); then
        log_error "${FUNCTION_DESCRIPTION}: CodeDeploy Group Name not specified"
        return $E_BAD_ARGS
    fi
    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    log "${FUNCTION_DESCRIPTION}: started"
    log "- CodeDeploy Application Name: [${TMP_CODEDEPLOY_APPLICATION_NAME}]"
    log "- CodeDeploy Deployment Group: [${TMP_CODEDEPLOY_DEPLOYMENT_GROUP}]"
    log "- AWS Region:                  [${TMP_AWS_REGION}]"
    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_CODEDEPLOY_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_CODEDEPLOY_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_CODEDEPLOY_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        log "${FUNCTION_DESCRIPTION}: deleting Deployment Group (Attempt::${COUNTER} of ${AWS_CODEDEPLOY_DEFAULT_RETRY_COUNT})"
        $(which aws) ${AWS_REGION_STRING} deploy delete-deployment-group --application-name "${TMP_CODEDEPLOY_APPLICATION_NAME}" --deployment-group-name "${TMP_CODEDEPLOY_DEPLOYMENT_GROUP}" >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
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
            call_sleep 1 "allowing CodeDeploy time to warmup"
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: codedeploy_get_current_revision_json()
## - Retrieves current revision information from specified Deployment Group and dumps JSON data to specified file
## - Arguments
##   - $1: File to pass Revision JSON data to
##   - $2: CodeDeploy Application Name
##   - $3: CodeDeploy Deployment Group
##   - $4: Region
function codedeploy_get_current_revision_json() {
    local FUNCTION_DESCRIPTION="CodeDeploy (Get Current Revision JSON)"
    local TMP_FILE_JSON_REVISION_DATA="${1}"
    local TMP_CODEDEPLOY_APPLICATION_NAME="${2}"
    local TMP_CODEDEPLOY_DEPLOYMENT_GROUP="${3}"
    local TMP_AWS_REGION="${4}"

    local RETURNVAL=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    if(is_empty "${TMP_FILE_JSON_REVISION_DATA}"); then
        log_error "${FUNCTION_DESCRIPTION}: Revision JSON data output file not specified"
        return $E_BAD_ARGS
    fi
    if ! $(which touch) ${TMP_FILE_JSON_REVISION_DATA} >/dev/null 2>&1; then
        log_error "${FUNCTION_DESCRIPTION}: Revision JSON data output file cannot be created [${TMP_FILE_JSON_REVISION_DATA}]"
        return $E_OBJECT_FAILED_TO_CREATE
    fi

    if(is_empty "${TMP_CODEDEPLOY_APPLICATION_NAME}"); then
        log_error "${FUNCTION_DESCRIPTION}: CodeDeploy Application Name not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_CODEDEPLOY_DEPLOYMENT_GROUP}"); then
        log_error "${FUNCTION_DESCRIPTION}: CodeDeploy Deployment Group not specified"
        return $E_BAD_ARGS
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    log_notice "${FUNCTION_DESCRIPTION}: started"
    log "- CodeDeploy Application Name: [${TMP_CODEDEPLOY_APPLICATION_NAME}]"
    log "- CodeDeploy Deployment Group: [${TMP_CODEDEPLOY_DEPLOYMENT_GROUP}]"
    log "- AWS Region:                  [${TMP_AWS_REGION}]"
    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_CODEDEPLOY_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_CODEDEPLOY_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_OBJECT_NOT_FOUND
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_CODEDEPLOY_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        log "${FUNCTION_DESCRIPTION}: Retrieving Current Revision JSON (Attempt::${COUNTER} of ${AWS_CODEDEPLOY_DEFAULT_RETRY_COUNT})"
        $(which aws) ${AWS_REGION_STRING} deploy get-deployment-group --application-name "${TMP_CODEDEPLOY_APPLICATION_NAME}" --deployment-group-name "${TMP_CODEDEPLOY_DEPLOYMENT_GROUP}" --query 'deploymentGroupInfo.targetRevision' >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
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
            log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Current Revision"
            log "${FUNCTION_DESCRIPTION}: successfully retrieved Current Revision"
            cat ${AWS_FILE_RESPONSE} > ${TMP_FILE_JSON_REVISION_DATA}
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: codedeploy_get_deployed_revisions()
## - Retrieves deployed revisions and places the information in the specified file
## - Arguments
##   - $1: CodeDeploy Application Name
##   - $2: File to pass revisions to
##   - $3: Region
function codedeploy_get_deployed_revisions() {
    local FUNCTION_DESCRIPTION="CodeDeploy (Get Deployed Revisions)"
    local TMP_CODEDEPLOY_APPLICATION_NAME="${1}"
    local TMP_FILE_REVISION_LIST="${2}"
    local TMP_AWS_REGION="${3}"

    local RETURNVAL=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    local CURRENT_LINE=""

    if(is_empty "${TMP_CODEDEPLOY_APPLICATION_NAME}"); then
        log_error "${FUNCTION_DESCRIPTION}: CodeDeploy Application Name not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_FILE_REVISION_LIST}"); then
        log_error "${FUNCTION_DESCRIPTION}: Revision List file not specified"
        return $E_BAD_ARGS
    fi
    if [ ! -f "${TMP_FILE_REVISION_LIST}" ]; then
        log_error "${FUNCTION_DESCRIPTION}: Revision List file not found [${TMP_FILE_REVISION_LIST}]"
        return $E_OBJECT_NOT_FOUND
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    log "${FUNCTION_DESCRIPTION}: started (application: [${TMP_CODEDEPLOY_APPLICATION_NAME}])"
    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_CODEDEPLOY_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_CODEDEPLOY_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_CODEDEPLOY_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        log "${FUNCTION_DESCRIPTION}: Retrieving Deployed Revisions (application: [${TMP_CODEDEPLOY_APPLICATION_NAME}]) (Attempt::${COUNTER} of ${AWS_CODEDEPLOY_DEFAULT_RETRY_COUNT})"
        $(which aws) ${AWS_REGION_STRING} deploy list-application-revisions --application-name "${TMP_CODEDEPLOY_APPLICATION_NAME}" --deployed include --query 'revisions[*].[s3Location.key]' --output text >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
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
            > ${TMP_FILE_REVISION_LIST}
            for CURRENT_LINE in $(cat "${AWS_FILE_RESPONSE}" 2>/dev/null | grep -v "^#\|^;"); do
                echo "$(basename "${CURRENT_LINE}")" >> ${TMP_FILE_REVISION_LIST}
            done
            log_add_from_file "${TMP_FILE_REVISION_LIST}" "${FUNCTION_DESCRIPTION}: Deployed Revisions"
            log "${FUNCTION_DESCRIPTION}: successfully retrieved Deployed Revisions"
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: codedeploy_get_deployment()
## - Tracks specified Deployment Revision via CodeDeploy
## - Arguments
##   - $1: Variable name to pass info to
##   - $2: Deployment ID
##   - $3: Region
function codedeploy_get_deployment() {
    local FUNCTION_DESCRIPTION="CodeDeploy (Get Deployment)"
    local __RESULTVAR=$1
    local TMP_CODEDEPLOY_DEPLOYMENT_ID="${2}"
    local TMP_AWS_REGION="${3}"

    local RETURNVAL=""
    local TMP_STRING=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    local CD_STATUS=""
    local CD_ERROR_MESSAGE=""
    local CD_ERROR_CODE=""
    local CD_OVERVIEW_SKIPPED=""
    local CD_OVERVIEW_SUCCEEDED=""
    local CD_OVERVIEW_FAILED=""
    local CD_OVERVIEW_INPROGRESS=""
    local CD_OVERVIEW_PENDING=""

    if(is_empty "${TMP_CODEDEPLOY_DEPLOYMENT_ID}"); then
        log_error "${FUNCTION_DESCRIPTION}: Deployment ID not specified"
        return $E_BAD_ARGS
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_CODEDEPLOY_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_CODEDEPLOY_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_CODEDEPLOY_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        log "${FUNCTION_DESCRIPTION}: Getting Deployment Information (deployment_id: [${TMP_CODEDEPLOY_DEPLOYMENT_ID}]) (Attempt::${COUNTER} of ${AWS_CODEDEPLOY_DEFAULT_RETRY_COUNT})"
        $(which aws) ${AWS_REGION_STRING} deploy get-deployment --deployment-id "${TMP_CODEDEPLOY_DEPLOYMENT_ID}" >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
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
            CD_STATUS="$(cat ${AWS_FILE_RESPONSE} | grep '"status":' | sed -e 's/{//g' -e 's/}//g' | cut -d',' -f1 | awk -F ": " '{print $2}' | cut -d '"' -f2)"
            CD_ERROR_MESSAGE="$(cat ${AWS_FILE_RESPONSE} | grep '"message":' | sed -e 's/{//g' -e 's/}//g' | cut -d',' -f1 | awk -F ": " '{print $2}' | cut -d '"' -f2)"
            CD_ERROR_CODE="$(cat ${AWS_FILE_RESPONSE} | grep '"code":' | sed -e 's/{//g' -e 's/}//g' | cut -d',' -f1 | awk -F ": " '{print $2}' | cut -d '"' -f2)"
            CD_OVERVIEW_SKIPPED="$(cat ${AWS_FILE_RESPONSE} | grep '"Skipped":' | sed -e 's/{//g' -e 's/}//g' | cut -d',' -f1 | awk -F ": " '{print $2}' | cut -d '"' -f2)"
            CD_OVERVIEW_SUCCEEDED="$(cat ${AWS_FILE_RESPONSE} | grep '"Succeeded":' | sed -e 's/{//g' -e 's/}//g' | cut -d',' -f1 | awk -F ": " '{print $2}' | cut -d '"' -f2)"
            CD_OVERVIEW_FAILED="$(cat ${AWS_FILE_RESPONSE} | grep '"Failed":' | sed -e 's/{//g' -e 's/}//g' | cut -d',' -f1 | awk -F ": " '{print $2}' | cut -d '"' -f2)"
            CD_OVERVIEW_INPROGRESS="$(cat ${AWS_FILE_RESPONSE} | grep '"InProgress":' | sed -e 's/{//g' -e 's/}//g' | cut -d',' -f1 | awk -F ": " '{print $2}' | cut -d '"' -f2)"
            CD_OVERVIEW_PENDING="$(cat ${AWS_FILE_RESPONSE} | grep '"Pending":' | sed -e 's/{//g' -e 's/}//g' | cut -d',' -f1 | awk -F ": " '{print $2}' | cut -d '"' -f2)"
            log "- Status: [${CD_STATUS}]"
            log "- Overview: [Succeeded: (${CD_OVERVIEW_SUCCEEDED}) | Failed: (${CD_OVERVIEW_FAILED}) | InProgress: (${CD_OVERVIEW_INPROGRESS}) | Skipped: (${CD_OVERVIEW_SKIPPED}) | Pending: (${CD_OVERVIEW_PENDING})]"
            if(! is_empty "${CD_ERROR_CODE}"); then
                log_warning "- Error: [${CD_ERROR_CODE} - ${CD_ERROR_MESSAGE}]"
            fi
            eval $__RESULTVAR="'${CD_STATUS}'"
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: codedeploy_get_deployment_revision_s3_bucket()
## - Returns S3 Bucket for specified Deployment Group Revision
## - Arguments
##   - $1: Variable name to pass info to
##   - $2: CodeDeploy Application Name
##   - $3: CodeDeploy Group Name
##   - $4: Region
function codedeploy_get_deployment_revision_s3_bucket() {
    local FUNCTION_DESCRIPTION="AWS Metadata (Account ID)"
    local __RESULTVAR=$1
    local TMP_CODEDEPLOY_APPLICATION_NAME="${2}"
    local TMP_CODEDEPLOY_DEPLOYMENT_GROUP="${3}"
    local TMP_AWS_REGION="${4}"

    local TMP_STRING=""
    local AWS_REGION_STRING=""

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    TMP_STRING="$($(which aws) ${AWS_REGION_STRING} deploy get-deployment-group --application-name "${TMP_CODEDEPLOY_APPLICATION_NAME}" --deployment-group-name "${TMP_CODEDEPLOY_DEPLOYMENT_GROUP}" --query 'deploymentGroupInfo.targetRevision.s3Location.bucket' --output text 2>/dev/null)"

    eval $__RESULTVAR="'${TMP_STRING}'"
}

###------------------------------------------------------------------------------------------------
## FUNCTION: codedeploy_get_deployment_revision_s3_key()
## - Returns S3 Key for specified Deployment Group Revision
## - Arguments
##   - $1: Variable name to pass info to
##   - $2: CodeDeploy Application Name
##   - $3: CodeDeploy Group Name
##   - $4: Region
function codedeploy_get_deployment_revision_s3_key() {
    local FUNCTION_DESCRIPTION="AWS Metadata (Account ID)"
    local __RESULTVAR=$1
    local TMP_CODEDEPLOY_APPLICATION_NAME="${2}"
    local TMP_CODEDEPLOY_DEPLOYMENT_GROUP="${3}"
    local TMP_AWS_REGION="${4}"

    local TMP_STRING=""
    local AWS_REGION_STRING=""

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    TMP_STRING="$($(which aws) ${AWS_REGION_STRING} deploy get-deployment-group --application-name "${TMP_CODEDEPLOY_APPLICATION_NAME}" --deployment-group-name "${TMP_CODEDEPLOY_DEPLOYMENT_GROUP}" --query 'deploymentGroupInfo.targetRevision.s3Location.key' --output text 2>/dev/null)"

    eval $__RESULTVAR="'${TMP_STRING}'"
}

###------------------------------------------------------------------------------------------------
## FUNCTION: codedeploy_get_service_role_arn()
## - Retrieves CodeDeploy Service Role ARN for specified CodeDeploy Deployment Group
## - Arguments
##   - $1: Variable name to pass info to
##   - $2: CodeDeploy Application Name
##   - $3: CodeDeploy Deployment Group
##   - $4: Region
function codedeploy_get_service_role_arn() {
    local FUNCTION_DESCRIPTION="CodeDeploy (Get Service Role ARN)"
    local __RESULTVAR=$1
    local TMP_CODEDEPLOY_APPLICATION_NAME="${2}"
    local TMP_CODEDEPLOY_DEPLOYMENT_GROUP="${3}"
    local TMP_AWS_REGION="${4}"

    local RETURNVAL=""
    local TMP_STRING=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    if(is_empty "${TMP_CODEDEPLOY_APPLICATION_NAME}"); then
        log_error "${FUNCTION_DESCRIPTION}: CodeDeploy Application Name not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_CODEDEPLOY_DEPLOYMENT_GROUP}"); then
        log_error "${FUNCTION_DESCRIPTION}: CodeDeploy Deployment Group not specified"
        return $E_BAD_ARGS
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_CODEDEPLOY_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_CODEDEPLOY_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_CODEDEPLOY_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        $(which aws) ${AWS_REGION_STRING} deploy get-deployment-group --application-name "${TMP_CODEDEPLOY_APPLICATION_NAME}" --deployment-group-name "${TMP_CODEDEPLOY_DEPLOYMENT_GROUP}" --query 'deploymentGroupInfo.serviceRoleArn' --output text >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
        RETURNVAL="$?"
        if [ ${RETURNVAL} -ne 0 ]; then
            log_add_from_file "${AWS_FILE_ERROR}" "${FUNCTION_DESCRIPTION}: Data containing error"
            log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Data"
            log_error "${FUNCTION_DESCRIPTION}: operation failed (aws_cli_exit_code::${RETURNVAL}])"
            RETRY_ENABLED=yes
            # Reset Files
            > ${AWS_FILE_RESPONSE}
            > ${AWS_FILE_ERROR}
        else
            TMP_STRING="$(cat ${AWS_FILE_RESPONSE} | head -1)"
            eval $__RESULTVAR="'$(echo "$(trim "${TMP_STRING}")" | sed -e 's/^ *//g' -e 's/ *$//g')'"
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: codedeploy_register_revision()
## - Register Application Revision with CodeDeploy
## - Arguments
##   - $1: CodeDeploy Application Name
##   - $2: CodeDeploy S3 Bucket
##   - $3: CodeDeploy S3 Key
##   - $4: Region
function codedeploy_register_revision() {
    local FUNCTION_DESCRIPTION="CodeDeploy (Create Deployment)"
    local TMP_CODEDEPLOY_APPLICATION_NAME="${1}"
    local TMP_CODEDEPLOY_S3_BUCKET="${2}"
    local TMP_CODEDEPLOY_S3_KEY="${3}"
    local TMP_AWS_REGION="${4}"

    local RETURNVAL=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    if(is_empty "${TMP_CODEDEPLOY_APPLICATION_NAME}"); then
        log_error "${FUNCTION_DESCRIPTION}: CodeDeploy Application Name not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_CODEDEPLOY_S3_BUCKET}"); then
        log_error "${FUNCTION_DESCRIPTION}: CodeDeploy S3 Bucket not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_CODEDEPLOY_S3_KEY}"); then
        log_error "${FUNCTION_DESCRIPTION}: CodeDeploy S3 Key not specified"
        return $E_BAD_ARGS
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    log "${FUNCTION_DESCRIPTION}: started"
    log "- Application Name: [${TMP_CODEDEPLOY_APPLICATION_NAME}]"
    log "- S3 Bucket:        [${TMP_CODEDEPLOY_S3_BUCKET}]"
    log "- S3 Key:           [${TMP_CODEDEPLOY_S3_KEY}]"
    log "- Region:           [${TMP_AWS_REGION}]"
    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_CODEDEPLOY_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_CODEDEPLOY_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_CODEDEPLOY_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        log "${FUNCTION_DESCRIPTION}: Registering Revision (Attempt::${COUNTER} of ${AWS_CODEDEPLOY_DEFAULT_RETRY_COUNT})"
        $(which aws) ${AWS_REGION_STRING} deploy register-application-revision --application-name "${TMP_CODEDEPLOY_APPLICATION_NAME}" --s3-location "bucket=${TMP_CODEDEPLOY_S3_BUCKET},key=${TMP_CODEDEPLOY_S3_KEY},bundleType=tgz" >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
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
            log "${FUNCTION_DESCRIPTION}: successfully registered Revision"
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}
