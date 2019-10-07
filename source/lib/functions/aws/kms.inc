###------------------------------------------------------------------------------------------------
# script:  kms.inc
# purpose: Collection of functions related to AWS (KMS)
# version: 1.0.0
#
# function list:
#          - kms_decrypt_file
#          - kms_decrypt_string
#          - kms_encrypt_file
#          - kms_encrypt_string
#          - kms_generate_envelope_key
###------------------------------------------------------------------------------------------------
REQUIRED_EXECUTABLES+=('aws')

###------------------------------------------------------------------------------------------------
# Variables
AWS_KMS_DEFAULT_RETRY_COUNT=3
AWS_KMS_DEFAULT_RETRY_TIMER_MAX_SEC=30

###------------------------------------------------------------------------------------------------
## FUNCTION: kms_decrypt_file()
## - Decrypts file using KMS
## - Arguments
##   - $1: File to Decrypt
##   - $2: Output Directory
##   - $3: Region
function kms_decrypt_file() {
    local FUNCTION_DESCRIPTION="KMS (Decrypt File)"
    local TMP_FILE_TARGET="${1}"
    local TMP_DIRECTORY_OUTPUT="${2}"
    local TMP_AWS_REGION="${3}"

    local RETURNVAL=""
    local TMP_DIRECTORY_WORKING=""
    local TMP_FILE_OPERATION_LOG=""
    local TMP_FILE_ENCRYPTED_ENVELOPE_KEY=""
    local TMP_FILE_ENCRYPTED_BUNDLE_METADATA=""
    local TMP_FILE_ENCRYPTED_BUNDLE_PAYLOAD=""
    local TMP_FILENAME_OUTPUT=""
    local TMP_ENVELOPE_KEY_ENCRYPTED=""
    local TMP_ENVELOPE_KEY_PLAINTEXT=""

    if(is_empty "${TMP_FILE_TARGET}"); then
        log_error "${FUNCTION_DESCRIPTION}: File to Encrypt not specified"
        return $E_BAD_ARGS
    fi
    TMP_DIRECTORY_OUTPUT="$(echo "${TMP_DIRECTORY_OUTPUT}" | sed 's:/*$::')"
    if(is_empty "${TMP_DIRECTORY_OUTPUT}"); then
        log_error "${FUNCTION_DESCRIPTION}: Output Directory not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_AWS_REGION}"); then
        log_error "${FUNCTION_DESCRIPTION}: AWS Region not specified"
        return $E_BAD_ARGS
    fi
    if [ ! -f "${TMP_FILE_TARGET}" ]; then
        log_error "${FUNCTION_DESCRIPTION}: Specified File Target does not exist [${TMP_FILE_TARGET}]"
        return $E_OBJECT_NOT_FOUND
    fi
    if [ ! -d "${TMP_DIRECTORY_OUTPUT}" ]; then
        log_error "${FUNCTION_DESCRIPTION}: Specified Output Directory does not exist [${TMP_DIRECTORY_OUTPUT}]"
        return $E_OBJECT_NOT_FOUND
    fi

    generate_temp_directory TMP_DIRECTORY_WORKING "encrypted file working directory"
    generate_temp_file TMP_FILE_ENCRYPTED_ENVELOPE_KEY "encrypted envelope key"
    generate_temp_file TMP_FILE_OPERATION_LOG "operation log"

    AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    TMP_FILE_ENCRYPTED_BUNDLE_METADATA="${TMP_DIRECTORY_WORKING}/metadata"
    TMP_FILE_ENCRYPTED_BUNDLE_PAYLOAD="${TMP_DIRECTORY_WORKING}/payload"

    log_notice "${FUNCTION_DESCRIPTION}: started"
    log "- File (Encrypted):  [${TMP_FILE_TARGET}]"
    log "- Output Directory:  [${TMP_DIRECTORY_OUTPUT}]"
    log "- Region:            [${TMP_AWS_REGION}]"
    line_break
    log "- Working Directory: [${TMP_DIRECTORY_WORKING}]"
    line_break

    log "${FUNCTION_DESCRIPTION}: exploding encrypted file"
    > ${TMP_FILE_OPERATION_LOG}
    $(which tar) -zxf "${TMP_FILE_TARGET}" -C "${TMP_DIRECTORY_WORKING}" . >${TMP_FILE_OPERATION_LOG} 2>&1
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_add_from_file "${TMP_FILE_OPERATION_LOG}" "${FUNCTION_DESCRIPTION}: Data containing error"
        log_error "${FUNCTION_DESCRIPTION}: Failed to Build Encrypted file [tar_Returned::${RETURNVAL}]"
        rm -f "${FILE_ARCHIVE}"
        return $E_OBJECT_FAILED_TO_CREATE
    fi

    if [ ! -f "${TMP_FILE_ENCRYPTED_BUNDLE_METADATA}" ]; then
        log_error "${FUNCTION_DESCRIPTION}: Encryption Metadata file does not exist [${TMP_FILE_ENCRYPTED_BUNDLE_METADATA}]"
        return $E_OBJECT_NOT_FOUND
    fi
    if [ ! -f "${TMP_FILE_ENCRYPTED_BUNDLE_PAYLOAD}" ]; then
        log_error "${FUNCTION_DESCRIPTION}: Encryption Payload file does not exist [${TMP_FILE_ENCRYPTED_BUNDLE_PAYLOAD}]"
        return $E_OBJECT_NOT_FOUND
    fi

    load_key_value_from_file TMP_FILENAME_OUTPUT "filename" "${TMP_FILE_ENCRYPTED_BUNDLE_METADATA}"
    load_key_value_from_file TMP_ENVELOPE_KEY_ENCRYPTED "envelope_key" "${TMP_FILE_ENCRYPTED_BUNDLE_METADATA}"

    if(is_empty "${TMP_FILENAME_OUTPUT}"); then
        log_error "${FUNCTION_DESCRIPTION}: Failed to load encrypted file destination target name"
        return $E_OBJECT_NOT_FOUND
    fi
    if(is_empty "${TMP_FILENAME_OUTPUT}"); then
        log_error "${FUNCTION_DESCRIPTION}: Failed to load encrypted envelope key"
        return $E_OBJECT_NOT_FOUND
    fi

    log "- File (Decrypted):  [${TMP_FILENAME_OUTPUT}]"
    line_break

    echo -n "${TMP_ENVELOPE_KEY_ENCRYPTED}" | base64 --decode > ${TMP_FILE_ENCRYPTED_ENVELOPE_KEY}
    RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then return $E_BASE64_CONVERSION_FAILURE; fi

    TMP_ENVELOPE_KEY_PLAINTEXT="$($(which aws) ${AWS_REGION_STRING} kms decrypt --ciphertext-blob fileb://${TMP_FILE_ENCRYPTED_ENVELOPE_KEY} --output text --query Plaintext 2>&1)"
    if(is_empty "${TMP_ENVELOPE_KEY_PLAINTEXT}"); then
        log_error "${FUNCTION_DESCRIPTION}: Failed to generate Plaintext Envelope Key"
        return $E_AWS_FAILURE
    fi

    log "${FUNCTION_DESCRIPTION}: decrypting payload"
    > ${TMP_FILE_OPERATION_LOG}
    $(which openssl) enc -in "${TMP_FILE_ENCRYPTED_BUNDLE_PAYLOAD}" -out "${TMP_DIRECTORY_OUTPUT}/${TMP_FILENAME_OUTPUT}" -d -aes256 -k "${TMP_ENVELOPE_KEY_PLAINTEXT}" >${TMP_FILE_OPERATION_LOG} 2>&1
    RETURNVAL="$?";
    if [ ${RETURNVAL} -ne 0 ]; then
        log_add_from_file "${TMP_FILE_OPERATION_LOG}" "${FUNCTION_DESCRIPTION}: Data containing error"
        log_error "${FUNCTION_DESCRIPTION}: Failed to decrypt file [openssl_returned::${RETURNVAL}]"
        return $E_OPENSSL_FAILURE
    fi

    log "${FUNCTION_DESCRIPTION}: successfully decrypted file [${TMP_DIRECTORY_OUTPUT}/${TMP_FILENAME_OUTPUT}]"
    return 0
}

