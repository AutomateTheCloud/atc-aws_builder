###------------------------------------------------------------------------------------------------
# script:  ssm.inc
# purpose: Collection of functions related to SSM (AWS Systems Manager)
# version: 1.0.0
#
# function list:
#          - parameter_delete
#          - parameter_exists
#          - parameter_get
#          - parameter_get_file
#          - parameter_get_file_multi_part
#          - parameter_get_file_silent
#          - parameter_get_silent
#          - parameter_path_delete
#          - parameter_put
#          - parameter_put_file
#          - parameter_put_file_multi_part
#          - parameter_tag
#          - parameter_tag_path
#          - parameters_to_key_value_file
#          - parameters_to_key_value_file_multipart
#          - parameters_to_properties_file
###------------------------------------------------------------------------------------------------
REQUIRED_EXECUTABLES+=('aws')

###------------------------------------------------------------------------------------------------
# Variables
AWS_SSM_DEFAULT_RETRY_COUNT=3
AWS_SSM_DEFAULT_RETRY_TIMER_MAX_SEC=30

###------------------------------------------------------------------------------------------------
## FUNCTION: parameter_delete()
## - Deletes SSM Parameter
## - Arguments
##   - $1: Parameter
##   - $2: Skip Exists Check (optional, defaults to no)
##   - $3: Region
function parameter_delete() {
    local FUNCTION_DESCRIPTION="Parameter (Delete)"
    local TMP_PARAMETER="${1}"
    local TMP_SKIP_EXISTS_CHECK="${2}"
    local TMP_AWS_REGION="${3}"

    local AWS_REGION_STRING=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    if(is_empty "${TMP_PARAMETER}"); then
        log_error "${FUNCTION_DESCRIPTION}: Parameter not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_SKIP_EXISTS_CHECK}"); then
        TMP_SKIP_EXISTS_CHECK=no
    fi
    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws response file"
    > ${AWS_FILE_RESPONSE}
    generate_temp_file AWS_FILE_ERROR "aws error log"
    > ${AWS_FILE_ERROR}

    if ! option_enabled TMP_SKIP_EXISTS_CHECK; then
        if(! parameter_exists "${TMP_PARAMETER}" "${TMP_AWS_REGION}"); then
            log "- parameter does not exist, no need to delete"
            return 0
        fi
    fi

    log "${FUNCTION_DESCRIPTION}: started"
    log "- Parameter:        [${TMP_PARAMETER}]"
    log "- AWS Region:       [${TMP_AWS_REGION}]"

    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_SSM_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_SSM_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_SSM_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        log "${FUNCTION_DESCRIPTION}: deleting parameter (Attempt::${COUNTER} of ${AWS_SSM_DEFAULT_RETRY_COUNT})"
        $(which aws) ${AWS_REGION_STRING} ssm delete-parameter --name "${TMP_PARAMETER}"  >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
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
            sleep 0.250
            return 0
        fi
    done

    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: parameter_exists()
## - Returns true if parameter exists
## - Arguments
##   - $1: Parameter
##   - $2: Region
function parameter_exists() {
    local TMP_PARAMETER="${1}"
    local TMP_AWS_REGION="${2}"
    local AWS_REGION_STRING=""
    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi
    $(which aws) ${AWS_REGION_STRING} ssm get-parameter --name "${TMP_PARAMETER}" >/dev/null 2>&1
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        return $E_AWS_RESOURCE_NOT_FOUND
    fi
    return 0
}

