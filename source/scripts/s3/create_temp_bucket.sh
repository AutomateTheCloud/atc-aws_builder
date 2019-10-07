#!/bin/bash
###------------------------------------------------------------------------------------------------
# alias:   aws_builder.s3.create_temp_bucket
# script:  create_temp_bucket.sh
# purpose: S3 (Create Temp Bucket)
# version: 1.0.0
###------------------------------------------------------------------------------------------------

###------------------------------------------------------------------------------------------------
## Config
declare -r SELF_IDENTITY="create_temp_bucket"
declare -r SELF_IDENTITY_H="S3 (Create Temp Bucket)"
declare -a ARGUMENTS_DESCRIPTION=(
    '-b <bucket_name> : S3 Bucket Name (recommended to include Account ID in name. example: temp-123456789012)'
    '-o <owner> : Owner string for Tagging'
    '-c <contact_email> : Contact Email for Tagging'
    '-r <region> : Region'
    '-P <profile> : AWS Profile (optional)'
)

###------------------------------------------------------------------------------------------------
## Load Defaults
declare -r GLOBAL_CONFIG_FILE="${DIRECTORY_AWS_BUILDER:-/opt/aws_builder}/config/global.config"
source "${GLOBAL_CONFIG_FILE}" || exit 3

###------------------------------------------------------------------------------------------------
## Declare dependencies
REQUIRED_SOURCE_FILES+=(
    "${LIB_FUNCTIONS_AWS_INITIALIZE}"
)

###------------------------------------------------------------------------------------------------
## Variables
VARIABLES_REQUIRED=(
    's3_bucket'
    'tag_owner'
    'tag_contact'
    'aws_region'
)
S3_BUCKET=""

TAG_ORGANIZATION="Account"
TAG_PROJECT="Account"
TAG_FUNCTION="Temporary Storage"
TAG_ENVIRONMENT="global"
TAG_OWNER=""
TAG_CONTACT=""

FILE_LOG_OPERATION=""

###------------------------------------------------------------------------------------------------
## Main
# Process Arguments
while getopts "hVb:o:c:r:P:" OPTION; do
    case $OPTION in
        b) S3_BUCKET=$OPTARG;;
        o) TAG_OWNER=$OPTARG;;
        c) TAG_CONTACT=$OPTARG;;
        r) AWS_REGION=$OPTARG;;
        P) AWS_PROFILE_TARGET=$OPTARG;;
        h) usage; exit 0;;
        V) echo "$(return_script_version "${0}")"; exit 0;;
        *) echo "ERROR: There is an error with one or more of the arguments"; usage; exit $E_BAD_ARGS;;
        ?) echo "ERROR: There is an error with one or more of the arguments"; usage; exit $E_BAD_ARGS;;
    esac
done

start_logic
log "${SELF_IDENTITY_H}: Started"

###------------------------------------------------------------------------------------------------
## START Initialize
line_break
log_highlight "Initialize"

verify_array_key_values "VARIABLES_REQUIRED[@]"
RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then exit_logic $E_BAD_ARGS; fi

generate_temp_file FILE_LOG_OPERATION "operation log file"

line_break
log "- S3 Bucket:          [${S3_BUCKET}]"
log "- Region:             [${AWS_REGION}]"
log "- Owner:              [${TAG_OWNER}]"
log "- Contact:            [${TAG_CONTACT}]"
## END Initialize
###------------------------------------------------------------------------------------------------

###------------------------------------------------------------------------------------------------
## START Execution
line_break
log_highlight "Execution"

log_notice "${SELF_IDENTITY_H}: Creating Bucket [${S3_BUCKET}]"
> ${FILE_LOG_OPERATION}
if [[ "ZZ_$(to_upper "${AWS_REGION}")" == "ZZ_US-EAST-1" ]]; then
    $(which aws) --region "${AWS_REGION}" s3api create-bucket --bucket "${S3_BUCKET}" >${FILE_LOG_OPERATION} 2>&1
    RETURNVAL="$?"
else
    $(which aws) --region "${AWS_REGION}" s3api create-bucket --bucket "${S3_BUCKET}" --create-bucket-configuration LocationConstraint=${AWS_REGION} >${FILE_LOG_OPERATION} 2>&1
    RETURNVAL="$?"
fi
$(which sed) -i "s/\o15/_AWS_BAD_IGNORE\\n/g" "${FILE_LOG_OPERATION}"
$(which sed) -i '/_AWS_BAD_IGNORE/d' "${FILE_LOG_OPERATION}"
if [ ${RETURNVAL} -ne 0 ]; then
    if grep -q "BucketAlreadyOwnedByYou" "${FILE_LOG_OPERATION}"; then
        log_warning "- Bucket already exists, skippling creation step"
    else
        log_add_from_file "${FILE_LOG_OPERATION}" "${FUNCTION_DESCRIPTION}: Data containing error" 200000
        log_error "- Failed to create S3 Bucket"
        exit_logic $E_OBJECT_FAILED_TO_CREATE
    fi
else
    log "- success"
fi
call_sleep 5 "giving AWS time to process request"

log_notice "${SELF_IDENTITY_H}: Enabling Versioning on Bucket"
> ${FILE_LOG_OPERATION}
$(which aws) --region "${AWS_REGION}" s3api put-bucket-versioning --bucket "${S3_BUCKET}" --versioning-configuration 'Status=Enabled' >${FILE_LOG_OPERATION} 2>&1
RETURNVAL="$?"
$(which sed) -i "s/\o15/_AWS_BAD_IGNORE\\n/g" "${FILE_LOG_OPERATION}"
$(which sed) -i '/_AWS_BAD_IGNORE/d' "${FILE_LOG_OPERATION}"
if [ ${RETURNVAL} -ne 0 ]; then
    log_add_from_file "${FILE_LOG_OPERATION}" "${FUNCTION_DESCRIPTION}: Data containing error" 200000
    log_error "- Failed to Enable Versioning on Bucket"
    exit_logic $E_OBJECT_FAILED_TO_CREATE