###------------------------------------------------------------------------------------------------
## FUNCTION: kms_decrypt_string()
## - Decrypts string using KMS
## - Arguments
##   - $1: Variable name to pass info to
##   - $2: Encrypted String
##   - $3: Mask Decrypted String <yes/no, defaults to no>
##   - $4: Region
function kms_decrypt_string() {
    local FUNCTION_DESCRIPTION="KMS (Decrypt String)"
    local __RESULTVAR=$1
    local STRING_ENCRYPTED="${2}"
    local MASK_STRING="${3}"
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

    local FILE_ENCRYPTED_STRING=""

    if(is_empty "${STRING_ENCRYPTED}"); then
        log_error "${FUNCTION_DESCRIPTION}: Encrypted String not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${MASK_STRING}"); then
        MASK_STRING=yes
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"
    generate_temp_file FILE_ENCRYPTED_STRING "encrypted data"

    echo -n "${STRING_ENCRYPTED}" | base64 --decode > ${FILE_ENCRYPTED_STRING}
    RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then return $E_BASE64_CONVERSION_FAILURE; fi

    log_notice "${FUNCTION_DESCRIPTION}: started"
    log "- Encrypted String: [${STRING_ENCRYPTED}]"
    log "- Region:           [${TMP_AWS_REGION}]"
    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_KMS_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_KMS_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_KMS_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        log "${FUNCTION_DESCRIPTION}: Decrypting String (Attempt::${COUNTER} of ${AWS_KMS_DEFAULT_RETRY_COUNT})"
        $(which aws) ${AWS_REGION_STRING} kms decrypt --ciphertext-blob fileb://${FILE_ENCRYPTED_STRING} >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
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
            TMP_STRING="$(cat ${AWS_FILE_RESPONSE} | grep '"Plaintext":' | sed -e 's/{//g' -e 's/}//g' | cut -d',' -f1 | awk -F ": " '{print $2}' | cut -d '"' -f2 |base64 --decode)"
            if option_enabled MASK_STRING; then
                log "${FUNCTION_DESCRIPTION}: successfully Decrypted String [decrypted_string::${TMP_STRING//?/*}]"
            else
                log "${FUNCTION_DESCRIPTION}: successfully Decrypted String [decrypted_string::${TMP_STRING}]"
            fi
            eval $__RESULTVAR="'$(echo "$(trim "${TMP_STRING}")" | sed -e 's/^ *//g' -e 's/ *$//g')'"
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: kms_encrypt_file()
## - Encrypts file using KMS
## - Generates encrypted file in specified directory using the Target Filename, appending '.enc' to the end of the filename
## - Arguments
##   - $1: File to Encrypt
##   - $2: Output Directory
##   - $3: KMS Key to use
##   - $4: Region
function kms_encrypt_file() {
    local FUNCTION_DESCRIPTION="KMS (Encrypt File)"
    local TMP_FILE_TARGET="${1}"
    local TMP_DIRECTORY_OUTPUT="${2}"
    local TMP_KMS_KEY="${3}"
    local TMP_AWS_REGION="${4}"

    local RETURNVAL=""
    local TMP_DIRECTORY_WORKING=""
    local TMP_FILE_OPERATION_LOG=""
    local TMP_FILE_ENCRYPTED_ENVELOPE_KEY=""
    local TMP_FILE_ENCRYPTED_BUNDLE_METADATA=""
    local TMP_FILE_ENCRYPTED_BUNDLE_PAYLOAD=""
    local TMP_FILENAME_TARGET=""
    local TMP_FILENAME_OUTPUT=""
    local TMP_ENVELOPE_KEY_ENCRYPTED=""
    local TMP_ENVELOPE_KEY_PLAINTEXT=""

    if(is_empty "${TMP_FILE_TARGET}"); then
        log_error "${FUNCTION_DESCRIPTION}: File to Encrypt not specified"
        return $E_BAD_ARGS
    fi
    TMP_DIRECTORY_OUTPUT="$(echo "${TMP_DIRECTORY_OUTPUT}" | sed 's:/*$::')"
    if(is_empty "${TMP_DIRECTORY_OUTPUT}"); then
        log_error "${FUNCTION_DESCRIPTION}: Output Directory not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_KMS_KEY}"); then
        log_error "${FUNCTION_DESCRIPTION}: KMS Key not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_AWS_REGION}"); then
        log_error "${FUNCTION_DESCRIPTION}: AWS Region not specified"
        return $E_BAD_ARGS
    fi
    if [ ! -f "${TMP_FILE_TARGET}" ]; then
        log_error "${FUNCTION_DESCRIPTION}: Specified File Target does not exist [${TMP_FILE_TARGET}]"
        return $E_OBJECT_NOT_FOUND
    fi
    if [ ! -d "${TMP_DIRECTORY_OUTPUT}" ]; then
        log_error "${FUNCTION_DESCRIPTION}: Specified Output Directory does not exist [${TMP_DIRECTORY_OUTPUT}]"
        return $E_OBJECT_NOT_FOUND
    fi

    generate_temp_directory TMP_DIRECTORY_WORKING "encrypted file working directory"
    generate_temp_file TMP_FILE_ENCRYPTED_ENVELOPE_KEY "encrypted envelope key"
    generate_temp_file TMP_FILE_OPERATION_LOG "operation log"

    AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    TMP_FILENAME_TARGET="$(basename "${TMP_FILE_TARGET}")"
    TMP_FILENAME_OUTPUT="${TMP_FILENAME_TARGET}.enc"
    TMP_FILE_ENCRYPTED_BUNDLE_METADATA="${TMP_DIRECTORY_WORKING}/metadata"
    TMP_FILE_ENCRYPTED_BUNDLE_PAYLOAD="${TMP_DIRECTORY_WORKING}/payload"

    log_notice "${FUNCTION_DESCRIPTION}: started"
    log "- File:              [${TMP_FILE_TARGET}]"
    log "- Output Directory:  [${TMP_DIRECTORY_OUTPUT}]"
    log "- Key:               [${TMP_KMS_KEY}]"
    log "- Region:            [${TMP_AWS_REGION}]"
    line_break
    log "- File (Encrypted):  [${TMP_FILENAME_OUTPUT}]"
    log "- Working Directory: [${TMP_DIRECTORY_WORKING}]"
    line_break

    kms_generate_envelope_key TMP_ENVELOPE_KEY_ENCRYPTED "${TMP_KMS_KEY}" "${TMP_AWS_REGION}"
    RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then return $RETURNVAL; fi

    echo -n "${TMP_ENVELOPE_KEY_ENCRYPTED}" | base64 --decode > ${TMP_FILE_ENCRYPTED_ENVELOPE_KEY}
    RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then return $E_BASE64_CONVERSION_FAILURE; fi

    TMP_ENVELOPE_KEY_PLAINTEXT="$($(which aws) ${AWS_REGION_STRING} kms decrypt --ciphertext-blob fileb://${TMP_FILE_ENCRYPTED_ENVELOPE_KEY} --output text --query Plaintext 2>&1)"
    if(is_empty "${TMP_ENVELOPE_KEY_PLAINTEXT}"); then
        log_error "${FUNCTION_DESCRIPTION}: Failed to generate Plaintext Envelope Key"
        return $E_AWS_FAILURE
    fi

    log "${FUNCTION_DESCRIPTION}: generating payload"
    > ${TMP_FILE_OPERATION_LOG}
    $(which openssl) enc -in "${TMP_FILE_TARGET}" -out "${TMP_FILE_ENCRYPTED_BUNDLE_PAYLOAD}" -e -aes256 -k "${TMP_ENVELOPE_KEY_PLAINTEXT}" >${TMP_FILE_OPERATION_LOG} 2>&1
    RETURNVAL="$?";
    if [ ${RETURNVAL} -ne 0 ]; then
        log_add_from_file "${TMP_FILE_OPERATION_LOG}" "${FUNCTION_DESCRIPTION}: Data containing error"
        log_error "${FUNCTION_DESCRIPTION}: Failed to encrypt file [openssl_returned::${RETURNVAL}]"
        return $E_OPENSSL_FAILURE
    fi

    log "${FUNCTION_DESCRIPTION}: generating metadata"