###------------------------------------------------------------------------------------------------
## FUNCTION: parameter_get()
## - Retrieves SSM Parameter from AWS and passes it to specified variable
##   - $1: Variable name to pass Parameter to
##   - $2: Parameter
##   - $3: Use Crypt functionality (encrypt/decrypt) [yes/no, defaults to yes]
##   - $4: Region
function parameter_get() {
    local FUNCTION_DESCRIPTION="Parameter (Get)"
    local __RESULTVAR=$1
    local TMP_PARAMETER="${2}"
    local TMP_USE_ENCRYPTION="${3}"
    if(is_empty "${TMP_USE_ENCRYPTION}"); then
        TMP_USE_ENCRYPTION=yes
    fi
    local TMP_AWS_REGION="${4}"
    local AWS_CRYPT_STRING=""
    local TMP_STRING=""
    local AWS_REGION_STRING=""

    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""

    if option_enabled TMP_USE_ENCRYPTION; then
        AWS_CRYPT_STRING="--with-decryption"
    fi
    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws response file"
    > ${AWS_FILE_RESPONSE}
    generate_temp_file AWS_FILE_ERROR "aws error log"
    > ${AWS_FILE_ERROR}

    log "${FUNCTION_DESCRIPTION}: started"
    log "- Result Variable:  [${__RESULTVAR}]"
    log "- Parameter:        [${TMP_PARAMETER}]"
    log "- Encryption:       [${TMP_USE_ENCRYPTION}]"
    log "- AWS Region:       [${TMP_AWS_REGION}]"

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_SSM_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_SSM_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_SSM_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        log "${FUNCTION_DESCRIPTION}: retrieving parameter (Attempt::${COUNTER} of ${AWS_SSM_DEFAULT_RETRY_COUNT})"
        $(which aws) ${AWS_REGION_STRING} ssm get-parameter --name "${TMP_PARAMETER}" ${AWS_CRYPT_STRING} >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
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
            log "${FUNCTION_DESCRIPTION}: successfully retrieved parameter"
            TMP_STRING="$(cat ${AWS_FILE_RESPONSE} | grep "Value" | sed -e 's/{//g' -e 's/}//g' | cut -d',' -f1 | awk -F ": " '{print $2}' | cut -d '"' -f2)"
            eval $__RESULTVAR="'${TMP_STRING}'"
            return 0
        fi
    done

    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: parameter_get_file()
## - Retrieves SSM Parameter from AWS to generate specified file
##   - $1: File to create
##   - $2: Parameter
##   - $3: Use Crypt functionality (encrypt/decrypt) [yes/no, defaults to yes]
##   - $4: Region
function parameter_get_file() {
    local FUNCTION_DESCRIPTION="Parameter (Get File)"
    local TMP_FILE_TARGET="${1}"
    local TMP_PARAMETER="${2}"
    local TMP_USE_ENCRYPTION="${3}"
    if(is_empty "${TMP_USE_ENCRYPTION}"); then
        TMP_USE_ENCRYPTION=yes
    fi
    local TMP_AWS_REGION="${4}"
    local AWS_CRYPT_STRING=""
    local TMP_STRING=""
    local AWS_REGION_STRING=""

    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""

    if(is_empty "${TMP_PARAMETER}"); then
        log_error "${FUNCTION_DESCRIPTION}: Parameter not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_FILE_TARGET}"); then
        log_error "${FUNCTION_DESCRIPTION}: Output file not specified"
        return $E_BAD_ARGS
    fi
    $(which touch) ${TMP_FILE_TARGET} >/dev/null 2>&1
    if [ ! -f "${TMP_FILE_TARGET}" ]; then
        log_error "${FUNCTION_DESCRIPTION}: Cannot create file [${TMP_FILE_TARGET}]"
        return $E_OBJECT_FAILED_TO_CREATE
    fi

    if option_enabled TMP_USE_ENCRYPTION; then
        AWS_CRYPT_STRING="--with-decryption"
    fi
    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws response file"
    > ${AWS_FILE_RESPONSE}
    generate_temp_file AWS_FILE_ERROR "aws error log"
    > ${AWS_FILE_ERROR}

    log "${FUNCTION_DESCRIPTION}: started"
    log "- Output File:      [${TMP_FILE_TARGET}]"
    log "- Parameter:        [${TMP_PARAMETER}]"
    log "- Encryption:       [${TMP_USE_ENCRYPTION}]"
    log "- AWS Region:       [${TMP_AWS_REGION}]"

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_SSM_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_SSM_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_SSM_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        log "${FUNCTION_DESCRIPTION}: retrieving parameter (Attempt::${COUNTER} of ${AWS_SSM_DEFAULT_RETRY_COUNT})"
        $(which aws) ${AWS_REGION_STRING} ssm get-parameter --name "${TMP_PARAMETER}" ${AWS_CRYPT_STRING} >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
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
            log "${FUNCTION_DESCRIPTION}: successfully retrieved parameter"
            TMP_STRING="$(cat ${AWS_FILE_RESPONSE} | grep "Value" | sed -e 's/{//g' -e 's/}//g' | cut -d',' -f1 | awk -F ": " '{print $2}' | cut -d '"' -f2)"
            echo -n "${TMP_STRING}" | base64 --decode > ${TMP_FILE_TARGET}
            chmod 0400 ${TMP_FILE_TARGET}
            return 0
        fi
    done

    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: parameter_get_file_multi_part()
## - Retrieves SSM Parameter from AWS to generate specified file
##   - $1: File to create
##   - $2: Parameter
##   - $3: Use Crypt functionality (encrypt/decrypt) [yes/no, defaults to yes]
##   - $4: Region
function parameter_get_file_multi_part() {
    local FUNCTION_DESCRIPTION="Parameter (Get File Multi-File)"
    local TMP_FILE_TARGET="${1}"
    local TMP_PARAMETER="${2}"
    local TMP_USE_ENCRYPTION="${3}"
    if(is_empty "${TMP_USE_ENCRYPTION}"); then
        TMP_USE_ENCRYPTION=yes
    fi
    local TMP_AWS_REGION="${4}"
    local AWS_CRYPT_STRING=""
    local TMP_STRING=""
    local AWS_REGION_STRING=""

    local TMP_FILE_PARTS=""
    local TMP_FILE_ENCODED=""

    if(is_empty "${TMP_PARAMETER}"); then
        log_error "${FUNCTION_DESCRIPTION}: Parameter not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_FILE_TARGET}"); then
        log_error "${FUNCTION_DESCRIPTION}: Output file not specified"
        return $E_BAD_ARGS
    fi
    $(which touch) ${TMP_FILE_TARGET} >/dev/null 2>&1
    if [ ! -f "${TMP_FILE_TARGET}" ]; then
        log_error "${FUNCTION_DESCRIPTION}: Cannot create file [${TMP_FILE_TARGET}]"
        return $E_OBJECT_FAILED_TO_CREATE
    fi

    if option_enabled TMP_USE_ENCRYPTION; then
        AWS_CRYPT_STRING="--with-decryption"
    fi
    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file TMP_FILE_PARTS "parameter parts"
    generate_temp_file TMP_FILE_ENCODED "encoded file"

    log "${FUNCTION_DESCRIPTION}: started"
    log "- Output File:      [${TMP_FILE_TARGET}]"
    log "- Parameter:        [${TMP_PARAMETER}]"
    log "- Encryption:       [${TMP_USE_ENCRYPTION}]"
    log "- AWS Region:       [${TMP_AWS_REGION}]"

    parameters_to_key_value_file_multipart "${TMP_PARAMETER}" "${TMP_FILE_PARTS}" "${TMP_USE_ENCRYPTION}" "${TMP_AWS_REGION}"
    RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then return ${RETURNVAL}; fi

    cat ${TMP_FILE_PARTS} | sort | awk -F"]=" '{print $2}' | base64 --decode > ${TMP_FILE_TARGET}
    chmod 0400 ${TMP_FILE_TARGET}
    log "${FUNCTION_DESCRIPTION}: successfully retrieved file"
    return 0
}

###------------------------------------------------------------------------------------------------
## FUNCTION: parameter_get_file_silent()
## - Retrieves SSM Parameter from AWS to generate specified file (Does not output to screen unless there is an error)
##   - $1: File to create
##   - $2: Parameter
##   - $3: Use Crypt functionality (encrypt/decrypt) [yes/no, defaults to yes]
##   - $4: Region
function parameter_get_file_silent() {
    local FUNCTION_DESCRIPTION="Parameter (Get File)"
    local TMP_FILE_TARGET="${1}"
    local TMP_PARAMETER="${2}"
    local TMP_USE_ENCRYPTION="${3}"
    if(is_empty "${TMP_USE_ENCRYPTION}"); then
        TMP_USE_ENCRYPTION=yes
    fi
    local TMP_AWS_REGION="${4}"
    local AWS_CRYPT_STRING=""
    local TMP_STRING=""
    local AWS_REGION_STRING=""

    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""

    if(is_empty "${TMP_PARAMETER}"); then
        log_error "${FUNCTION_DESCRIPTION}: Parameter not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_FILE_TARGET}"); then
        log_error "${FUNCTION_DESCRIPTION}: Output file not specified"
        return $E_BAD_ARGS
    fi
    $(which touch) ${TMP_FILE_TARGET} >/dev/null 2>&1
    if [ ! -f "${TMP_FILE_TARGET}" ]; then
        log_error "${FUNCTION_DESCRIPTION}: Cannot create file [${TMP_FILE_TARGET}]"
        return $E_OBJECT_FAILED_TO_CREATE
    fi

    if option_enabled TMP_USE_ENCRYPTION; then
        AWS_CRYPT_STRING="--with-decryption"
    fi
    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_SSM_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_SSM_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_SSM_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        $(which aws) ${AWS_REGION_STRING} ssm get-parameter --name "${TMP_PARAMETER}" ${AWS_CRYPT_STRING} >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
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
            TMP_STRING="$(cat ${AWS_FILE_RESPONSE} | grep "Value" | sed -e 's/{//g' -e 's/}//g' | cut -d',' -f1 | awk -F ": " '{print $2}' | cut -d '"' -f2)"
            echo -n "${TMP_STRING}" | base64 --decode > ${TMP_FILE_TARGET}
            chmod 0400 ${TMP_FILE_TARGET}
            return 0
        fi
    done

    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: parameter_get_silent()
