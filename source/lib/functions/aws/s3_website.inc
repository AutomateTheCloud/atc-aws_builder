###------------------------------------------------------------------------------------------------
# script:  s3_website.inc
# purpose: Collection of functions related to S3 Site
# version: 1.0.0
#
# function list:
#          - load_info_s3_website
#          - process_s3_website
###------------------------------------------------------------------------------------------------
REQUIRED_EXECUTABLES+=('s3_website')

###------------------------------------------------------------------------------------------------
# Variables
S3_WEBSITE_VARIABLES_REQUIRED+=(
    'S3Bucket'
)
S3_WEBSITE_VARIABLES_OPTIONAL+=(
    'DistributionId'
)
S3_WEBSITE_DISTRIBUTIONID=""
S3_WEBSITE_S3BUCKET=""

###------------------------------------------------------------------------------------------------
## FUNCTION: load_info_s3_website()
## - Loads S3 Website Information into memory from CloudFormation Outputs file
## - Arguments
##   - $1: CloudFormation Outputs File
function load_info_s3_website() {
    local FUNCTION_DESCRIPTION="Load Info (S3 Website)"
    local TMP_FILE_CLOUDFORMATION_OUTPUTS="${1}"

    if(is_empty "${TMP_FILE_CLOUDFORMATION_OUTPUTS}"); then
        log_error "${FUNCTION_DESCRIPTION}: CloudFormation Outputs file not specified"
        return $E_BAD_ARGS
    fi
    if [ ! -f "${TMP_FILE_CLOUDFORMATION_OUTPUTS}" ]; then
        log_error "${FUNCTION_DESCRIPTION}: CloudFormation Outputs file does not exist [${TMP_FILE_CLOUDFORMATION_OUTPUTS}]"
        return $E_OBJECT_NOT_FOUND
    fi

    log_notice "${FUNCTION_DESCRIPTION}: loading"
    load_array_properties_from_file "S3_WEBSITE_VARIABLES_REQUIRED[@]" "${TMP_FILE_CLOUDFORMATION_OUTPUTS}" "S3_WEBSITE"
    verify_array_key_values "S3_WEBSITE_VARIABLES_REQUIRED[@]" "S3_WEBSITE"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "${FUNCTION_DESCRIPTION}: Failed to load required variables [S3 Website Related]"
        return $RETURNVAL
    fi

    load_array_properties_from_file "S3_WEBSITE_VARIABLES_OPTIONAL[@]" "${TMP_FILE_CLOUDFORMATION_OUTPUTS}" "S3_WEBSITE"
    line_break
    log "- S3 Bucket Origin:           [${S3_WEBSITE_S3BUCKET}]"
    log "- CloudFlare Distribution:    [${S3_WEBSITE_DISTRIBUTIONID}]"
    line_break
    return 0
}

