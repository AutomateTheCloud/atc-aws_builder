#!/bin/bash
###------------------------------------------------------------------------------------------------
# alias:   aws_builder.rds.tools.download_logs
# script:  download_logs.sh
# purpose: RDS - Retrieves RDS Logs for specified Database
# version: 1.0.0
###------------------------------------------------------------------------------------------------

###------------------------------------------------------------------------------------------------
## Config
declare -r SELF_IDENTITY="download_logs"
declare -r SELF_IDENTITY_H="RDS (Download Logs)"
declare -a ARGUMENTS_DESCRIPTION=(
    '-d <database_id> : Database ID'
    '-D <directory> : Directory to store logs'
    '-t <time_in_hours> : Pull logs for X hours (optional, defaults to all logs)'
    '-z : Compress Logs'
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
   "${LIB_FUNCTIONS_AWS_INITIALIZE}"
   "${LIB_FUNCTIONS_AWS_RDS}"
)

###------------------------------------------------------------------------------------------------
## Variables
VARIABLES_REQUIRED=(
    'database_id'
    'directory_logs'
    'aws_region'
)

DATABASE_ID="" # -d
DIRECTORY_LOGS="" # -D
TIME_HOURS=0 # -t
ENABLE_COMPRESSION=no # -z
AWS_REGION="" # -r

ARG_FILE_LAST_WRITTEN=""

###------------------------------------------------------------------------------------------------
## Main
# Process Arguments
while getopts "hVd:D:t:zr:P:" OPTION; do
    case $OPTION in
        d) DATABASE_ID=$OPTARG;;
        D) DIRECTORY_LOGS=$OPTARG;;
        t) TIME_HOURS=$OPTARG;;
        z) ENABLE_COMPRESSION=yes;;
        r) AWS_REGION=$OPTARG;;
        P) AWS_PROFILE_TARGET=$OPTARG;;
        h) usage; exit 0;;
        V) echo "$(return_script_version "${0}")"; exit 0;;
        *) echo "ERROR: There is an error with one or more of the arguments"; usage; exit $E_BAD_ARGS;;
        ?) echo "ERROR: There is an error with one or more of the arguments"; usage; exit $E_BAD_ARGS;;
    esac
done

if [ ${TIME_HOURS} -ne 0 ]; then
    ARG_FILE_LAST_WRITTEN="--file-last-written $($(which date) +%s%3N --date="-${TIME_HOURS} hour")"
fi

start_logic
log "${SELF_IDENTITY_H}: Started"

###------------------------------------------------------------------------------------------------
## START Initialize
line_break
log_highlight "Initialize"

DIRECTORY_LOGS="$(echo "${DIRECTORY_LOGS}" | sed 's:/*$::')"

verify_array_key_values "VARIABLES_REQUIRED[@]"
RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then exit_logic $E_BAD_ARGS; fi
if [ ! -d "${DIRECTORY_LOGS}" ]; then
    TMP_ERROR_MSG="Directory does not exist [${DIRECTORY_LOGS}]"
    log_error "- ${TMP_ERROR_MSG}"
    exit_logic $E_OBJECT_NOT_FOUND "${TMP_ERROR_MSG}"
fi

line_break
log "- Database ID:                [${DATABASE_ID}]"
if [ ${TIME_HOURS} -ne 0 ]; then
    log "- Time range:                 [last ${TIME_HOURS} hours]"
else
    log "- Time range:                 [all]"
fi
log "- Enable Compression:         [${ENABLE_COMPRESSION}]"
log "- Log Directory:              [${DIRECTORY_LOGS}]"
log "- AWS Region:                 [${AWS_REGION}]"
## END Initialize
###------------------------------------------------------------------------------------------------

###------------------------------------------------------------------------------------------------
## START Execution
line_break
log_highlight "Execution"

log_notice "Retrieving Log list"

ARRAY_LOG_FILES=($($(which aws) --region ${AWS_REGION} rds describe-db-log-files --db-instance-identifier "${DATABASE_ID}" --query "DescribeDBLogFiles[].LogFileName" ${ARG_FILE_LAST_WRITTEN} --output text 2>/dev/null))
for TMP_LOG in "${ARRAY_LOG_FILES[@]}"; do
    log "- [${TMP_LOG}]"
done

log_notice "Downloading Logs"
for TMP_LOG in "${ARRAY_LOG_FILES[@]}"; do
    log "- [${TMP_LOG}]"
    $(which aws) --region ${AWS_REGION} rds download-db-log-file-portion --db-instance-identifier "${DATABASE_ID}" --starting-token 0 --output text --log-file-name "${TMP_LOG}" > ${DIRECTORY_LOGS}/$($(which basename) "${TMP_LOG}") 2>/dev/null
    if option_enabled ENABLE_COMPRESSION; then
        $(which gzip) ${DIRECTORY_LOGS}/$($(which basename) "${TMP_LOG}") 2>/dev/null
    fi
    
done
## END Execution
###------------------------------------------------------------------------------------------------

log_success "${SELF_IDENTITY_H}: Finished"
exit_logic 0