## - Retrieves SSM Parameter from AWS and passes it to specified variable (Does not output to screen unless there is an error)
##   - $1: Variable name to pass Parameter to
##   - $2: Parameter
##   - $3: Use Crypt functionality (encrypt/decrypt) [yes/no, defaults to yes]
##   - $4: Region
function parameter_get_silent() {
    local FUNCTION_DESCRIPTION="Parameter (Get)"
    local __RESULTVAR=$1
    local TMP_PARAMETER="${2}"
    local TMP_USE_ENCRYPTION="${3}"
    if(is_empty "${TMP_USE_ENCRYPTION}"); then
        TMP_USE_ENCRYPTION=yes
    fi
    local TMP_AWS_REGION="${4}"
    local AWS_CRYPT_STRING=""
    local TMP_STRING=""
    local AWS_REGION_STRING=""

    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""

    if option_enabled TMP_USE_ENCRYPTION; then
        AWS_CRYPT_STRING="--with-decryption"
    fi
    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws response file"
    > ${AWS_FILE_RESPONSE}
    generate_temp_file AWS_FILE_ERROR "aws error log"
    > ${AWS_FILE_ERROR}

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_SSM_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_SSM_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_SSM_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        $(which aws) ${AWS_REGION_STRING} ssm get-parameter --name "${TMP_PARAMETER}" ${AWS_CRYPT_STRING} >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
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
            TMP_STRING="$(cat ${AWS_FILE_RESPONSE} | grep "Value" | sed -e 's/{//g' -e 's/}//g' | cut -d',' -f1 | awk -F ": " '{print $2}' | cut -d '"' -f2)"
            eval $__RESULTVAR="'${TMP_STRING}'"
            return 0
        fi
    done

    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: parameter_path_delete()
## - Dumps parameters to file in [key]=value format
## - Arguments
##   - $1: Parameter Path
##   - $2: Region
function parameter_path_delete() {
    local FUNCTION_DESCRIPTION="Parameter Path (Delete)"
    local TMP_PARAMETER_PATH="${1}"
    local TMP_AWS_REGION="${2}"

    local AWS_REGION_STRING=""

    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""

    local OLD_IFS="${IFS}"
    local LINE=""
    local TMP_KEY=""
    local TMP_VALUE=""
    local NEXT_TOKEN=""
    local ARG_NEXT_TOKEN=""
    local ARRAY_PARAMETERS=()

    TMP_PARAMETER_PATH="$(echo "${TMP_PARAMETER_PATH}" | sed -e 's;/\+$;;')"
    if(is_empty "${TMP_PARAMETER_PATH}"); then
        log_error "${FUNCTION_DESCRIPTION}: Parameter Path not specified"
        return $E_BAD_ARGS
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws response file"
    > ${AWS_FILE_RESPONSE}
    generate_temp_file AWS_FILE_ERROR "aws error log"
    > ${AWS_FILE_ERROR}

    log "${FUNCTION_DESCRIPTION}: started"
    log "- Parameter Path:   [${TMP_PARAMETER_PATH}/]"
    log "- AWS Region:       [${TMP_AWS_REGION}]"

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_SSM_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_SSM_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_SSM_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        if(! is_empty "${NEXT_TOKEN}"); then
            log "${FUNCTION_DESCRIPTION}: getting parameters (Attempt::${COUNTER} of ${AWS_SSM_DEFAULT_RETRY_COUNT}) (token: [${NEXT_TOKEN}])"
            ARG_NEXT_TOKEN="--next-token ${NEXT_TOKEN}"
        else
            log "${FUNCTION_DESCRIPTION}: getting parameters (Attempt::${COUNTER} of ${AWS_SSM_DEFAULT_RETRY_COUNT})"
            ARG_NEXT_TOKEN=""
        fi
        > ${AWS_FILE_RESPONSE}
        > ${AWS_FILE_ERROR}
        $(which aws) ${AWS_REGION_STRING} ssm get-parameters-by-path --recursive --path "${TMP_PARAMETER_PATH}/" ${AWS_CRYPT_STRING} ${ARG_NEXT_TOKEN} --query "[NextToken,Parameters[].[Name,Value]]" --output text >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
        RETURNVAL="$?"
        # Fix Response File
        $(which sed) -i "s/\o15/_AWS_BAD_IGNORE\\n/g" "${AWS_FILE_RESPONSE}"
        $(which sed) -i '/_AWS_BAD_IGNORE/d' "${AWS_FILE_RESPONSE}"
        if [ ${RETURNVAL} -ne 0 ]; then
            log_add_from_file "${AWS_FILE_ERROR}" "${FUNCTION_DESCRIPTION}: Data containing error"
            log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Data"
            log_error "${FUNCTION_DESCRIPTION}: operation failed (aws_cli_exit_code::${RETURNVAL}])"
            RETRY_ENABLED=yes
        else
            NEXT_TOKEN="$(cat ${AWS_FILE_RESPONSE} 2>/dev/null | grep -v "^/" | grep -v "^None" | head -1 | sed -e 's/^ *//g' -e 's/ *$//g')"
            IFS=$'\n'
            for TMP_OBJECT in $(cat "${AWS_FILE_RESPONSE}" 2>/dev/null | grep "^/" | awk -F $'\t' '{print $1}'); do
                ARRAY_PARAMETERS+=(${TMP_OBJECT})
            done
            IFS="${OLD_IFS}"
            if(! is_empty "${NEXT_TOKEN}"); then
                COUNTER=0
                sleep 0.250
            else
                RUN=0
            fi
        fi
    done

    line_break
    log "- Parameter Count:  [${#ARRAY_PARAMETERS[@]}]"
    line_break
    for TMP_OBJECT in "${ARRAY_PARAMETERS[@]}"; do
        if(! is_empty "${TMP_OBJECT}"); then
            parameter_delete "${TMP_OBJECT}" yes "${TMP_AWS_REGION}"
        fi
    done

    return 0
}

###------------------------------------------------------------------------------------------------
## FUNCTION: parameter_put()
## - Creates SSM Parameter
## - Parameter values that end in an asterisk (*) will have the asterisk removed and its data masked for any visible output
## - Arguments
##   - $1: Parameter
##   - $2: Value
##   - $3: Encryption Key [optional, defaults to AWS Account Default key]
##   - $4: Description [optional]
##   - $5: Replace Parameter if exists [yes/no, defaults to yes]
##   - $6: Region
function parameter_put() {
    local FUNCTION_DESCRIPTION="Parameter (Put)"
    local TMP_PARAMETER="${1}"
    local TMP_VALUE="${2}"
    local TMP_ENCRYPTION_KEY="${3}"
    local TMP_DESCRIPTION="${4}"
    local REPLACE_PARAMETER="${5}"
    local TMP_AWS_REGION="${6}"

    local AWS_CRYPT_STRING=""
    local AWS_REGION_STRING=""

    local IS_SENSITIVE_VALUE=no

    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""

    if(is_empty "${TMP_PARAMETER}"); then
        log_error "${FUNCTION_DESCRIPTION}: Parameter not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_VALUE}"); then
        log_error "${FUNCTION_DESCRIPTION}: String not specified"
        return $E_BAD_ARGS
    fi
    if $(echo "${TMP_VALUE}" | grep '*$' >/dev/null 2>&1); then
        IS_SENSITIVE_VALUE=yes
        TMP_VALUE="${TMP_VALUE%\*}"
    fi
    if(! is_empty "${TMP_ENCRYPTION_KEY}"); then
        AWS_CRYPT_STRING="--key-id ${TMP_ENCRYPTION_KEY}"
    fi
    if(is_empty "${REPLACE_PARAMETER}"); then
        REPLACE_PARAMETER=yes
    fi
    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws response file"
    > ${AWS_FILE_RESPONSE}
    generate_temp_file AWS_FILE_ERROR "aws error log"
    > ${AWS_FILE_ERROR}

    log "${FUNCTION_DESCRIPTION}: started"
    log "- Parameter:        [${TMP_PARAMETER}]"
    if option_enabled IS_SENSITIVE_VALUE; then
        log "- Value:            [${TMP_VALUE//?/*}]"
    else
        log "- Value:            [${TMP_VALUE}]"
    fi
    log "- Key:              [${TMP_ENCRYPTION_KEY}]"
    log "- Description:      [${TMP_DESCRIPTION}]"
    log "- Replace:          [${REPLACE_PARAMETER}]"
    log "- AWS Region:       [${TMP_AWS_REGION}]"

    if option_enabled REPLACE_PARAMETER; then
        log "${FUNCTION_DESCRIPTION}: attempting to remove parameter if it exists [replace enabled]"
        parameter_delete "${TMP_PARAMETER}" no "${TMP_AWS_REGION}"
    fi

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_SSM_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_SSM_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_SSM_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        log "${FUNCTION_DESCRIPTION}: creating parameter (Attempt::${COUNTER} of ${AWS_SSM_DEFAULT_RETRY_COUNT})"
        $(which aws) ${AWS_REGION_STRING} ssm put-parameter --name "${TMP_PARAMETER}" --value "${TMP_VALUE}" --type "SecureString" --description "${TMP_DESCRIPTION}" --overwrite ${AWS_CRYPT_STRING} >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
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
            log "${FUNCTION_DESCRIPTION}: successfully created parameter"
            return 0
        fi
    done

    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: parameter_put_file()
## - Creates SSM Parameter from a file (base64 encoded)
## - Arguments
##   - $1: Parameter
##   - $2: Filename
##   - $3: Encryption Key [optional, defaults to AWS Account Default key]
##   - $4: Description [optional]
##   - $5: Replace Parameter if exists [yes/no, defaults to yes]
##   - $6: Region
function parameter_put_file() {
    local FUNCTION_DESCRIPTION="Parameter (Put File)"
    local TMP_PARAMETER="${1}"
    local TMP_FILE_TARGET="${2}"
    local TMP_ENCRYPTION_KEY="${3}"
    local TMP_DESCRIPTION="${4}"
    local REPLACE_PARAMETER="${5}"
    local TMP_AWS_REGION="${6}"

    local TMP_VALUE=""
    local AWS_CRYPT_STRING=""
    local AWS_REGION_STRING=""

    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""

    if(is_empty "${TMP_PARAMETER}"); then
        log_error "${FUNCTION_DESCRIPTION}: Parameter not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_FILE_TARGET}"); then
        log_error "${FUNCTION_DESCRIPTION}: Target file not specified"
        return $E_BAD_ARGS
    fi
    if [ ! -f "${TMP_FILE_TARGET}" ]; then
        log_error "${FUNCTION_DESCRIPTION}: File target not does not exist [${TMP_FILE_TARGET}]"
        return $E_OBJECT_NOT_FOUND
    fi

    if(! is_empty "${TMP_ENCRYPTION_KEY}"); then
        AWS_CRYPT_STRING="--key-id ${TMP_ENCRYPTION_KEY}"
    fi
    if(is_empty "${REPLACE_PARAMETER}"); then
        REPLACE_PARAMETER=yes
    fi
    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    TMP_VALUE="$(cat ${TMP_FILE_TARGET} | base64 -w 0)"
    generate_temp_file AWS_FILE_RESPONSE "aws response file"
    > ${AWS_FILE_RESPONSE}
    generate_temp_file AWS_FILE_ERROR "aws error log"
    > ${AWS_FILE_ERROR}

    log "${FUNCTION_DESCRIPTION}: started"
    log "- Parameter:        [${TMP_PARAMETER}]"
    log "- File Target:      [${TMP_FILE_TARGET}]"
    log "- Key:              [${TMP_ENCRYPTION_KEY}]"
    log "- Description:      [${TMP_DESCRIPTION}]"
    log "- Replace:          [${REPLACE_PARAMETER}]"
    log "- String:           [${TMP_VALUE}]"
    log "- AWS Region:       [${TMP_AWS_REGION}]"

    if option_enabled REPLACE_PARAMETER; then
        log "${FUNCTION_DESCRIPTION}: attempting to remove parameter if it exists [replace enabled]"
        parameter_delete "${TMP_PARAMETER}" no "${TMP_AWS_REGION}"
    fi

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_SSM_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_SSM_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_SSM_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        log "${FUNCTION_DESCRIPTION}: creating parameter (Attempt::${COUNTER} of ${AWS_SSM_DEFAULT_RETRY_COUNT})"
        $(which aws) ${AWS_REGION_STRING} ssm put-parameter --name "${TMP_PARAMETER}" --value "${TMP_VALUE}" --type "SecureString" --description "${TMP_DESCRIPTION}" --overwrite ${AWS_CRYPT_STRING} >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
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
            log "${FUNCTION_DESCRIPTION}: successfully created parameter"
            return 0
        fi
    done

    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: parameter_put_file_multi_part()
## - Creates SSM Parameter from a file (base64 encoded)
## - Separates file into 4096 bit chunks
## - Arguments
##   - $1: Parameter
##   - $2: Filename
##   - $3: Encryption Key [optional, defaults to AWS Account Default key]
##   - $4: Description [optional]
##   - $5: Replace Parameter if exists [yes/no, defaults to yes]
##   - $6: Region
function parameter_put_file_multi_part() {
    local FUNCTION_DESCRIPTION="Parameter (Put File Multi-Part)"
    local TMP_PARAMETER="${1}"
    local TMP_FILE_TARGET="${2}"
    local TMP_ENCRYPTION_KEY="${3}"
    local TMP_DESCRIPTION="${4}"
    local REPLACE_PARAMETER="${5}"
    local TMP_AWS_REGION="${6}"

    local OLD_IFS="${IFS}"
    local TMP_VALUE=""
    local TMP_PART=""
    local AWS_CRYPT_STRING=""
    local AWS_REGION_STRING=""
    local PART_COUNTER=0

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""

    if(is_empty "${TMP_PARAMETER}"); then
        log_error "${FUNCTION_DESCRIPTION}: Parameter not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_FILE_TARGET}"); then
        log_error "${FUNCTION_DESCRIPTION}: Target file not specified"
        return $E_BAD_ARGS
    fi
    if [ ! -f "${TMP_FILE_TARGET}" ]; then
        log_error "${FUNCTION_DESCRIPTION}: File target not does not exist [${TMP_FILE_TARGET}]"
        return $E_OBJECT_NOT_FOUND
    fi

    if(! is_empty "${TMP_ENCRYPTION_KEY}"); then
        AWS_CRYPT_STRING="--key-id ${TMP_ENCRYPTION_KEY}"
    fi
    if(is_empty "${REPLACE_PARAMETER}"); then
        REPLACE_PARAMETER=yes
    fi
    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws response file"
    > ${AWS_FILE_RESPONSE}
    generate_temp_file AWS_FILE_ERROR "aws error log"
    > ${AWS_FILE_ERROR}

    log "${FUNCTION_DESCRIPTION}: started"
    log "- Parameter:        [${TMP_PARAMETER}]"
    log "- File Target:      [${TMP_FILE_TARGET}]"
    log "- Key:              [${TMP_ENCRYPTION_KEY}]"
    log "- Description:      [${TMP_DESCRIPTION}]"
    log "- Replace:          [${REPLACE_PARAMETER}]"
    log "- AWS Region:       [${TMP_AWS_REGION}]"

    TMP_VALUE="$(cat ${TMP_FILE_TARGET} | base64 -w 4096)"
    IFS=$'\n'; TMP_VALUE=(${TMP_VALUE}); IFS="${OLD_IFS}"

    if option_enabled REPLACE_PARAMETER; then
        log "${FUNCTION_DESCRIPTION}: attempting to remove parameter if it exists [replace enabled]"
        parameter_path_delete "${TMP_PARAMETER}" "${TMP_AWS_REGION}"
        line_break
    fi

    for TMP_PART in "${TMP_VALUE[@]}"; do
        PART_COUNTER=$((${PART_COUNTER}+1))
        log "${FUNCTION_DESCRIPTION}: Upload: part_$(printf "%04d" ${PART_COUNTER})"
        RUN=1
        COUNTER=0
        RETRY_ENABLED=no
        HAS_ERROR=no
        while [ ${RUN} == 1 ]; do
            COUNTER=$((${COUNTER}+1))
            if [ ${COUNTER} -gt ${AWS_SSM_DEFAULT_RETRY_COUNT} ]; then
                log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_SSM_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
                return $E_AWS_FAILURE
            fi
            if option_enabled RETRY_ENABLED; then
                call_sleep_random ${AWS_SSM_DEFAULT_RETRY_TIMER_MAX_SEC}
            fi

            # Reset Temporary Variables for the current aws run
            HAS_ERROR=no
            RETURNVAL=""
            RETRY_ENABLED=no

            log "- creating parameter (Attempt::${COUNTER} of ${AWS_SSM_DEFAULT_RETRY_COUNT})"
            $(which aws) ${AWS_REGION_STRING} ssm put-parameter --name "${TMP_PARAMETER}/part_$(printf "%04d" ${PART_COUNTER})" --value "${TMP_PART}" --type "SecureString" --description "${TMP_DESCRIPTION}" --overwrite ${AWS_CRYPT_STRING} >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
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
                RUN=0
            fi
        done
    done

    log "${FUNCTION_DESCRIPTION}: successfully created parameter [parts: ${PART_COUNTER}]"
    return 0
}