###------------------------------------------------------------------------------------------------
## FUNCTION: process_s3_website()
## - Processes S3 Website
## - Arguments
##   - $1: s3_website.yaml base
##   - $2: Static site files directory
##   - $3: Force Sync (optional, defaults to no)
##   - $4: Region
function process_s3_website() {
    local FUNCTION_DESCRIPTION="Process (S3 Website)"
    local TMP_FILE_S3_WEBSITE_YML_TEMPLATE="${1}"
    local TMP_DIRECTORY_SITE_SOURCE="${2}"
    local TMP_FORCE_SYNC="${3}"
    local TMP_AWS_REGION="${4}"

    local RETURNVAL=""
    local CMD_FORCE_SYNC=""
    local AWS_REGION_STRING=""

    local S3_WEBSITE_OUTPUT=""
    local S3_WEBSITE_WORKING_DIRECTORY=""
    local S3_WEBSITE_CONFIG_FILE=""

    if(is_empty "${TMP_FILE_S3_WEBSITE_YML_TEMPLATE}"); then
        log_error "${FUNCTION_DESCRIPTION}: s3_website.yml.template not specified"
        return $E_BAD_ARGS
    fi
    TMP_DIRECTORY_SITE_SOURCE="$(echo "${TMP_DIRECTORY_SITE_SOURCE}" | sed 's:/*$::')"
    if(is_empty "${TMP_DIRECTORY_SITE_SOURCE}"); then
        log_error "${FUNCTION_DESCRIPTION}: Site Source directory not specified"
        return $E_BAD_ARGS
    fi
    if [ ! -f "${TMP_FILE_S3_WEBSITE_YML_TEMPLATE}" ]; then
        log_error "${FUNCTION_DESCRIPTION}: s3_website.yml.template file does not exist [${TMP_FILE_S3_WEBSITE_YML_TEMPLATE}]"
        return $E_OBJECT_NOT_FOUND
    fi
    if [ ! -d "${TMP_DIRECTORY_SITE_SOURCE}" ]; then
        log_error "${FUNCTION_DESCRIPTION}: Site Source directory does not exist [${TMP_DIRECTORY_SITE_SOURCE}]"
        return $E_OBJECT_NOT_FOUND
    fi

    verify_array_key_values "S3_WEBSITE_VARIABLES_REQUIRED[@]" "S3_WEBSITE"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "${FUNCTION_DESCRIPTION}: Failed to load required variables [S3 Website Related]"
        return $RETURNVAL
    fi

    if(is_empty "${TMP_FORCE_SYNC}"); then
        TMP_FORCE_SYNC=no
    fi
    if option_enabled TMP_FORCE_SYNC; then
        CMD_FORCE_SYNC="--force"
    fi
    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="s3_endpoint: ${TMP_AWS_REGION}"
    fi

    generate_temp_file S3_WEBSITE_OUTPUT "s3 website output file"
    generate_temp_directory S3_WEBSITE_WORKING_DIRECTORY "s3 website working directory"
    S3_WEBSITE_CONFIG_FILE="${S3_WEBSITE_WORKING_DIRECTORY}/s3_website.yml"

    log "${FUNCTION_DESCRIPTION}: generating s3_website.yml file"
cat > ${S3_WEBSITE_CONFIG_FILE} << ZZEOF
${AWS_REGION_STRING}
$(cat ${TMP_FILE_S3_WEBSITE_YML_TEMPLATE})
ZZEOF

    sed -i "s|ZZ_SITE_SOURCE_ZZ|${TMP_DIRECTORY_SITE_SOURCE}|" ${S3_WEBSITE_CONFIG_FILE}
    sed -i "s|ZZ_S3_BUCKET_ZZ|${S3_WEBSITE_S3BUCKET}|" ${S3_WEBSITE_CONFIG_FILE}
    sed -i "s|ZZ_CLOUDFRONT_DISTRIBUTION_ID_ZZ|${S3_WEBSITE_DISTRIBUTIONID}|" ${S3_WEBSITE_CONFIG_FILE}

    log_add_from_file "${S3_WEBSITE_CONFIG_FILE}" "${FUNCTION_DESCRIPTION}: s3_website config"

    log "${FUNCTION_DESCRIPTION}: Executing s3_website"
    $(which s3_website) push --config-dir=${S3_WEBSITE_WORKING_DIRECTORY} ${CMD_FORCE_SYNC} >${S3_WEBSITE_OUTPUT} 2>&1
    RETURNVAL="$?"
    $(which sed) -i -r "s/\x1B\[([0-9]{1,2}(;[0-9]{1,2})?)?[mGK]//g" "${S3_WEBSITE_OUTPUT}"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_add_from_file "${S3_WEBSITE_OUTPUT}" "${FUNCTION_DESCRIPTION}: Data"
        log_error "${FUNCTION_DESCRIPTION}: operation failed (s3_website_exit_code::${RETURNVAL}])"
        return $E_S3_WEBSITE_FAILURE
    else
        log_add_from_file "${S3_WEBSITE_OUTPUT}" "${FUNCTION_DESCRIPTION}: Data"
        log "${FUNCTION_DESCRIPTION}: successfully uploaded website"
    fi

    return 0
}
