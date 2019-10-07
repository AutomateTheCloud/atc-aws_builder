###------------------------------------------------------------------------------------------------
# script:  ssl.inc
# purpose: Collection of functions related to SSL Certificates in AWS
# version: 1.0.0
#
# function list:
#          - acm_certificate_exists
#          - acm_import_certificate
#          - acm_tag_certificate
#          - iam_certificate_exists
#          - iam_import_certificate
#          - ssl_download_certificate
###------------------------------------------------------------------------------------------------
REQUIRED_EXECUTABLES+=('aws' 'openssl')

###------------------------------------------------------------------------------------------------
# Variables
AWS_SSL_DEFAULT_RETRY_COUNT=3
AWS_SSL_DEFAULT_RETRY_TIMER_MAX_SEC=30

DIRECTORY_SSL_BASE="/apps/ssl"
FILENAME_SSL_CRT="server.crt"
FILENAME_SSL_CHAIN="server.chain"
FILENAME_SSL_KEY="server.key"
FILENAME_SSL_COMBINED="server.combined"
SSM_PARAMETER_SSL_CERTIFICATE_BASE="/ssl"

###--------------------------------------------------------------------------------
## FUNCTION: acm_certificate_exists()
## - Checks to see if SSL Certificate exists in ACM
## - Arguments
##   - $1: SSL Certificate Name
##   - $2: Region
function acm_certificate_exists() {
    local FUNCTION_DESCRIPTION="SSL - ACM (Does Certificate Exist)"
    local TMP_SSL_NAME="${1}"
    local TMP_AWS_REGION="${2}"

    local RETURNVAL=""
    local AWS_FILE_RESPONSE=""
    local AWS_REGION_STRING=""

    if(is_empty "${TMP_SSL_NAME}"); then
        log_error "${FUNCTION_DESCRIPTION}: SSL Certificate Name not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_AWS_REGION}"); then
        log_error "${FUNCTION_DESCRIPTION}: AWS Region not specified"
        return $E_BAD_ARGS
    fi

    AWS_REGION_STRING="--region ${TMP_AWS_REGION}"

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"

    local TMP_ARRAY_SSL_ARNS=($($(which aws) ${AWS_REGION_STRING} acm list-certificates --no-paginate --query 'CertificateSummaryList[*].CertificateArn' --output text 2>/dev/null))
    for TMP_SSL_ARN in "${TMP_ARRAY_SSL_ARNS[@]}"; do
        > ${AWS_FILE_RESPONSE}
        $(which aws) ${AWS_REGION_STRING} acm list-tags-for-certificate --certificate-arn "${TMP_SSL_ARN}" >${AWS_FILE_RESPONSE} 2>/dev/null
        if grep -q "${TMP_SSL_NAME}" ${AWS_FILE_RESPONSE}; then
            return 0
        fi
        sleep 0.50
    done

    return $E_AWS_RESOURCE_NOT_FOUND
}