###------------------------------------------------------------------------------------------------
## FUNCTION: parameter_tag()
## - Tags SSM Parameter
## - Parameter values that end in an asterisk (*) will have the asterisk removed and its data masked for any visible output
## - Arguments
##   - $1: Parameter
##   - $2: Tags [semicolon separated, values are sent as <KeyName>:<Value>;<KeyName>:<Value>]
##   - $3: Region
function parameter_tag() {
    local FUNCTION_DESCRIPTION="Parameter (Tag)"
    local TMP_PARAMETER="${1}"
    local TMP_TAGS="${2}"
    local TMP_AWS_REGION="${3}"

    local AWS_REGION_STRING=""

    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local TMP_FILE_TAGS=""

    local TMP_OBJECT=""
    local TMP_KEY=""
    local TMP_VALUE=""

    local OLD_IFS="${IFS}"

    if(is_empty "${TMP_PARAMETER}"); then
        log_error "${FUNCTION_DESCRIPTION}: Parameter not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_TAGS}"); then
        log_error "${FUNCTION_DESCRIPTION}: Tag(s) not specified"
        return $E_BAD_ARGS
    fi
    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"
    generate_temp_file TMP_FILE_TAGS "tags.json"

    log "${FUNCTION_DESCRIPTION}: started"
    log "- Parameter:        [${TMP_PARAMETER}]"
    log "- Tags:"
    > ${TMP_FILE_TAGS}
    echo "[" >> ${TMP_FILE_TAGS}
    IFS=';'
    for TMP_OBJECT in $(echo "${TMP_TAGS}"); do
        TMP_KEY="$(echo "${TMP_OBJECT}" | awk -F: '{print $1}')"
        TMP_VALUE="$(echo "${TMP_OBJECT}" | awk -F: '{print $2}')"
        log "-   ${TMP_KEY}: [${TMP_VALUE}]"
        echo "${INDENT_JSON}{ \"Key\": \"${TMP_KEY}\", \"Value\": \"${TMP_VALUE}\" }," >> ${TMP_FILE_TAGS}
    done
    IFS="${OLD_IFS}"
    echo "]" >> ${TMP_FILE_TAGS}
    # This little abomination snippet for sed is to remove the very last comma from a file
    sed -i -e '1h;1!H;$!d;${s/.*//;x};s/\(.*\),/\1 /' ${TMP_FILE_TAGS}

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_SSM_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_SSM_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_SSM_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        log "${FUNCTION_DESCRIPTION}: tagging parameter (Attempt::${COUNTER} of ${AWS_SSM_DEFAULT_RETRY_COUNT})"
        $(which aws) ${AWS_REGION_STRING} ssm add-tags-to-resource --resource-type "Parameter" --resource-id "${TMP_PARAMETER}" --tags file://${TMP_FILE_TAGS} >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
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
            log "${FUNCTION_DESCRIPTION}: successfully tagged parameter"
            return 0
        fi
    done

    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: parameter_tag_path()
