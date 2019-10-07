#!/bin/bash
###------------------------------------------------------------------------------------------------
# alias:   aws_builder.cloudformation.validate
# script:  validate.sh
# purpose: Validates of CloudFormation template
# version: 1.0.0
###------------------------------------------------------------------------------------------------

###------------------------------------------------------------------------------------------------
## Config
declare -r SELF_IDENTITY="cloudformation_validate"
declare -r SELF_IDENTITY_H="CloudFormation (Validate)"
declare -a ARGUMENTS_DESCRIPTION=(
    '-f <file> : CloudFormation Template File'
    '-s <s3_bucket_temp> : S3 Bucket to Place Temporary files'
    '-r <region> : AWS Region'
    '-P <profile> : AWS Profile (optional)'
)

###------------------------------------------------------------------------------------------------
## Load Defaults
declare -r GLOBAL_CONFIG_FILE="${DIRECTORY_AWS_BUILDER:-/opt/aws_builder}/config/global.config"
source "${GLOBAL_CONFIG_FILE}" || exit 3

###------------------------------------------------------------------------------------------------
## Declare dependencies
REQUIRED_SOURCE_FILES+=(
   "${LIB_FUNCTIONS_CORE_BUILDER}"
   "${LIB_FUNCTIONS_AWS_INITIALIZE}"
   "${LIB_FUNCTIONS_AWS_S3}"
   "${LIB_FUNCTIONS_AWS_CLOUDFORMATION}"
)

###------------------------------------------------------------------------------------------------
## Variables
VARIABLES_REQUIRED=(
    'file_template'
    's3_bucket_temp'
    'aws_region'
)

###------------------------------------------------------------------------------------------------
## Main
# Process Arguments
while getopts "hVf:s:r:P:" OPTION; do
    case $OPTION in
        f) FILE_TEMPLATE=$OPTARG;;
        s) S3_BUCKET_TEMP=$OPTARG;;
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

if [ ! -f "${FILE_TEMPLATE}" ]; then
    TMP_ERROR_MSG="CloudFormation Template file does not exist [${FILE_TEMPLATE}]"
    log_error "- ${TMP_ERROR_MSG}"
    exit_logic $E_OBJECT_NOT_FOUND "${TMP_ERROR_MSG}"
fi

line_break
log "- CloudFormation Template File: [${FILE_TEMPLATE}]"
log "- S3 Bucket (temp):             [${S3_BUCKET_TEMP}]"
log "- AWS Region:                   [${AWS_REGION}]"

## END Initialize
###------------------------------------------------------------------------------------------------

###------------------------------------------------------------------------------------------------
## START Execution
line_break
log_highlight "Execution"

cloudformation_validate_template "${FILE_TEMPLATE}" "${S3_BUCKET_TEMP}" "${AWS_REGION}"
RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then exit_logic $E_BAD_ARGS; fi
## END Execution
###------------------------------------------------------------------------------------------------

log_success "${SELF_IDENTITY_H}: Finished"
exit_logic 0