###------------------------------------------------------------------------------------------------
## FUNCTION: acm_import_certificate()
## - Uploads SSL Certificate to ACM (AWS Certificate Manager)
## - Arguments
##   - $1: SSL Certificate Name
##   - $2: SSL CRT File
##   - $3: SSL Chain File
##   - $4: SSL Key File
##   - $5: Region
function acm_import_certificate() {
    local FUNCTION_DESCRIPTION="SSL - ACM (Import Certificate)"
    local TMP_SSL_NAME="${1}"
    local TMP_FILE_SSL_CRT="${2}"
    local TMP_FILE_SSL_CHAIN="${3}"
    local TMP_FILE_SSL_KEY="${4}"
    local TMP_AWS_REGION="${5}"

    local TMP_ACM_CERTIFICATE_ARN=""

    local RETURNVAL=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    if(is_empty "${TMP_SSL_NAME}"); then
        log_error "${FUNCTION_DESCRIPTION}: SSL Certificate Name not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_FILE_SSL_CRT}"); then
        log_error "${FUNCTION_DESCRIPTION}: SSL Certificate file not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_FILE_SSL_CHAIN}"); then
        log_error "${FUNCTION_DESCRIPTION}: SSL Chain file not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_FILE_SSL_KEY}"); then
        log_error "${TMP_FILE_SSL_KEY}: SSL Key file not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_AWS_REGION}"); then
        log_error "${FUNCTION_DESCRIPTION}: AWS Region not specified"
        return $E_BAD_ARGS
    fi

    if [ ! -f "${TMP_FILE_SSL_CRT}" ]; then
        log_error "${FUNCTION_DESCRIPTION}: SSL CRT file does not exist [${TMP_FILE_SSL_CRT}]"
        return $E_OBJECT_NOT_FOUND
    fi
    if [ ! -f "${TMP_FILE_SSL_CHAIN}" ]; then
        log_error "${FUNCTION_DESCRIPTION}: SSL Chain file does not exist [${TMP_FILE_SSL_CHAIN}]"
        return $E_OBJECT_NOT_FOUND
    fi
    if [ ! -f "${TMP_FILE_SSL_KEY}" ]; then
        log_error "${FUNCTION_DESCRIPTION}: SSL Key file does not exist [${TMP_FILE_SSL_KEY}]"
        return $E_OBJECT_NOT_FOUND
    fi

    AWS_REGION_STRING="--region ${TMP_AWS_REGION}"

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    log_notice "${FUNCTION_DESCRIPTION}: started"
    line_break
    log "- SSL Certificate Name: [${TMP_SSL_NAME}]"
    log "- SSL Certificate File: [${TMP_FILE_SSL_CRT}]"
    log "- SSL Chain File:       [${TMP_FILE_SSL_CHAIN}]"
    log "- SSL Key File:         [${TMP_FILE_SSL_KEY}]"
    log "- Region:               [${TMP_AWS_REGION}]"
    line_break

    if(acm_certificate_exists "${TMP_SSL_NAME}" "${TMP_AWS_REGION}"); then
        log_warning "${FUNCTION_DESCRIPTION}: SSL Certificate currently exists in ACM. If this is in error, please investigate and remove the existing ACM SSL Certificate manually"
        return 0
    fi

    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_SSL_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_SSL_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_SSL_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        log "${FUNCTION_DESCRIPTION}: importing SSL certificate to ACM (Attempt::${COUNTER} of ${AWS_SSL_DEFAULT_RETRY_COUNT})"
        $(which aws) ${AWS_REGION_STRING} acm import-certificate --certificate file://${TMP_FILE_SSL_CRT} --certificate-chain file://${TMP_FILE_SSL_CHAIN} --private-key file://${TMP_FILE_SSL_KEY} >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
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
            log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Data" 200000
            TMP_ACM_CERTIFICATE_ARN="$(cat ${AWS_FILE_RESPONSE} | grep '"CertificateArn":' | sed -e 's/{//g' -e 's/}//g' | cut -d',' -f1 | awk -F ": " '{print $2}' | cut -d '"' -f2)"
            log "${FUNCTION_DESCRIPTION}: successfully imported SSL Certificate to ACM [${TMP_ACM_CERTIFICATE_ARN}]"
            call_sleep 1
            acm_tag_certificate "${TMP_SSL_NAME}" "${TMP_ACM_CERTIFICATE_ARN}" "${TMP_AWS_REGION}"
            RETURNVAL="$?"
            call_sleep 1
            return $RETURNVAL
        fi
    done
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: acm_tag_certificate()
## - Tags SSL Certificate in ACM (AWS Certificate Manager)
## - Arguments
##   - $1: SSL Certificate Name
##   - $2: SSL Certificate ARN
##   - $3: Region
###------------------------------------------------------------------------------------------------
function acm_tag_certificate() {
    local FUNCTION_DESCRIPTION="SSL - ACM (Tag Certificate)"
    local TMP_SSL_NAME="${1}"
    local TMP_SSL_ARN="${2}"
    local TMP_AWS_REGION="${3}"

    local RETURNVAL=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    if(is_empty "${TMP_SSL_NAME}"); then
        log_error "${FUNCTION_DESCRIPTION}: SSL Certificate Name not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_SSL_ARN}"); then
        log_error "${FUNCTION_DESCRIPTION}: SSL Certificate ARN not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_AWS_REGION}"); then
        log_error "${FUNCTION_DESCRIPTION}: AWS Region not specified"
        return $E_BAD_ARGS
    fi

    AWS_REGION_STRING="--region ${TMP_AWS_REGION}"

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    log_notice "${FUNCTION_DESCRIPTION}: started"
    line_break
    log "- SSL Certificate Name: [${TMP_SSL_NAME}]"
    log "- SSL Certificate ARN:  [${TMP_SSL_ARN}]"
    log "- Region:               [${TMP_AWS_REGION}]"
    line_break

    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_SSL_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_SSL_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_SSL_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        log "${FUNCTION_DESCRIPTION}: tagging SSL certificate in ACM (Attempt::${COUNTER} of ${AWS_SSL_DEFAULT_RETRY_COUNT})"
        $(which aws) ${AWS_REGION_STRING} acm add-tags-to-certificate --certificate-arn "${TMP_SSL_ARN}" --tags "Key=Name,Value=${TMP_SSL_NAME}" >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
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
            log "${FUNCTION_DESCRIPTION}: successfully tagged SSL Certificate in ACM"
            call_sleep 1
            return 0
        fi
    done
    return $E_AWS_FAILURE
}

