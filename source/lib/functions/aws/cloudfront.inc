###------------------------------------------------------------------------------------------------
# script:  cloudfront.inc
# purpose: Collection of functions related to CloudFront
# version: 1.0.0
#
# function list:
#          - cloudfront_invalidate_cache
###------------------------------------------------------------------------------------------------
REQUIRED_EXECUTABLES+=('aws')

###------------------------------------------------------------------------------------------------
## FUNCTION: cloudfront_invalidate_cache()
## - Invalidates CloudFront Distribution based on Path
## - Arguments
##   - $1: CloudFront Distribution ID
##   - $2: Path to Invalidate
##   - $3: Region
function cloudfront_invalidate_cache() {
    local FUNCTION_DESCRIPTION="CloudFront (Invalidate Cache)"
    local TMP_CLOUDFRONT_DISTRIBUTION_ID="${1}"
    local TMP_CLOUDFRONT_PATH="${2}"
    local TMP_AWS_REGION="${3}"

    local RETURNVAL=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    if(is_empty "${TMP_CLOUDFRONT_DISTRIBUTION_ID}"); then
        log_error "${FUNCTION_DESCRIPTION}: CloudFront Distribution ID not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_CLOUDFRONT_PATH}"); then
        log_error "${FUNCTION_DESCRIPTION}: Path to Invalidate not specified"
        return $E_BAD_ARGS
    fi
    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    log_notice "${FUNCTION_DESCRIPTION}: started"
    log "- CloudFront Distribution ID: [${TMP_CLOUDFRONT_DISTRIBUTION_ID}]"
    log "- Path to Invalidate:         [${TMP_CLOUDFRONT_PATH}]"
    log "- Region:                     [${TMP_AWS_REGION}]"

    $(which aws) ${AWS_REGION_STRING} cloudfront create-invalidation --distribution-id "${TMP_CLOUDFRONT_DISTRIBUTION_ID}" --paths "${TMP_CLOUDFRONT_PATH}" >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
    RETURNVAL="$?"
    $(which sed) -i "s/\o15/_AWS_BAD_IGNORE\\n/g" "${AWS_FILE_RESPONSE}"
    $(which sed) -i '/_AWS_BAD_IGNORE/d' "${AWS_FILE_RESPONSE}"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_add_from_file "${AWS_FILE_ERROR}" "${FUNCTION_DESCRIPTION}: Data containing error" 200000
        log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Data" 200000
        log_error "${FUNCTION_DESCRIPTION}: Failed to Invalidate Cache"
        return $E_AWS_FAILURE
    fi
    log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Data" 200000
    log "${FUNCTION_DESCRIPTION}: successfully sent Invalidation Request"
    return 0
}
