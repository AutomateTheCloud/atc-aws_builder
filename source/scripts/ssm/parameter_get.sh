#!/bin/bash
###------------------------------------------------------------------------------------------------
# alias:   aws_builder.ssm.parameter_get
# script:  parameter_get.sh
# purpose: Parameter (Get)
# version: 1.0.0
###------------------------------------------------------------------------------------------------

###------------------------------------------------------------------------------------------------
## Config
declare -r SELF_IDENTITY="parameter_get"
declare -r SELF_IDENTITY_H="Parameter (Get)"
declare -a ARGUMENTS_DESCRIPTION=(
    '-p <parameter> : Parameter'
    '-r <region> : Region'
    '-d : Disable decryption'
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
    "${LIB_FUNCTIONS_AWS_SSM}"
)

###------------------------------------------------------------------------------------------------
## Script specific settings (non-configurable)
# Variables
VARIABLES_REQUIRED=(
    'parameter_name'
    'aws_region'
)
USE_ENCRYPTION=yes
PARAMETER_OUTPUT=""

###------------------------------------------------------------------------------------------------
## Main
# Process Arguments
while getopts "hVp:r:dP:" OPTION; do
    case $OPTION in
        p) PARAMETER_NAME=$OPTARG;;
        r) AWS_REGION=$OPTARG;;
        d) USE_ENCRYPTION=no;;
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
log "- Parameter:        [${PARAMETER_NAME}]"
log "- AWS Region:       [${AWS_REGION}]"
log "- Encryption:       [${USE_ENCRYPTION}]"
## END Initialize
###------------------------------------------------------------------------------------------------

###------------------------------------------------------------------------------------------------
## START Execution
line_break
log_highlight "Execution"

parameter_get PARAMETER_OUTPUT "${PARAMETER_NAME}" "${USE_ENCRYPTION}" "${AWS_REGION}"
RETURNVAL="$?"
if [ ${RETURNVAL} -ne 0 ]; then
    exit_logic $RETURNVAL
fi
log "- Output:           [$(color_text "${COLOR_YELLOW}" "${PARAMETER_OUTPUT}")]"
## END Execution
###------------------------------------------------------------------------------------------------

exit_logic 0