###--------------------------------------------------------------------------------
## FUNCTION: iam_certificate_exists()
## - Checks to see if SSL Certificate exists in IAM
## - Arguments
##   - $1: SSL Certificate Name
##   - $2: Region
function iam_certificate_exists() {
    local FUNCTION_DESCRIPTION="SSL - IAM (Does Certificate Exist)"
    local TMP_SSL_NAME="${1}"
    local TMP_AWS_REGION="${2}"

    local RETURNVAL=""
    local AWS_REGION_STRING=""

    if(is_empty "${TMP_SSL_NAME}"); then
        log_error "${FUNCTION_DESCRIPTION}: SSL Certificate Name not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_AWS_REGION}"); then
        log_error "${FUNCTION_DESCRIPTION}: AWS Region not specified"
        return $E_BAD_ARGS
    fi

    AWS_REGION_STRING="--region ${TMP_AWS_REGION}"

    $(which aws) ${AWS_REGION_STRING} iam get-server-certificate --server-certificate-name "${TMP_SSL_NAME}" --query 'ServerCertificate.ServerCertificateMetadata.{ServerCertificateName:ServerCertificateName}' --output text >/dev/null 2>&1
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        return $E_AWS_RESOURCE_NOT_FOUND
    fi
    return 0
}

###------------------------------------------------------------------------------------------------
## FUNCTION: iam_import_certificate()
## - Uploads SSL Certificate to IAM
## - Arguments
##   - $1: SSL Certificate Name
##   - $2: SSL CRT File
##   - $3: SSL Chain File
##   - $4: SSL Key File
##   - $5: Region
function iam_import_certificate() {
    local FUNCTION_DESCRIPTION="SSL - IAM (Import Certificate)"
    local TMP_SSL_NAME="${1}"
    local TMP_FILE_SSL_CRT="${2}"
    local TMP_FILE_SSL_CHAIN="${3}"
    local TMP_FILE_SSL_KEY="${4}"
    local TMP_AWS_REGION="${5}"

    local RETURNVAL=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    if(is_empty "${TMP_SSL_NAME}"); then
        log_error "${FUNCTION_DESCRIPTION}: SSL Certificate Name not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_FILE_SSL_CRT}"); then
        log_error "${FUNCTION_DESCRIPTION}: SSL Certificate file not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_FILE_SSL_CHAIN}"); then
        log_error "${FUNCTION_DESCRIPTION}: SSL Chain file not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_FILE_SSL_KEY}"); then
        log_error "${TMP_FILE_SSL_KEY}: SSL Key file not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_AWS_REGION}"); then
        log_error "${FUNCTION_DESCRIPTION}: AWS Region not specified"
        return $E_BAD_ARGS
    fi

    if [ ! -f "${TMP_FILE_SSL_CRT}" ]; then
        log_error "${FUNCTION_DESCRIPTION}: SSL CRT file does not exist [${TMP_FILE_SSL_CRT}]"
        return $E_OBJECT_NOT_FOUND
    fi
    if [ ! -f "${TMP_FILE_SSL_CHAIN}" ]; then
        log_error "${FUNCTION_DESCRIPTION}: SSL Chain file does not exist [${TMP_FILE_SSL_CHAIN}]"
        return $E_OBJECT_NOT_FOUND
    fi
    if [ ! -f "${TMP_FILE_SSL_KEY}" ]; then
        log_error "${FUNCTION_DESCRIPTION}: SSL Key file does not exist [${TMP_FILE_SSL_KEY}]"
        return $E_OBJECT_NOT_FOUND
    fi

    AWS_REGION_STRING="--region ${TMP_AWS_REGION}"

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    log_notice "${FUNCTION_DESCRIPTION}: started"
    line_break
    log "- SSL Certificate Name: [${TMP_SSL_NAME}]"
    log "- SSL Certificate File: [${TMP_FILE_SSL_CRT}]"
    log "- SSL Chain File:       [${TMP_FILE_SSL_CHAIN}]"
    log "- SSL Key File:         [${TMP_FILE_SSL_KEY}]"
    log "- Region:               [${TMP_AWS_REGION}]"
    line_break

    if(iam_certificate_exists "${TMP_SSL_NAME}" "${TMP_AWS_REGION}"); then
        log_warning "${FUNCTION_DESCRIPTION}: SSL Certificate currently exists in IAM. If this is in error, please investigate and remove the existing IAM SSL Certificate manually"
        return 0
    fi

    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_SSL_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_SSL_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_SSL_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        log "${FUNCTION_DESCRIPTION}: importing SSL certificate to IAM (Attempt::${COUNTER} of ${AWS_SSL_DEFAULT_RETRY_COUNT})"
        $(which aws) ${AWS_REGION_STRING} iam upload-server-certificate --server-certificate-name "${TMP_SSL_NAME}" --certificate-body file://${TMP_FILE_SSL_CRT} --certificate-chain file://${TMP_FILE_SSL_CHAIN} --private-key file://${TMP_FILE_SSL_KEY} >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
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
            log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Data" 200000
            log "${FUNCTION_DESCRIPTION}: successfully imported SSL Certificate to IAM"
            call_sleep 1
            return 0
        fi
    done
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: ssl_download_certificate()
## - Downloads specified SSL Certificate from AWS
## - Generates the following files:
##   - Certificate: DIRECTORY_SSL/server.crt
##   - Chain:       DIRECTORY_SSL/server.chain
##   - Key:         DIRECTORY_SSL/server.key
##   - Combined:    DIRECTORY_SSL/server.combined
## - Arguments
##   - $1: SSL Certificate Name
##   - $2: Common Name (optional, if specified, certificate will be placed in a sub directory named after this Common Name)
##   - $3: Region (optional, if not specified, Region will be loaded through AWS Metadata
function ssl_download_certificate() {
    local FUNCTION_DESCRIPTION="SSL (Download Certificate)"
    local TMP_SSL_CERT_NAME="${1}"
    local TMP_SSL_COMMON_NAME="${2}"
    local TMP_AWS_REGION="${3}"

    local TMP_DIRECTORY_SSL=""
    local TMP_FILE_SSL_CRT=""
    local TMP_FILE_SSL_KEY=""
    local TMP_FILE_SSL_CHAIN=""
    local TMP_FILE_SSL_COMBINED=""
    local TMP_SSL_CERTIFICATE_SSM_PARAMETER=""

    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local OPENSSL_FILE_OUTPUT=""

    if(! is_empty "${TMP_SSL_COMMON_NAME}"); then
        TMP_DIRECTORY_SSL="${DIRECTORY_SSL_BASE}/${TMP_SSL_COMMON_NAME}"
    else
        TMP_DIRECTORY_SSL="${DIRECTORY_SSL_BASE}"
    fi
    TMP_DIRECTORY_SSL="$(echo "${TMP_DIRECTORY_SSL}" | sed 's:/*$::')"

    if(is_empty "${TMP_AWS_REGION}"); then
        aws_metadata_region TMP_AWS_REGION
    fi

    if(is_empty "${TMP_SSL_CERT_NAME}"); then
        log_error "${FUNCTION_DESCRIPTION}: SSL Certificate Name not specified"
        return $E_BAD_ARGS
    fi

    # Sanity check to ensure we dont accidentally nuke everything
    if(is_empty "${TMP_DIRECTORY_SSL}"); then
        log_error "${FUNCTION_DESCRIPTION}: Attempted to nuke root filesystem, aborting."
        return $E_RESONANCE_CASCADE
    fi

    log_notice "${FUNCTION_DESCRIPTION}: started"

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"
    generate_temp_file OPENSSL_FILE_OUTPUT "openssl log"

    TMP_FILE_SSL_CRT="${TMP_DIRECTORY_SSL}/${FILENAME_SSL_CRT}"
    TMP_FILE_SSL_CHAIN="${TMP_DIRECTORY_SSL}/${FILENAME_SSL_CHAIN}"
    TMP_FILE_SSL_KEY="${TMP_DIRECTORY_SSL}/${FILENAME_SSL_KEY}"
    TMP_FILE_SSL_COMBINED="${TMP_DIRECTORY_SSL}/${FILENAME_SSL_COMBINED}"
    TMP_SSL_CERTIFICATE_SSM_PARAMETER="${SSM_PARAMETER_SSL_CERTIFICATE_BASE}/${TMP_SSL_CERT_NAME}/files"

    log "- Certificate Name:    [${TMP_SSL_CERT_NAME}]"
    log "- Common Name:         [${TMP_SSL_COMMON_NAME}]"
    log "- SSM Parameter (key): [ssm::${TMP_SSL_CERTIFICATE_SSM_PARAMETER}]"
    log "- Output Directory:    [${TMP_DIRECTORY_SSL}]"

    rm -rf ${TMP_DIRECTORY_SSL} >/dev/null 2>&1
    $(which mkdir) -p ${TMP_DIRECTORY_SSL} >/dev/null 2>&1

    log "${FUNCTION_DESCRIPTION}: downloading SSL Certificate file [${TMP_FILE_SSL_CRT}]"
    parameter_get_file_multi_part "${TMP_FILE_SSL_CRT}" "${TMP_SSL_CERTIFICATE_SSM_PARAMETER}/${FILENAME_SSL_CRT}" yes "${TMP_AWS_REGION}"
    RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then return $RETURNVAL; fi

    log "${FUNCTION_DESCRIPTION}: downloading SSL Chain file [${TMP_FILE_SSL_CHAIN}]"
    parameter_get_file_multi_part "${TMP_FILE_SSL_CHAIN}" "${TMP_SSL_CERTIFICATE_SSM_PARAMETER}/${FILENAME_SSL_CHAIN}" yes "${TMP_AWS_REGION}"
    RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then return $RETURNVAL; fi

    log "${FUNCTION_DESCRIPTION}: downloading SSL Key file [${TMP_FILE_SSL_KEY}]"
    parameter_get_file_multi_part "${TMP_FILE_SSL_KEY}" "${TMP_SSL_CERTIFICATE_SSM_PARAMETER}/${FILENAME_SSL_KEY}" yes "${TMP_AWS_REGION}"
    RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then return $RETURNVAL; fi

    $(which openssl) x509 -in ${TMP_FILE_SSL_CRT} -text -noout > ${OPENSSL_FILE_OUTPUT} 2>&1
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "${FUNCTION_DESCRIPTION}: Failed to validate SSL Certificate (openssl::${RETURNVAL})"
        log_add_from_file "${OPENSSL_FILE_OUTPUT}" "openssl output file"
        return $E_SSL_FAILURE
    fi

    log "${FUNCTION_DESCRIPTION}: generating SSL Combined file [${TMP_FILE_SSL_COMBINED}]"
    cat ${TMP_FILE_SSL_CRT} ${TMP_FILE_SSL_CHAIN} > ${TMP_FILE_SSL_COMBINED}
    return 0
}
