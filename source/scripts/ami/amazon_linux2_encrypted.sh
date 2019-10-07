#!/bin/bash
###------------------------------------------------------------------------------------------------
# alias:   aws_builder.ami.amazon_linux2_encrypted
# script:  amazon_linux2_encrypted.sh
# purpose: AMI - Amazon Linux 2 (encrypted)
# version: 1.0.0
###------------------------------------------------------------------------------------------------

###------------------------------------------------------------------------------------------------
## Config
declare -r SELF_IDENTITY="amazon_linux2_encrypted"
declare -r SELF_IDENTITY_H="AMI - Amazon Linux 2 (encrypted)"
declare -a ARGUMENTS_DESCRIPTION=(
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
    "${LIB_FUNCTIONS_AWS_AMI}"
)

###------------------------------------------------------------------------------------------------
## Variables
VARIABLES_REQUIRED=(
    'aws_region'
)

AMI_BASE_ID=""
AMI_BASE_NAME=""
AMI_BASE_DESCRIPTION=""
AMI_ENCRYPTED_ID=""
AMI_ENCRYPTED_NAME=""
AMI_ENCRYPTED_DESCRIPTION=""

AMI_STATE=""
AVAILABILITY_RETRY_COUNT=60
DELAY_BETWEEN_RETRY=30
RUN=1
COUNTER=0
RETRY_AVAILABILITY_CHECK=no

###------------------------------------------------------------------------------------------------
## Main
# Process Arguments
while getopts "hVr:P:" OPTION; do
    case $OPTION in
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
log "- Region:                      [${AWS_REGION}]"

log_notice "${SELF_IDENTITY_H}: Retrieving AMI Metadata"

get_latest_ami_amazon_linux2_hvm AMI_BASE_ID "${AWS_REGION}"
RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then exit_logic $RETURNVAL; fi

ami_name AMI_BASE_NAME "${AMI_BASE_ID}" "${AWS_REGION}"
RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then exit_logic $RETURNVAL; fi

ami_description AMI_BASE_DESCRIPTION "${AMI_BASE_ID}" "${AWS_REGION}"
RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then exit_logic $RETURNVAL; fi

line_break
log "- AMI ID (Base):               [${AMI_BASE_ID}]"
log "- AMI Name (Base):             [${AMI_BASE_NAME}]"
log "- AMI Description (Base):      [${AMI_BASE_DESCRIPTION}]"

AMI_ENCRYPTED_NAME="enc-${AMI_BASE_NAME}"
AMI_ENCRYPTED_DESCRIPTION="${AMI_BASE_DESCRIPTION} (Encrypted)"

line_break
log "- AMI Name (Encrypted):        [${AMI_ENCRYPTED_NAME}]"
log "- AMI Description (Encrypted): [${AMI_ENCRYPTED_DESCRIPTION}]"
## END Initialize
###------------------------------------------------------------------------------------------------

###------------------------------------------------------------------------------------------------
## START Execution
line_break
log_highlight "Execution"

if(ami_name_exists "${AMI_ENCRYPTED_NAME}" "${AWS_REGION}"); then
    log_notice "- AMI Exists [${AMI_ENCRYPTED_NAME}], no need to run"
    exit_logic 0
fi

log_notice "${SELF_IDENTITY_H}: Encypted AMI does not exist, starting AMI creation"
ami_copy AMI_ENCRYPTED_ID "${AMI_BASE_ID}" "${AMI_ENCRYPTED_NAME}" "${AMI_ENCRYPTED_DESCRIPTION}" "${AWS_REGION}" "${AWS_REGION}" yes
RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then exit_logic $RETURNVAL; fi

log_notice "- Pausing for 60 seconds to allow AMI Copy process some time"
call_sleep 60

log_notice "${SELF_IDENTITY_H}: Attempting to determine if AMI is in an AVAILABLE state"
while [ ${RUN} == 1 ]; do
    COUNTER=$((${COUNTER}+1))
    if [ ${COUNTER} -gt ${AVAILABILITY_RETRY_COUNT} ]; then
        TMP_ERROR_MSG="${SELF_IDENTITY_H}: retry count [${AVAILABILITY_RETRY_COUNT}] exceeded, AMI failed to enter AVAILABLE state"
        log_error "${SELF_IDENTITY_H}: ${TMP_ERROR_MSG}"
        exit_logic $E_AWS_RESOURCE_NOT_READY "${TMP_ERROR_MSG}"
    fi
    if option_enabled RETRY_AVAILABILITY_CHECK; then
        call_sleep ${DELAY_BETWEEN_RETRY}
    fi
    log "- AMI Availability check [ami_id::${AMI_ENCRYPTED_ID}] [attempt::${COUNTER}/${AVAILABILITY_RETRY_COUNT}]"
    RETRY_AVAILABILITY_CHECK=no

    ami_state AMI_STATE "${AMI_ENCRYPTED_ID}" "${AWS_REGION}"
    if [[ "ZZ_$(to_upper "${AMI_STATE}")" == "ZZ_AVAILABLE" ]]; then
        log "  - AMI is [AVAILABLE]"
        RUN=0
    else
        RETRY_AVAILABILITY_CHECK=yes
        log "  - AMI is [NOT AVAILABLE]"
    fi
done
## END Execution
###------------------------------------------------------------------------------------------------

log_success "${SELF_IDENTITY_H}: Finished"
exit_logic 0
