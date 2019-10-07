#!/bin/bash
###------------------------------------------------------------------------------------------------
# alias:   aws_builder.ssm.parameter_delete
# script:  parameter_delete.sh
# purpose: Parameter (Delete)
# version: 1.0.0
###------------------------------------------------------------------------------------------------

###------------------------------------------------------------------------------------------------
## Config
declare -r SELF_IDENTITY="parameter_delete"
declare -r SELF_IDENTITY_H="Parameter (Delete)"
declare -a ARGUMENTS_DESCRIPTION=(
    '-p <parameter> : Parameter'
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
    "${LIB_FUNCTIONS_AWS_SSM}"
)

###------------------------------------------------------------------------------------------------
## Script specific settings (non-configurable)
# Variables
VARIABLES_REQUIRED=(
    'parameter_name'
    'aws_region'
)

###------------------------------------------------------------------------------------------------
## Main
# Process Arguments
while getopts "hVp:r:P:" OPTION; do
    case $OPTION in
        p) PARAMETER_NAME=$OPTARG;;
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
log "- Parameter:        [${PARAMETER_NAME}]"
log "- AWS Region:       [${AWS_REGION}]"
## END Initialize
###------------------------------------------------------------------------------------------------

###------------------------------------------------------------------------------------------------
## START Execution
line_break
log_highlight "Execution"

parameter_delete "${PARAMETER_NAME}" no "${AWS_REGION}"
RETURNVAL="$?"
## END Execution
###------------------------------------------------------------------------------------------------

exit_logic $RETURNVAL
