#!/bin/bash
###------------------------------------------------------------------------------------------------
# alias:   aws_builder.ssm.parameter_put_file
# script:  parameter_put_file.sh
# purpose: Parameter (Put File)
# version: 1.0.0
###------------------------------------------------------------------------------------------------

###------------------------------------------------------------------------------------------------
## Config
declare -r SELF_IDENTITY="parameter_put_file"
declare -r SELF_IDENTITY_H="Parameter (Put File)"
declare -a ARGUMENTS_DESCRIPTION=(
    '-p <parameter> : Parameter'
    '-f <filename> : File Target'
    '-k <key> : Encryption key to use <optional, defaults to AWS Account Default key>'
    '-d <description> : Description <optional>'
    '-D : Disable Parameter replacement'
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
	'file_target'
    'aws_region'
)

ENCRYPTION_KEY="" # -k
DESCRIPTION="" # -d
REPLACE_PARAMETER=yes
USE_ENCRYPTION=yes

###------------------------------------------------------------------------------------------------
## Main
# Process Arguments
###------------------------------------------------------------------------------------------------
## Main
# Process Arguments
while getopts "hVp:f:k:d:Dr:P:" OPTION; do
    case $OPTION in
        p) PARAMETER_NAME=$OPTARG;;
        f) FILE_TARGET=$OPTARG;;
        k) ENCRYPTION_KEY=$OPTARG;;
        d) DESCRIPTION=$OPTARG;;
        D) REPLACE_PARAMETER=no;;
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

if [ ! -f "${FILE_TARGET}" ]; then
    TMP_ERROR_MSG="File target not does not exist [${FILE_TARGET}]"
    log_error "- ${TMP_ERROR_MSG}"
    exit_logic $E_OBJECT_NOT_FOUND "${TMP_ERROR_MSG}"
fi
## END Initialize
###------------------------------------------------------------------------------------------------

###------------------------------------------------------------------------------------------------
## START Execution
line_break
log_highlight "Execution"

parameter_put_file "${PARAMETER_NAME}" "${FILE_TARGET}" "${ENCRYPTION_KEY}" "${DESCRIPTION}" "${REPLACE_PARAMETER}" "${AWS_REGION}"
RETURNVAL="$?"
## END Execution
###------------------------------------------------------------------------------------------------

exit_logic $RETURNVAL
