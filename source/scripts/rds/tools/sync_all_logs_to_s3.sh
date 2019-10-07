#!/bin/bash
###------------------------------------------------------------------------------------------------
# alias:   aws_builder.rds.tools.sync_all_logs_to_s3
# script:  sync_all_logs_to_s3.sh
# purpose: RDS - Syncs all Logs from all Databases to S3
# version: 1.0.0
###------------------------------------------------------------------------------------------------

###------------------------------------------------------------------------------------------------
## Config
declare -r SELF_IDENTITY="sync_all_logs_to_s3"
declare -r SELF_IDENTITY_H="RDS (Sync All Logs)"
declare -a ARGUMENTS_DESCRIPTION=(
    '-s <s3_bucket> : S3 Bucket'
    '-p <s3_prefix> : S3 Prefix'
    '-t <time_in_hours> : Pull logs for X hours (optional, defaults to all logs)'
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
   "${LIB_FUNCTIONS_AWS_S3}"
)

###------------------------------------------------------------------------------------------------
## Variables
VARIABLES_REQUIRED=(
    's3_bucket'
    's3_prefix'
    'aws_region'
)

S3_BUCKET="" # -s
S3_PREFIX="" # -p
TIME_HOURS=0 # -t
AWS_REGION="" # -r
DATE_YEAR="$($(which date) +%Y)"
DATE_MONTH="$($(which date) +%m)"
DATE_DAY="$($(which date) +%d)"

TMP_DIRECTORY_LOGS=""
TMP_DIRECTORY_LOGS_DATABASE=""


###------------------------------------------------------------------------------------------------
## Main
# Process Arguments
while getopts "hVs:p:t:r:P:" OPTION; do
    case $OPTION in
        s) S3_BUCKET=$OPTARG;;
        p) S3_PREFIX=$OPTARG;;
        t) TIME_HOURS=$OPTARG;;
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

S3_PREFIX="$(echo "${S3_PREFIX}" | sed 's:/*$::')"

verify_array_key_values "VARIABLES_REQUIRED[@]"
RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then exit_logic $E_BAD_ARGS; fi

line_break
log "- S3 Bucket:                  [${S3_BUCKET}]"
log "- S3 Prefix:                  [${S3_PREFIX}]"
if [ ${TIME_HOURS} -ne 0 ]; then
    log "- Time range:                 [last ${TIME_HOURS} hours]"
else
    log "- Time range:                 [all]"
fi
log "- AWS Region:                 [${AWS_REGION}]"

generate_temp_directory TMP_DIRECTORY_LOGS "Log Directory"
## END Initialize
###------------------------------------------------------------------------------------------------

###------------------------------------------------------------------------------------------------
## START Execution
line_break
log_highlight "Execution"

log_notice "Retrieving Database list"
ARRAY_DATABASES=($($(which aws) --region ${AWS_REGION} rds describe-db-instances --query "DBInstances[].DBInstanceIdentifier" --output text 2>/dev/null))
for TMP_DATABASE in "${ARRAY_DATABASES[@]}"; do
    log "- [${TMP_DATABASE}]"
done

log_notice "Processing Databases"
for TMP_DATABASE in "${ARRAY_DATABASES[@]}"; do
    if(! is_empty "${TMP_DATABASE}"); then
        log "- [${TMP_DATABASE}]"
        TMP_DIRECTORY_LOGS_DATABASE="${TMP_DIRECTORY_LOGS}/${TMP_DATABASE}"
        $(which mkdir) -p ${TMP_DIRECTORY_LOGS_DATABASE}
        exec_script "${SCRIPTS_RDS_TOOLS_DOWNLOAD_LOGS}" -r "${AWS_REGION}" -d "${TMP_DATABASE}" -D "${TMP_DIRECTORY_LOGS_DATABASE}" -t "${TIME_HOURS}" -z
        RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then exit_logic ${RETURNVAL};fi
        
        s3_sync_upload "${S3_BUCKET}" "${S3_PREFIX}/${TMP_DATABASE}/${DATE_YEAR}/${DATE_MONTH}/${DATE_DAY}" "${TMP_DIRECTORY_LOGS_DATABASE}" no no yes yes
        RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then exit_logic ${RETURNVAL};fi
        
        rm -rf ${TMP_DIRECTORY_LOGS_DATABASE}  >/dev/null 2>&1
    fi
done
exit_logic 0
## END Execution
###------------------------------------------------------------------------------------------------

log_success "${SELF_IDENTITY_H}: Finished"
exit_logic 0