## - Tags SSM Parameter Path
## - Arguments
##   - $1: Parameter Path
##   - $2: Tags [semicolon separated, values are sent as <KeyName>:<Value>;<KeyName>:<Value>]
##   - $3: Region
function parameter_tag_path() {
    local FUNCTION_DESCRIPTION="Parameter (Tag Path)"
    local TMP_PARAMETER_PATH="${1}"
    local TMP_TAGS="${2}"
    local TMP_AWS_REGION="${3}"

    local AWS_REGION_STRING=""

    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local TMP_FILE_TAGS=""

    local TMP_PARAMETER=""
    local TMP_OBJECT=""
    local TMP_KEY=""
    local TMP_VALUE=""
    local ARRAY_PARAMETERS=()

    local OLD_IFS="${IFS}"

    if(is_empty "${TMP_PARAMETER_PATH}"); then
        log_error "${FUNCTION_DESCRIPTION}: Parameter Path not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_TAGS}"); then
        log_error "${FUNCTION_DESCRIPTION}: Tag(s) not specified"
        return $E_BAD_ARGS
    fi
    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"
    generate_temp_file TMP_FILE_TAGS "tags.json"

    log "${FUNCTION_DESCRIPTION}: started"
    log "- Parameter Path:        [${TMP_PARAMETER_PATH}]"

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no

    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_SSM_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_SSM_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_SSM_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        RETURNVAL=""
        RETRY_ENABLED=no

        if(! is_empty "${NEXT_TOKEN}"); then
            log "${FUNCTION_DESCRIPTION}: getting parameters (Attempt::${COUNTER} of ${AWS_SSM_DEFAULT_RETRY_COUNT}) (token: [${NEXT_TOKEN}])"
            ARG_NEXT_TOKEN="--next-token ${NEXT_TOKEN}"
        else
            log "${FUNCTION_DESCRIPTION}: getting parameters (Attempt::${COUNTER} of ${AWS_SSM_DEFAULT_RETRY_COUNT})"
            ARG_NEXT_TOKEN=""
        fi
        > ${AWS_FILE_RESPONSE}
        > ${AWS_FILE_ERROR}
        $(which aws) ${AWS_REGION_STRING} ssm get-parameters-by-path --recursive --path "${TMP_PARAMETER_PATH}/" ${AWS_CRYPT_STRING} ${ARG_NEXT_TOKEN} --query "[NextToken,Parameters[].[Name,Value]]" --output text >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
        RETURNVAL="$?"
        # Fix Response File
        $(which sed) -i "s/\o15/_AWS_BAD_IGNORE\\n/g" "${AWS_FILE_RESPONSE}"
        $(which sed) -i '/_AWS_BAD_IGNORE/d' "${AWS_FILE_RESPONSE}"
        if [ ${RETURNVAL} -ne 0 ]; then
            log_add_from_file "${AWS_FILE_ERROR}" "${FUNCTION_DESCRIPTION}: Data containing error"
            log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Data"
            log_error "${FUNCTION_DESCRIPTION}: operation failed (aws_cli_exit_code::${RETURNVAL}])"
            RETRY_ENABLED=yes
        else
            NEXT_TOKEN="$(cat ${AWS_FILE_RESPONSE} 2>/dev/null | grep -v "^/" | grep -v "^None" | head -1 | sed -e 's/^ *//g' -e 's/ *$//g')"
            IFS=$'\n'
            for TMP_OBJECT in $(cat "${AWS_FILE_RESPONSE}" 2>/dev/null | grep "^/" | awk -F $'\t' '{print $1}'); do
                ARRAY_PARAMETERS+=(${TMP_OBJECT})
            done
            IFS="${OLD_IFS}"
            if(! is_empty "${NEXT_TOKEN}"); then
                COUNTER=0
                sleep 0.250
            else
                RUN=0
            fi
        fi
    done

    line_break
    log "- Parameter Count:  [${#ARRAY_PARAMETERS[@]}]"
    line_break
    for TMP_OBJECT in "${ARRAY_PARAMETERS[@]}"; do
        if(! is_empty "${TMP_OBJECT}"); then
            parameter_tag "${TMP_OBJECT}" "${TMP_TAGS}" "${TMP_AWS_REGION}"
            RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then return $RETURNVAL; fi
        fi
    done

    log "${FUNCTION_DESCRIPTION}: successfully tagged path"
    return 0
}