cat > ${TMP_FILE_ENCRYPTED_BUNDLE_METADATA} << ZZEOF
[filename]=${TMP_FILENAME_TARGET}
[envelope_key]=${TMP_ENVELOPE_KEY_ENCRYPTED}
[kms_key]=${TMP_KMS_KEY}
ZZEOF

    log "${FUNCTION_DESCRIPTION}: building encrypted file"
    > ${TMP_FILE_OPERATION_LOG}
    $(which tar) -zcf "${TMP_DIRECTORY_OUTPUT}/${TMP_FILENAME_OUTPUT}" -C "${TMP_DIRECTORY_WORKING}" . >${TMP_FILE_OPERATION_LOG} 2>&1
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_add_from_file "${TMP_FILE_OPERATION_LOG}" "${FUNCTION_DESCRIPTION}: Data containing error"
        log_error "${FUNCTION_DESCRIPTION}: Failed to Build Encrypted file [tar_Returned::${RETURNVAL}]"
        rm -f "${FILE_ARCHIVE}"
        return $E_OBJECT_FAILED_TO_CREATE
    fi

    log "${FUNCTION_DESCRIPTION}: successfully built encrypted file [${TMP_DIRECTORY_OUTPUT}/${TMP_FILENAME_OUTPUT}]"
    return 0
}