else
    log "- success"
fi
call_sleep 5 "giving AWS time to process request"

log_notice "${SELF_IDENTITY_H}: Enable Encryption on Bucket"
> ${FILE_LOG_OPERATION}
$(which aws) --region "${AWS_REGION}" s3api put-bucket-encryption --bucket "${S3_BUCKET}" --server-side-encryption-configuration '{"Rules": [{"ApplyServerSideEncryptionByDefault": {"SSEAlgorithm": "AES256"}}]}' >${FILE_LOG_OPERATION} 2>&1
RETURNVAL="$?"
$(which sed) -i "s/\o15/_AWS_BAD_IGNORE\\n/g" "${FILE_LOG_OPERATION}"
$(which sed) -i '/_AWS_BAD_IGNORE/d' "${FILE_LOG_OPERATION}"
if [ ${RETURNVAL} -ne 0 ]; then
    log_add_from_file "${FILE_LOG_OPERATION}" "${FUNCTION_DESCRIPTION}: Data containing error" 200000
    log_error "- Failed to Enable Encryption on Bucket"
    exit_logic $E_OBJECT_FAILED_TO_CREATE
else
    log "- success"
fi
call_sleep 5 "giving AWS time to process request"

log_notice "${SELF_IDENTITY_H}: Block Public Access on Bucket"
> ${FILE_LOG_OPERATION}
$(which aws) --region "${AWS_REGION}" s3api put-public-access-block --bucket "${S3_BUCKET}" --public-access-block-configuration 'BlockPublicAcls=true,IgnorePublicAcls=true,BlockPublicPolicy=true,RestrictPublicBuckets=true' >${FILE_LOG_OPERATION} 2>&1
RETURNVAL="$?"
$(which sed) -i "s/\o15/_AWS_BAD_IGNORE\\n/g" "${FILE_LOG_OPERATION}"
$(which sed) -i '/_AWS_BAD_IGNORE/d' "${FILE_LOG_OPERATION}"
if [ ${RETURNVAL} -ne 0 ]; then
    log_add_from_file "${FILE_LOG_OPERATION}" "${FUNCTION_DESCRIPTION}: Data containing error" 200000
    log_error "- Failed to Block Public Access on Bucket"
    exit_logic $E_OBJECT_FAILED_TO_CREATE
else
    log "- success"
fi
call_sleep 5 "giving AWS time to process request"

log_notice "${SELF_IDENTITY_H}: Setting Lifecycle Rules on Bucket"
> ${FILE_LOG_OPERATION}
$(which aws) --region "${AWS_REGION}" s3api put-bucket-lifecycle-configuration --bucket "${S3_BUCKET}" --lifecycle-configuration '{"Rules":[{"Expiration":{"Days":1},"ID":"temp_file_cleanup","Prefix":"","Status":"Enabled","NoncurrentVersionExpiration":{"NoncurrentDays":1},"AbortIncompleteMultipartUpload":{"DaysAfterInitiation":1}}]}' >${FILE_LOG_OPERATION} 2>&1
RETURNVAL="$?"
$(which sed) -i "s/\o15/_AWS_BAD_IGNORE\\n/g" "${FILE_LOG_OPERATION}"
$(which sed) -i '/_AWS_BAD_IGNORE/d' "${FILE_LOG_OPERATION}"
if [ ${RETURNVAL} -ne 0 ]; then
    log_add_from_file "${FILE_LOG_OPERATION}" "${FUNCTION_DESCRIPTION}: Data containing error" 200000
    log_error "- Failed to Set Lifecycle Rules on Bucket"
    exit_logic $E_OBJECT_FAILED_TO_CREATE
else
    log "- success"
fi
call_sleep 5 "giving AWS time to process request"

log_notice "${SELF_IDENTITY_H}: Setting Tags on Bucket"
> ${FILE_LOG_OPERATION}
$(which aws) --region "${AWS_REGION}" s3api put-bucket-tagging --bucket "${S3_BUCKET}" --tagging "TagSet=[{Key=Name,Value=${S3_BUCKET}},{Key=Organization,Value=${TAG_ORGANIZATION}},{Key=Project,Value=${TAG_PROJECT}},{Key=Function,Value=${TAG_FUNCTION}},{Key=Environment,Value=${TAG_ENVIRONMENT}},{Key=Region,Value=${AWS_REGION}},{Key=Owner,Value=${TAG_OWNER}},{Key=Contact,Value=${TAG_CONTACT}}]" >${FILE_LOG_OPERATION} 2>&1
RETURNVAL="$?"
$(which sed) -i "s/\o15/_AWS_BAD_IGNORE\\n/g" "${FILE_LOG_OPERATION}"
$(which sed) -i '/_AWS_BAD_IGNORE/d' "${FILE_LOG_OPERATION}"
if [ ${RETURNVAL} -ne 0 ]; then
    log_add_from_file "${FILE_LOG_OPERATION}" "${FUNCTION_DESCRIPTION}: Data containing error" 200000
    log_error "- Failed to Set Tags on Bucket"
    exit_logic $E_OBJECT_FAILED_TO_CREATE
else
    log "- success"
fi

## END Execution
###------------------------------------------------------------------------------------------------

log_success "${SELF_IDENTITY_H}: Finished"
exit_logic 0