###------------------------------------------------------------------------------------------------
## FUNCTION: parameters_to_key_value_file()
## - Dumps parameters to file in [key]=value format
## - Arguments
##   - $1: Parameter Path
##   - $2: Output file
##   - $3: Use Crypt functionality (encrypt/decrypt) [yes/no, defaults to yes]
##   - $4: Region
function parameters_to_key_value_file() {
    local FUNCTION_DESCRIPTION="Parameters (to KeyValue file)"
    local TMP_PARAMETER_PATH="${1}"
    local TMP_OUTPUT_FILE="${2}"
    local TMP_USE_ENCRYPTION="${3}"
    if(is_empty "${TMP_USE_ENCRYPTION}"); then
        TMP_USE_ENCRYPTION=yes
    fi
    local TMP_AWS_REGION="${4}"

    local AWS_CRYPT_STRING=""
    local AWS_REGION_STRING=""

    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""

    local OLD_IFS="${IFS}"
    local LINE=""
    local TMP_KEY=""
    local TMP_VALUE=""
    local NEXT_TOKEN=""
    local ARG_NEXT_TOKEN=""
    local ARRAY_PARAMETERS=()

    TMP_PARAMETER_PATH="$(echo "${TMP_PARAMETER_PATH}" | sed -e 's;/\+$;;')"
    if(is_empty "${TMP_PARAMETER_PATH}"); then
        log_error "${FUNCTION_DESCRIPTION}: Parameter Path not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_OUTPUT_FILE}"); then
        log_error "${FUNCTION_DESCRIPTION}: Output file not specified"
        return $E_BAD_ARGS
    fi
    $(which touch) ${TMP_OUTPUT_FILE} >/dev/null 2>&1
    if [ ! -f "${TMP_OUTPUT_FILE}" ]; then
        log_error "${FUNCTION_DESCRIPTION}: Output file does not exist [${TMP_OUTPUT_FILE}]"
        return $E_OBJECT_NOT_FOUND
    fi
    if option_enabled TMP_USE_ENCRYPTION; then
        AWS_CRYPT_STRING="--with-decryption"
    fi
    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    > ${TMP_OUTPUT_FILE}
    generate_temp_file AWS_FILE_RESPONSE "aws response file"
    > ${AWS_FILE_RESPONSE}
    generate_temp_file AWS_FILE_ERROR "aws error log"
    > ${AWS_FILE_ERROR}

    log "${FUNCTION_DESCRIPTION}: started"
    log "- Parameter Path:   [${TMP_PARAMETER_PATH}/]"
    log "- Output File:      [${TMP_OUTPUT_FILE}]"
    log "- Decrypt:          [${TMP_USE_ENCRYPTION}]"
    log "- AWS Region:       [${TMP_AWS_REGION}]"

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_SSM_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_SSM_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_SSM_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        if(! is_empty "${NEXT_TOKEN}"); then
            log "${FUNCTION_DESCRIPTION}: getting parameters (Attempt::${COUNTER} of ${AWS_SSM_DEFAULT_RETRY_COUNT}) (token: [${NEXT_TOKEN}])"
            ARG_NEXT_TOKEN="--next-token ${NEXT_TOKEN}"
        else
            log "${FUNCTION_DESCRIPTION}: getting parameters (Attempt::${COUNTER} of ${AWS_SSM_DEFAULT_RETRY_COUNT})"
            ARG_NEXT_TOKEN=""
        fi
        > ${AWS_FILE_RESPONSE}
        > ${AWS_FILE_ERROR}
        $(which aws) ${AWS_REGION_STRING} ssm get-parameters-by-path --recursive --path "${TMP_PARAMETER_PATH}/" ${AWS_CRYPT_STRING} ${ARG_NEXT_TOKEN} --query "[NextToken,Parameters[].[Name,Value]]" --output text >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
        RETURNVAL="$?"
        # Fix Response File
        $(which sed) -i "s/\o15/_AWS_BAD_IGNORE\\n/g" "${AWS_FILE_RESPONSE}"
        $(which sed) -i '/_AWS_BAD_IGNORE/d' "${AWS_FILE_RESPONSE}"
        if [ ${RETURNVAL} -ne 0 ]; then
            log_add_from_file "${AWS_FILE_ERROR}" "${FUNCTION_DESCRIPTION}: Data containing error"
            log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Data"
            log_error "${FUNCTION_DESCRIPTION}: operation failed (aws_cli_exit_code::${RETURNVAL}])"
            RETRY_ENABLED=yes
        else
            NEXT_TOKEN="$(cat ${AWS_FILE_RESPONSE} 2>/dev/null | grep -v "^/" | grep -v "^None" | head -1 | sed -e 's/^ *//g' -e 's/ *$//g')"
            IFS=$'\n'
            for TMP_OBJECT in $(cat "${AWS_FILE_RESPONSE}" 2>/dev/null | grep "^/" | awk -F $'\t' '{print $1"::::"$2}'); do
                if ! [[ "${TMP_OBJECT}" =~ .*/files/.* ]]; then
                    ARRAY_PARAMETERS+=(${TMP_OBJECT})
                fi
            done
            IFS="${OLD_IFS}"
            if(! is_empty "${NEXT_TOKEN}"); then
                COUNTER=0
                sleep 0.250
            else
                RUN=0
            fi
        fi
    done

    > ${TMP_OUTPUT_FILE}
    echo "## START" >> ${TMP_OUTPUT_FILE}
    echo "# Generated: $(date +"${LOG_DATE_FORMAT}")" >> ${TMP_OUTPUT_FILE}
    for TMP_OBJECT in "${ARRAY_PARAMETERS[@]}"; do
        TMP_KEY="$($(which basename) "$(echo "${TMP_OBJECT}" | awk -F:::: '{print $1}' | sed -e 's/^ *//g' -e 's/ *$//g')")"
        TMP_VALUE="$(echo "${TMP_OBJECT}" | awk -F:::: '{print $2}' | sed -e 's/^ *//g' -e 's/ *$//g')"
        if(! is_empty "${TMP_KEY}"); then
            echo "[${TMP_KEY}]=${TMP_VALUE}" >> ${TMP_OUTPUT_FILE}
        fi
    done
    echo "## END" >> ${TMP_OUTPUT_FILE}

    log "${FUNCTION_DESCRIPTION}: successfully created key value file"
    return 0
}