###------------------------------------------------------------------------------------------------
## FUNCTION: kms_encrypt_string()
## - Encrypts string using KMS
## - Arguments
##   - $1: Variable name to pass info to
##   - $2: String to encrypt
##   - $3: KMS Key to use
##   - $4: Region
function kms_encrypt_string() {
    local FUNCTION_DESCRIPTION="KMS (Encrypt String)"
    local __RESULTVAR=$1
    local STRING_DECRYPTED="${2}"
    local TMP_KMS_KEY="${3}"
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

    if(is_empty "${STRING_DECRYPTED}"); then
        log_error "${FUNCTION_DESCRIPTION}: String to Encrypt not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_KMS_KEY}"); then
        log_error "${FUNCTION_DESCRIPTION}: KMS Key not specified"
        return $E_BAD_ARGS
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    log_notice "${FUNCTION_DESCRIPTION}: started"
    log "- Key:     [${TMP_KMS_KEY}]"
    log "- String:  [${STRING_DECRYPTED}]"
    log "- Region:  [${TMP_AWS_REGION}]"
    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_KMS_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_KMS_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_KMS_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        log "${FUNCTION_DESCRIPTION}: Encrypting String (Attempt::${COUNTER} of ${AWS_KMS_DEFAULT_RETRY_COUNT})"
        $(which aws) ${AWS_REGION_STRING} kms encrypt --key-id ${TMP_KMS_KEY} --plaintext "${STRING_DECRYPTED}" >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
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
            TMP_STRING="$(cat ${AWS_FILE_RESPONSE} | grep '"CiphertextBlob":' | sed -e 's/{//g' -e 's/}//g' | cut -d',' -f1 | awk -F ": " '{print $2}' | cut -d '"' -f2)"
            log "${FUNCTION_DESCRIPTION}: successfully Encrypted String [encrypted_string::${TMP_STRING}]"
            eval $__RESULTVAR="'$(echo "$(trim "${TMP_STRING}")" | sed -e 's/^ *//g' -e 's/ *$//g')'"
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: kms_generate_envelope_key()
## - Generates Envelope Key using specified KMS Key
## - Arguments
##   - $1: Variable name to pass info to
##   - $2: KMS Key ID
##   - $3: Region
function kms_generate_envelope_key() {
    local FUNCTION_DESCRIPTION="KMS (Generate Envelope Key)"
    local __RESULTVAR=$1
    local TMP_KMS_KEY="${2}"
    local TMP_AWS_REGION="${3}"

    local RETURNVAL=""
    local TMP_STRING=""
    local AWS_REGION_STRING=""
    local AWS_FILE_ERROR=""

    if(is_empty "${TMP_KMS_KEY}"); then
        log_error "${FUNCTION_DESCRIPTION}: KMS Key not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_AWS_REGION}"); then
        log_error "${FUNCTION_DESCRIPTION}: AWS Region not specified"
        return $E_BAD_ARGS
    fi
    AWS_REGION_STRING="--region ${TMP_AWS_REGION}"

    generate_temp_file AWS_FILE_ERROR "aws error log"

    log_notice "${FUNCTION_DESCRIPTION}: started"
    log "- Key:     [${TMP_KMS_KEY}]"
    log "- Region:  [${TMP_AWS_REGION}]"

    TMP_STRING="$($(which aws) ${AWS_REGION_STRING} kms generate-data-key --key-id "${TMP_KMS_KEY}" --key-spec AES_256 --output text --query CiphertextBlob 2>${AWS_FILE_ERROR} | sed -e 's/^ *//g' -e 's/ *$//g')"
    if(is_empty "${TMP_STRING}"); then
        log_add_from_file "${AWS_FILE_ERROR}" "${FUNCTION_DESCRIPTION}: Data containing error"
        log_error "${FUNCTION_DESCRIPTION}: Failed to generate Envelope Key"
        return $E_AWS_FAILURE
    fi
    log "${FUNCTION_DESCRIPTION}: successfully generated Envelope Key"
    eval $__RESULTVAR="'${TMP_STRING}'"
    return 0
}
