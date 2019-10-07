#!/bin/bash
###------------------------------------------------------------------------------------------------
# alias:   aws_builder.cloudfront.invalidate_cache
# script:  invalidate_cache.sh
# purpose: CloudFront (Invalidate Cache)
# version: 1.0.0
###------------------------------------------------------------------------------------------------

###------------------------------------------------------------------------------------------------
## Config
declare -r SELF_IDENTITY="invalidate_cache"
declare -r SELF_IDENTITY_H="CloudFront (Invalidate Cache)"
declare -a ARGUMENTS_DESCRIPTION=(
    '-s <stack_name> : Cloudformation Stack Name for CloudFront Deployment'
    '-p <invalidation_path> : Path to Invalidate'
    '-r <aws_region> : AWS Region'
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
    "${LIB_FUNCTIONS_AWS_CLOUDFORMATION}"
    "${LIB_FUNCTIONS_AWS_CLOUDFRONT}"
)

###------------------------------------------------------------------------------------------------
## Script specific settings (non-configurable)
# Variables
VARIABLES_REQUIRED=(
    'aws_stack_name'
    'cloudfront_invalidation_path'
    'aws_region'
)
STACK_VARIABLES_REQUIRED=(
    'DistributionId'
)

FILE_CLOUDFORMATION_OUTPUT=""

###------------------------------------------------------------------------------------------------
## Main
# Process Arguments
while getopts "hVs:p:r:P:" OPTION; do
    case $OPTION in
        s) AWS_STACK_NAME=$OPTARG;;
        p) CLOUDFRONT_INVALIDATION_PATH=$OPTARG;;
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


line_break
log "- CloudFormation Stack:         [${AWS_STACK_NAME}]"
log "- CloudFront Invalidation Path: [${CLOUDFRONT_INVALIDATION_PATH}]"
log "- AWS Region:                   [${AWS_REGION}]"
line_break

generate_temp_file FILE_CLOUDFORMATION_OUTPUT "CloudFormation Stack Output"

cloudformation_get_outputs_silent "${FILE_CLOUDFORMATION_OUTPUT}" "${AWS_STACK_NAME}" "${AWS_REGION}"
RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then exit_logic ${RETURNVAL}; fi

load_array_properties_from_file "STACK_VARIABLES_REQUIRED[@]" "${FILE_CLOUDFORMATION_OUTPUT}" "STACK"
verify_array_key_values "STACK_VARIABLES_REQUIRED[@]" "STACK"
RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then exit_logic $RETURNVAL; fi

log "- CloudFront Distribution ID:   [${STACK_DISTRIBUTIONID}]"
## END Initialize
###------------------------------------------------------------------------------------------------

###------------------------------------------------------------------------------------------------
## START Execution
line_break
log_highlight "Execution"

cloudfront_invalidate_cache "${STACK_DISTRIBUTIONID}" "${CLOUDFRONT_INVALIDATION_PATH}" "${AWS_REGION}"
RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then exit_logic $RETURNVAL; fi
## END Execution
###------------------------------------------------------------------------------------------------

log_success "${SELF_IDENTITY_H}: Finished"
exit_logic 0