###------------------------------------------------------------------------------------------------
## FUNCTION: parameters_to_key_value_file_multipart()
## - Dumps parameters to file in [key]=value format (for Multipart Downloads)
## - Arguments
##   - $1: Parameter Path
##   - $2: Output file
##   - $3: Use Crypt functionality (encrypt/decrypt) [yes/no, defaults to yes]
##   - $4: Region
function parameters_to_key_value_file_multipart() {
    local FUNCTION_DESCRIPTION="Parameters (to KeyValue file)"
    local TMP_PARAMETER_PATH="${1}"
    local TMP_OUTPUT_FILE="${2}"
    local TMP_USE_ENCRYPTION="${3}"
    if(is_empty "${TMP_USE_ENCRYPTION}"); then
        TMP_USE_ENCRYPTION=yes
    fi
    local TMP_AWS_REGION="${4}"

    local AWS_CRYPT_STRING=""
    local AWS_REGION_STRING=""

    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""

    local OLD_IFS="${IFS}"
    local LINE=""
    local TMP_KEY=""
    local TMP_VALUE=""
    local NEXT_TOKEN=""
    local ARG_NEXT_TOKEN=""
    local ARRAY_PARAMETERS=()

    TMP_PARAMETER_PATH="$(echo "${TMP_PARAMETER_PATH}" | sed -e 's;/\+$;;')"
    if(is_empty "${TMP_PARAMETER_PATH}"); then
        log_error "${FUNCTION_DESCRIPTION}: Parameter Path not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_OUTPUT_FILE}"); then
        log_error "${FUNCTION_DESCRIPTION}: Output file not specified"
        return $E_BAD_ARGS
    fi
    $(which touch) ${TMP_OUTPUT_FILE} >/dev/null 2>&1
    if [ ! -f "${TMP_OUTPUT_FILE}" ]; then
        log_error "${FUNCTION_DESCRIPTION}: Output file does not exist [${TMP_OUTPUT_FILE}]"
        return $E_OBJECT_NOT_FOUND
    fi
    if option_enabled TMP_USE_ENCRYPTION; then
        AWS_CRYPT_STRING="--with-decryption"
    fi
    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    > ${TMP_OUTPUT_FILE}
    generate_temp_file AWS_FILE_RESPONSE "aws response file"
    > ${AWS_FILE_RESPONSE}
    generate_temp_file AWS_FILE_ERROR "aws error log"
    > ${AWS_FILE_ERROR}

    log "${FUNCTION_DESCRIPTION}: started"
    log "- Parameter Path:   [${TMP_PARAMETER_PATH}/]"
    log "- Output File:      [${TMP_OUTPUT_FILE}]"
    log "- Decrypt:          [${TMP_USE_ENCRYPTION}]"
    log "- AWS Region:       [${TMP_AWS_REGION}]"

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_SSM_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_SSM_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_SSM_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        if(! is_empty "${NEXT_TOKEN}"); then
            log "${FUNCTION_DESCRIPTION}: getting parameters (Attempt::${COUNTER} of ${AWS_SSM_DEFAULT_RETRY_COUNT}) (token: [${NEXT_TOKEN}])"
            ARG_NEXT_TOKEN="--next-token ${NEXT_TOKEN}"
        else
            log "${FUNCTION_DESCRIPTION}: getting parameters (Attempt::${COUNTER} of ${AWS_SSM_DEFAULT_RETRY_COUNT})"
            ARG_NEXT_TOKEN=""
        fi
        > ${AWS_FILE_RESPONSE}
        > ${AWS_FILE_ERROR}
        $(which aws) ${AWS_REGION_STRING} ssm get-parameters-by-path --recursive --path "${TMP_PARAMETER_PATH}/" ${AWS_CRYPT_STRING} ${ARG_NEXT_TOKEN} --query "[NextToken,Parameters[].[Name,Value]]" --output text >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
        RETURNVAL="$?"
        # Fix Response File
        $(which sed) -i "s/\o15/_AWS_BAD_IGNORE\\n/g" "${AWS_FILE_RESPONSE}"
        $(which sed) -i '/_AWS_BAD_IGNORE/d' "${AWS_FILE_RESPONSE}"
        if [ ${RETURNVAL} -ne 0 ]; then
            log_add_from_file "${AWS_FILE_ERROR}" "${FUNCTION_DESCRIPTION}: Data containing error"
            log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Data"
            log_error "${FUNCTION_DESCRIPTION}: operation failed (aws_cli_exit_code::${RETURNVAL}])"
            RETRY_ENABLED=yes
        else
            NEXT_TOKEN="$(cat ${AWS_FILE_RESPONSE} 2>/dev/null | grep -v "^/" | grep -v "^None" | head -1 | sed -e 's/^ *//g' -e 's/ *$//g')"
            IFS=$'\n'
            for TMP_OBJECT in $(cat "${AWS_FILE_RESPONSE}" 2>/dev/null | grep "^/" | awk -F $'\t' '{print $1"::::"$2}'); do
                ARRAY_PARAMETERS+=(${TMP_OBJECT})
            done
            IFS="${OLD_IFS}"
            if(! is_empty "${NEXT_TOKEN}"); then
                COUNTER=0
                sleep 0.250
            else
                RUN=0
            fi
        fi
    done

    > ${TMP_OUTPUT_FILE}
    echo "## START" >> ${TMP_OUTPUT_FILE}
    echo "# Generated: $(date +"${LOG_DATE_FORMAT}")" >> ${TMP_OUTPUT_FILE}
    for TMP_OBJECT in "${ARRAY_PARAMETERS[@]}"; do
        TMP_KEY="$($(which basename) "$(echo "${TMP_OBJECT}" | awk -F:::: '{print $1}' | sed -e 's/^ *//g' -e 's/ *$//g')")"
        TMP_VALUE="$(echo "${TMP_OBJECT}" | awk -F:::: '{print $2}' | sed -e 's/^ *//g' -e 's/ *$//g')"
        if(! is_empty "${TMP_KEY}"); then
            echo "[${TMP_KEY}]=${TMP_VALUE}" >> ${TMP_OUTPUT_FILE}
        fi
    done
    echo "## END" >> ${TMP_OUTPUT_FILE}

    log "${FUNCTION_DESCRIPTION}: successfully created key value file"
    return 0
}

###------------------------------------------------------------------------------------------------
## FUNCTION: parameters_to_properties_file()
## - Dumps parameters to file in key=value format
## - Arguments
##   - $1: Parameter Path
##   - $2: Output file
##   - $3: Use Crypt functionality (encrypt/decrypt) [yes/no, defaults to yes]
##   - $4: Region
function parameters_to_properties_file() {
    local FUNCTION_DESCRIPTION="Parameters (to Properties file)"
    local TMP_PARAMETER_PATH="${1}"
    local TMP_OUTPUT_FILE="${2}"
    local TMP_USE_ENCRYPTION="${3}"
    if(is_empty "${TMP_USE_ENCRYPTION}"); then
        TMP_USE_ENCRYPTION=yes
    fi
    local TMP_AWS_REGION="${4}"

    local AWS_CRYPT_STRING=""
    local AWS_REGION_STRING=""

    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""

    local OLD_IFS="${IFS}"
    local LINE=""
    local TMP_KEY=""
    local TMP_VALUE=""
    local NEXT_TOKEN=""
    local ARG_NEXT_TOKEN=""
    local ARRAY_PARAMETERS=()

    TMP_PARAMETER_PATH="$(echo "${TMP_PARAMETER_PATH}" | sed -e 's;/\+$;;')"
    if(is_empty "${TMP_PARAMETER_PATH}"); then
        log_error "${FUNCTION_DESCRIPTION}: Parameter Path not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_OUTPUT_FILE}"); then
        log_error "${FUNCTION_DESCRIPTION}: Output file not specified"
        return $E_BAD_ARGS
    fi
    $(which touch) ${TMP_OUTPUT_FILE} >/dev/null 2>&1
    if [ ! -f "${TMP_OUTPUT_FILE}" ]; then
        log_error "${FUNCTION_DESCRIPTION}: Output file does not exist [${TMP_OUTPUT_FILE}]"
        return $E_OBJECT_NOT_FOUND
    fi
    if option_enabled TMP_USE_ENCRYPTION; then
        AWS_CRYPT_STRING="--with-decryption"
    fi
    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    > ${TMP_OUTPUT_FILE}
    generate_temp_file AWS_FILE_RESPONSE "aws response file"
    > ${AWS_FILE_RESPONSE}
    generate_temp_file AWS_FILE_ERROR "aws error log"
    > ${AWS_FILE_ERROR}

    log "${FUNCTION_DESCRIPTION}: started"
    log "- Parameter Path:   [${TMP_PARAMETER_PATH}/]"
    log "- Output File:      [${TMP_OUTPUT_FILE}]"
    log "- Decrypt:          [${TMP_USE_ENCRYPTION}]"
    log "- AWS Region:       [${TMP_AWS_REGION}]"

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_SSM_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_SSM_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_SSM_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        if(! is_empty "${NEXT_TOKEN}"); then
            log "${FUNCTION_DESCRIPTION}: getting parameters (Attempt::${COUNTER} of ${AWS_SSM_DEFAULT_RETRY_COUNT}) (token: [${NEXT_TOKEN}])"
            ARG_NEXT_TOKEN="--next-token ${NEXT_TOKEN}"
        else
            log "${FUNCTION_DESCRIPTION}: getting parameters (Attempt::${COUNTER} of ${AWS_SSM_DEFAULT_RETRY_COUNT})"
            ARG_NEXT_TOKEN=""
        fi
        > ${AWS_FILE_RESPONSE}
        > ${AWS_FILE_ERROR}
        $(which aws) ${AWS_REGION_STRING} ssm get-parameters-by-path --recursive --path "${TMP_PARAMETER_PATH}/" ${AWS_CRYPT_STRING} ${ARG_NEXT_TOKEN} --query "[NextToken,Parameters[].[Name,Value]]" --output text >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
        RETURNVAL="$?"
        # Fix Response File
        $(which sed) -i "s/\o15/_AWS_BAD_IGNORE\\n/g" "${AWS_FILE_RESPONSE}"
        $(which sed) -i '/_AWS_BAD_IGNORE/d' "${AWS_FILE_RESPONSE}"
        if [ ${RETURNVAL} -ne 0 ]; then
            log_add_from_file "${AWS_FILE_ERROR}" "${FUNCTION_DESCRIPTION}: Data containing error"
            log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Data"
            log_error "${FUNCTION_DESCRIPTION}: operation failed (aws_cli_exit_code::${RETURNVAL}])"
            RETRY_ENABLED=yes
        else
            NEXT_TOKEN="$(cat ${AWS_FILE_RESPONSE} 2>/dev/null | grep -v "^/" | grep -v "^None" | head -1 | sed -e 's/^ *//g' -e 's/ *$//g')"
            IFS=$'\n'
            for TMP_OBJECT in $(cat "${AWS_FILE_RESPONSE}" 2>/dev/null | grep "^/" | awk -F $'\t' '{print $1"::::"$2}'); do
                if ! [[ "${TMP_OBJECT}" =~ .*/files/.* ]]; then
                    ARRAY_PARAMETERS+=(${TMP_OBJECT})
                fi
            done
            IFS="${OLD_IFS}"
            if(! is_empty "${NEXT_TOKEN}"); then
                COUNTER=0
                sleep 0.250
            else
                RUN=0
            fi
        fi
    done

    > ${TMP_OUTPUT_FILE}
    echo "## START" >> ${TMP_OUTPUT_FILE}
    echo "# Generated: $(date +"${LOG_DATE_FORMAT}")" >> ${TMP_OUTPUT_FILE}
    for TMP_OBJECT in "${ARRAY_PARAMETERS[@]}"; do
        TMP_KEY="$($(which basename) "$(echo "${TMP_OBJECT}" | awk -F:::: '{print $1}' | sed -e 's/^ *//g' -e 's/ *$//g')")"
        TMP_VALUE="$(echo "${TMP_OBJECT}" | awk -F:::: '{print $2}' | sed -e 's/^ *//g' -e 's/ *$//g')"
        if(! is_empty "${TMP_KEY}"); then
            echo "${TMP_KEY}=${TMP_VALUE}" >> ${TMP_OUTPUT_FILE}
        fi
    done
    echo "## END" >> ${TMP_OUTPUT_FILE}

    log "${FUNCTION_DESCRIPTION}: successfully created key value file"
    return 0
}
