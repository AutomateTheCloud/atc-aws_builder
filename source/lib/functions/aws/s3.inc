###------------------------------------------------------------------------------------------------
# script:  s3.inc
# purpose: Collection of functions related to AWS (S3)
# version: 1.0.0
#
# function list:
#          - s3_cp_download
#          - s3_cp_upload
#          - s3_delete
#          - s3_move_in_bucket
#          - s3_sync_download
#          - s3_sync_upload
#          - s3_verify
###------------------------------------------------------------------------------------------------
REQUIRED_EXECUTABLES+=('aws')

###------------------------------------------------------------------------------------------------
# Variables
AWS_S3_DEFAULT_RETRY_COUNT=3
AWS_S3_DEFAULT_RETRY_TIMER_MAX_SEC=30

###------------------------------------------------------------------------------------------------
## FUNCTION: s3_cp_download()
## - Downloads files from AWS S3 Bucket (via copy method)
## - Arguments
##   - $1: S3 Bucket
##   - $2: S3 Target
##   - $3: Local File
function s3_cp_download() {
    local FUNCTION_DESCRIPTION="AWS S3 Copy (Download)"
    local AWS_S3_BUCKET="${1}"
    local AWS_S3_TARGET="${2}"
    local AWS_CP_LOCAL_FILE="${3}"

    local RETURNVAL=""
    local TMP_STRING=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    local AWS_S3_URL=""

    if(is_empty "${AWS_S3_BUCKET}"); then
        log_error "${FUNCTION_DESCRIPTION}: S3 Bucket not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${AWS_CP_LOCAL_FILE}"); then
        log_error "${FUNCTION_DESCRIPTION}: Local File not specified"
        return $E_BAD_ARGS
    fi

    if(is_empty "${AWS_S3_TARGET}"); then
        AWS_S3_URL="s3://${AWS_S3_BUCKET}/"
    else
        AWS_S3_URL="s3://${AWS_S3_BUCKET}/${AWS_S3_TARGET}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    log "${FUNCTION_DESCRIPTION}: started"
    log "- Source:             [${AWS_S3_URL}]"
    log "- Destination:        [${AWS_CP_LOCAL_FILE}]"
    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_S3_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_S3_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_S3_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        log "${FUNCTION_DESCRIPTION}: Uploading file (Attempt::${COUNTER} of ${AWS_S3_DEFAULT_RETRY_COUNT})"
        $(which aws) s3 cp "${AWS_S3_URL}" "${AWS_CP_LOCAL_FILE}" >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
        RETURNVAL="$?"
        # Fix Response File
        $(which sed) -i "s/\o15/_AWS_BAD_IGNORE\\n/g" "${AWS_FILE_RESPONSE}"
        $(which sed) -i '/_AWS_BAD_IGNORE/d' "${AWS_FILE_RESPONSE}"
        if [ ${RETURNVAL} -ne 0 ]; then
            log_add_from_file "${AWS_FILE_ERROR}" "${FUNCTION_DESCRIPTION}: Data containing error"
            log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Data"
            log_error "${FUNCTION_DESCRIPTION}: operation failed (aws_cli_exit_code::${RETURNVAL}])"
            RETRY_ENABLED=yes
            # Reset Files
            > ${AWS_FILE_RESPONSE}
            > ${AWS_FILE_ERROR}
        else
            log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Data"
            log "${FUNCTION_DESCRIPTION}: successfully uploaded file to S3"
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: s3_cp_upload()
## - Uploads files to AWS S3 Bucket (via copy method)
## - Arguments
##   - $1: S3 Bucket
##   - $2: S3 Target
##   - $3: Local File
##   - $4: Enable Server-side encryption (optional, defaults to yes)
##   - $5: Enable Reduced Redundancy (optional, defaults to yes)
function s3_cp_upload() {
    local FUNCTION_DESCRIPTION="AWS S3 Copy (Upload)"
    local AWS_S3_BUCKET="${1}"
    local AWS_S3_TARGET="${2}"
    local AWS_CP_LOCAL_FILE="${3}"
    local AWS_S3_FLAG_SERVERSIDENECRYPTION_OPTION="${4}"
    local AWS_S3_FLAG_REDUCEDREDUNDANCY_OPTION="${5}"

    local RETURNVAL=""
    local TMP_STRING=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    local AWS_S3_URL=""
    local AWS_S3_FLAG_SERVERSIDENECRYPTION_STRING=""
    local AWS_S3_FLAG_REDUCEDREDUNDANCY_STRING=""

    if(is_empty "${AWS_S3_BUCKET}"); then
        log_error "${FUNCTION_DESCRIPTION}: S3 Bucket not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${AWS_CP_LOCAL_FILE}"); then
        log_error "${FUNCTION_DESCRIPTION}: Local File not specified"
        return $E_BAD_ARGS
    fi
    if [ ! -f "${AWS_CP_LOCAL_FILE}" ]; then
        log_error "${FUNCTION_DESCRIPTION}: Local File does not exist [${AWS_CP_LOCAL_FILE}]"
        return $E_OBJECT_NOT_FOUND
    fi

    if(is_empty "${AWS_S3_TARGET}"); then
        AWS_S3_URL="s3://${AWS_S3_BUCKET}/"
    else
        AWS_S3_URL="s3://${AWS_S3_BUCKET}/${AWS_S3_TARGET}"
    fi
    if(is_empty "${AWS_S3_FLAG_SERVERSIDENECRYPTION_OPTION}"); then
        AWS_S3_FLAG_SERVERSIDENECRYPTION_OPTION=yes
    fi
    if(is_empty "${AWS_S3_FLAG_REDUCEDREDUNDANCY_OPTION}"); then
        AWS_S3_FLAG_REDUCEDREDUNDANCY_OPTION=yes
    fi
    if option_enabled AWS_S3_FLAG_SERVERSIDENECRYPTION_OPTION; then
        AWS_S3_FLAG_SERVERSIDENECRYPTION_STRING="--sse"
    fi
    if option_enabled AWS_S3_FLAG_REDUCEDREDUNDANCY_OPTION; then
        AWS_S3_FLAG_REDUCEDREDUNDANCY_STRING="--storage-class REDUCED_REDUNDANCY"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    log "${FUNCTION_DESCRIPTION}: started"
    log "- Source:             [${AWS_CP_LOCAL_FILE}]"
    log "- Destination:        [${AWS_S3_URL}]"
    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_S3_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_S3_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_S3_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        log "${FUNCTION_DESCRIPTION}: Uploading file (Attempt::${COUNTER} of ${AWS_S3_DEFAULT_RETRY_COUNT})"
        $(which aws) s3 cp "${AWS_CP_LOCAL_FILE}" "${AWS_S3_URL}" ${AWS_S3_FLAG_SERVERSIDENECRYPTION_STRING} ${AWS_S3_FLAG_REDUCEDREDUNDANCY_STRING} >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
        RETURNVAL="$?"
        # Fix Response File
        $(which sed) -i "s/\o15/_AWS_BAD_IGNORE\\n/g" "${AWS_FILE_RESPONSE}"
        $(which sed) -i '/_AWS_BAD_IGNORE/d' "${AWS_FILE_RESPONSE}"
        if [ ${RETURNVAL} -ne 0 ]; then
            log_add_from_file "${AWS_FILE_ERROR}" "${FUNCTION_DESCRIPTION}: Data containing error"
            log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Data"
            log_error "${FUNCTION_DESCRIPTION}: operation failed (aws_cli_exit_code::${RETURNVAL}])"
            RETRY_ENABLED=yes
            # Reset Files
            > ${AWS_FILE_RESPONSE}
            > ${AWS_FILE_ERROR}
        else
            log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Data"
            log "${FUNCTION_DESCRIPTION}: successfully uploaded file to S3"
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: s3_delete()
## - Downloads files from AWS S3 Bucket (via copy method)
## - Arguments
##   - $1: S3 Bucket
##   - $2: S3 Target
function s3_delete() {
    local FUNCTION_DESCRIPTION="AWS S3 Delete"
    local AWS_S3_BUCKET="${1}"
    local AWS_S3_TARGET="${2}"

    local RETURNVAL=""
    local TMP_STRING=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    local AWS_S3_URL=""

    if(is_empty "${AWS_S3_BUCKET}"); then
        log_error "${FUNCTION_DESCRIPTION}: S3 Bucket not specified"
        return $E_BAD_ARGS
    fi

    if(is_empty "${AWS_S3_TARGET}"); then
        AWS_S3_URL="s3://${AWS_S3_BUCKET}/"
    else
        AWS_S3_URL="s3://${AWS_S3_BUCKET}/${AWS_S3_TARGET}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    log "${FUNCTION_DESCRIPTION}: started"
    log "- Target:             [${AWS_S3_URL}]"
    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_S3_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_S3_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_S3_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        log "${FUNCTION_DESCRIPTION}: Deleting file (target: [${AWS_S3_URL}]) (Attempt::${COUNTER} of ${AWS_S3_DEFAULT_RETRY_COUNT})"
        $(which aws) s3 rm "${AWS_S3_URL}" >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
        RETURNVAL="$?"
        # Fix Response File
        $(which sed) -i "s/\o15/_AWS_BAD_IGNORE\\n/g" "${AWS_FILE_RESPONSE}"
        $(which sed) -i '/_AWS_BAD_IGNORE/d' "${AWS_FILE_RESPONSE}"
        if [ ${RETURNVAL} -ne 0 ]; then
            log_add_from_file "${AWS_FILE_ERROR}" "${FUNCTION_DESCRIPTION}: Data containing error"
            log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Data"
            log_error "${FUNCTION_DESCRIPTION}: operation failed (aws_cli_exit_code::${RETURNVAL}])"
            RETRY_ENABLED=yes
            # Reset Files
            > ${AWS_FILE_RESPONSE}
            > ${AWS_FILE_ERROR}
        else
            log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Data"
            log "${FUNCTION_DESCRIPTION}: successfully deleted file from S3"
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: s3_move_in_bucket()
## - Moves files inside AWS S3 Bucket
## - Arguments
##   - $1: S3 Bucket
##   - $2: S3 File Source
##   - $3: S3 File Target
##   - $4: Enable Server-side encryption (optional, defaults to yes)
##   - $5: Enable Reduced Redundancy (optional, defaults to yes)
function s3_move_in_bucket() {
    local FUNCTION_DESCRIPTION="AWS S3 Move"
    local AWS_S3_BUCKET="${1}"
    local AWS_S3_FILE_SOURCE="${2}"
    local AWS_S3_FILE_TARGET="${3}"
    local AWS_S3_FLAG_SERVERSIDENECRYPTION_OPTION="${4}"
    local AWS_S3_FLAG_REDUCEDREDUNDANCY_OPTION="${5}"

    local RETURNVAL=""
    local TMP_STRING=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    local AWS_S3_URL_SOURCE=""
    local AWS_S3_URL_TARGET=""
    local AWS_S3_FLAG_SERVERSIDENECRYPTION_STRING=""
    local AWS_S3_FLAG_REDUCEDREDUNDANCY_STRING=""

    if(is_empty "${AWS_S3_BUCKET}"); then
        log_error "${FUNCTION_DESCRIPTION}: S3 Bucket not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${AWS_S3_FILE_SOURCE}"); then
        log_error "${FUNCTION_DESCRIPTION}: Source File not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${AWS_S3_FILE_TARGET}"); then
        log_error "${FUNCTION_DESCRIPTION}: Target File not specified"
        return $E_BAD_ARGS
    fi

    AWS_S3_URL_SOURCE="s3://${AWS_S3_BUCKET}/${AWS_S3_FILE_SOURCE}"
    AWS_S3_URL_TARGET="s3://${AWS_S3_BUCKET}/${AWS_S3_FILE_TARGET}"

    if(is_empty "${AWS_S3_FLAG_SERVERSIDENECRYPTION_OPTION}"); then
        AWS_S3_FLAG_SERVERSIDENECRYPTION_OPTION=yes
    fi
    if(is_empty "${AWS_S3_FLAG_REDUCEDREDUNDANCY_OPTION}"); then
        AWS_S3_FLAG_REDUCEDREDUNDANCY_OPTION=yes
    fi
    if option_enabled AWS_S3_FLAG_SERVERSIDENECRYPTION_OPTION; then
        AWS_S3_FLAG_SERVERSIDENECRYPTION_STRING="--sse"
    fi
    if option_enabled AWS_S3_FLAG_REDUCEDREDUNDANCY_OPTION; then
        AWS_S3_FLAG_REDUCEDREDUNDANCY_STRING="--storage-class REDUCED_REDUNDANCY"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    log "${FUNCTION_DESCRIPTION}: started"
    log "- Source:             [${AWS_S3_URL_SOURCE}]"
    log "- Target:             [${AWS_S3_URL_TARGET}]"
    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_S3_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_S3_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_S3_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        log "${FUNCTION_DESCRIPTION}: Moving file (Attempt::${COUNTER} of ${AWS_S3_DEFAULT_RETRY_COUNT})"
        $(which aws) s3 mv "${AWS_S3_URL_SOURCE}" "${AWS_S3_URL_TARGET}" ${AWS_S3_FLAG_SERVERSIDENECRYPTION_STRING} ${AWS_S3_FLAG_REDUCEDREDUNDANCY_STRING} >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
        RETURNVAL="$?"
        # Fix Response File
        $(which sed) -i "s/\o15/_AWS_BAD_IGNORE\\n/g" "${AWS_FILE_RESPONSE}"
        $(which sed) -i '/_AWS_BAD_IGNORE/d' "${AWS_FILE_RESPONSE}"
        if [ ${RETURNVAL} -ne 0 ]; then
            log_add_from_file "${AWS_FILE_ERROR}" "${FUNCTION_DESCRIPTION}: Data containing error"
            log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Data"
            log_error "${FUNCTION_DESCRIPTION}: operation failed (aws_cli_exit_code::${RETURNVAL}])"
            RETRY_ENABLED=yes
            # Reset Files
            > ${AWS_FILE_RESPONSE}
            > ${AWS_FILE_ERROR}
        else
            log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Data"
            log "${FUNCTION_DESCRIPTION}: successfully moved file on S3"
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: s3_sync_download()
## - Downloads files from AWS S3 Bucket (via sync method)
## - Arguments
##   - $1: S3 Bucket
##   - $2: S3 Directory
##   - $3: Local Directory
##   - $4: Delete (yes/no) - Files that exist in the destination but not in the source are deleted during sync (optional, defaults to no)
##   - $5: Exact Timestamps (yes/no) - When syncing from S3 to local,  same-sized items  will  be  ignored  only  when  the timestamps match
#         exactly. The default behavior is to ignore same-sized items unless the local version is newer than the S3 version.
#        (optional, defaults to no)
##   - $6: Excludes (optional, semicolon separated)
##   - $7: Include (optional, semicolon separated)
function s3_sync_download() {
    local FUNCTION_DESCRIPTION="AWS S3 Sync (Download)"
    local AWS_S3_BUCKET="${1}"
    local AWS_S3_DIRECTORY="${2}"
    local AWS_SYNC_LOCAL_DIRECTORY="${3%/}/"
    local AWS_S3_FLAG_DELETE_OPTION="${4}"
    local AWS_S3_FLAG_EXACTTIMESTAMPS_OPTION="${5}"
    local EXCLUDES_LIST="${6}"
    local INCLUDES_LIST="${7}"

    local RETURNVAL=""
    local TMP_STRING=""
    local TMP_ARRAY=()
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    local AWS_S3_URL=""
    local AWS_S3_FLAG_DELETE_STRING=""
    local AWS_S3_FLAG_EXACTTIMESTAMPS_STRING=""
    local AWS_S3_EXCLUDES=""
    local AWS_S3_INCLUDES=""

    if(is_empty "${AWS_S3_BUCKET}"); then
        log_error "${FUNCTION_DESCRIPTION}: S3 Bucket not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${AWS_SYNC_LOCAL_DIRECTORY}"); then
        log_error "${FUNCTION_DESCRIPTION}: Local Directory not specified"
        return $E_BAD_ARGS
    fi
    if [ ! -d "${AWS_SYNC_LOCAL_DIRECTORY}" ]; then
        log_error "${FUNCTION_DESCRIPTION}: Local Directory does not exist [${AWS_SYNC_LOCAL_DIRECTORY}]"
        return $E_OBJECT_NOT_FOUND
    fi

    if(is_empty "${AWS_S3_DIRECTORY}"); then
        AWS_S3_URL="s3://${AWS_S3_BUCKET}/"
    else
        AWS_S3_URL="s3://${AWS_S3_BUCKET}/${AWS_S3_DIRECTORY}/"
    fi
    if(is_empty "${AWS_S3_FLAG_DELETE_OPTION}"); then
        AWS_S3_FLAG_DELETE_OPTION=no
    fi
    if(is_empty "${AWS_S3_FLAG_EXACTTIMESTAMPS_OPTION}"); then
        AWS_S3_FLAG_EXACTTIMESTAMPS_OPTION=no
    fi
    if option_enabled AWS_S3_FLAG_DELETE_OPTION; then
        AWS_S3_FLAG_DELETE_STRING="--delete"
    fi
    if option_enabled AWS_S3_FLAG_EXACTTIMESTAMPS_OPTION; then
        AWS_S3_FLAG_EXACTTIMESTAMPS_STRING="--exact-timestamps"
    fi
    if (! is_empty "${EXCLUDES_LIST}"); then
        TMP_ARRAY=(${EXCLUDES_LIST//;/ })
        for TMP_STRING in "${TMP_ARRAY[@]}"; do
            AWS_S3_EXCLUDES="${AWS_S3_EXCLUDES} --exclude ${TMP_STRING} "
        done
    fi
    if (! is_empty "${INCLUDES_LIST}"); then
        TMP_ARRAY=(${INCLUDES_LIST//;/ })
        for TMP_STRING in "${TMP_ARRAY[@]}"; do
            AWS_S3_INCLUDES="${AWS_S3_INCLUDES} --exclude ${TMP_STRING} "
        done
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    log "${FUNCTION_DESCRIPTION}: started"
    log "- Source:           [${AWS_S3_URL}]"
    log "- Destination:      [${AWS_SYNC_LOCAL_DIRECTORY}]"
    log "- Delete Flag:      [${AWS_S3_FLAG_DELETE_OPTION}]"
    log "- Exact Timestamps: [${AWS_S3_FLAG_EXACTTIMESTAMPS_OPTION}]"
    log "- Exclude:          [${EXCLUDES_LIST}]"
    log "- Include:          [${INCLUDES_LIST}]"
    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_S3_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_S3_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_S3_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current curl run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        log "${FUNCTION_DESCRIPTION}: Requesting download (source: [${AWS_S3_URL}] / destination: [${AWS_SYNC_LOCAL_DIRECTORY}]) (Attempt::${COUNTER} of ${AWS_S3_DEFAULT_RETRY_COUNT})"
        $(which aws) s3 sync "${AWS_S3_URL}" "${AWS_SYNC_LOCAL_DIRECTORY}" ${AWS_S3_FLAG_DELETE_STRING} ${AWS_S3_FLAG_EXACTTIMESTAMPS_STRING} ${AWS_S3_EXCLUDES} ${AWS_S3_INCLUDES} >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
        RETURNVAL="$?"
        # Fix Response File
        $(which sed) -i "s/\o15/_AWS_BAD_IGNORE\\n/g" "${AWS_FILE_RESPONSE}"
        $(which sed) -i '/_AWS_BAD_IGNORE/d' "${AWS_FILE_RESPONSE}"
        if [ ${RETURNVAL} -ne 0 ]; then
            log_add_from_file "${AWS_FILE_ERROR}" "${FUNCTION_DESCRIPTION}: Data containing error"
            log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Data"
            log_error "${FUNCTION_DESCRIPTION}: operation failed (aws_cli_exit_code::${RETURNVAL}])"
            RETRY_ENABLED=yes
            # Reset Files
            > ${AWS_FILE_RESPONSE}
            > ${AWS_FILE_ERROR}
        else
            log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Data"
            log "${FUNCTION_DESCRIPTION}: successfully downloaded files from S3"
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: s3_sync_upload()
## - Uploads files to AWS S3 Bucket (via sync method)
## - Arguments
##   - $1: S3 Bucket
##   - $2: S3 Directory
##   - $3: Local Directory
##   - $4: Delete (yes/no) - Files that exist in the destination but not in the source are deleted during sync (optional, defaults to no)
##   - $5: Exact Timestamps (yes/no) - When syncing from S3 to local, same-sized items  will  be  ignored  only  when  the timestamps match
#         exactly. The default behavior is to ignore same-sized items unless the local version is newer than the S3 version.
#        (optional, defaults to no)
##   - $6: Enable Server-side encryption (optional, defaults to yes)
##   - $7: Enable Reduced Redundancy (optional, defaults to yes)
##   - $8: Excludes (optional, semicolon separated)
##   - $9: Include (optional, semicolon separated)
function s3_sync_upload() {
    local FUNCTION_DESCRIPTION="AWS S3 Sync (Upload)"
    local AWS_S3_BUCKET="${1}"
    local AWS_S3_DIRECTORY="${2}"
    local AWS_SYNC_LOCAL_DIRECTORY="${3}"
    local AWS_S3_FLAG_DELETE_OPTION="${4}"
    local AWS_S3_FLAG_EXACTTIMESTAMPS_OPTION="${5}"
    local AWS_S3_FLAG_SERVERSIDENECRYPTION_OPTION="${6}"
    local AWS_S3_FLAG_REDUCEDREDUNDANCY_OPTION="${7}"
    local EXCLUDES_LIST="${8}"
    local INCLUDES_LIST="${9}"

    local RETURNVAL=""
    local TMP_STRING=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    local AWS_S3_URL=""
    local AWS_S3_FLAG_DELETE_STRING=""
    local AWS_S3_FLAG_EXACTTIMESTAMPS_STRING=""
    local AWS_S3_FLAG_SERVERSIDENECRYPTION_STRING=""
    local AWS_S3_FLAG_REDUCEDREDUNDANCY_STRING=""
    local AWS_S3_EXCLUDES=""
    local AWS_S3_INCLUDES=""

    if(is_empty "${AWS_S3_BUCKET}"); then
        log_error "${FUNCTION_DESCRIPTION}: S3 Bucket not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${AWS_SYNC_LOCAL_DIRECTORY}"); then
        log_error "${FUNCTION_DESCRIPTION}: Local Directory not specified"
        return $E_BAD_ARGS
    fi
    if [ ! -d "${AWS_SYNC_LOCAL_DIRECTORY}" ]; then
        log_error "${FUNCTION_DESCRIPTION}: Local Directory does not exist [${AWS_SYNC_LOCAL_DIRECTORY}]"
        return $E_OBJECT_NOT_FOUND
    fi

    if(is_empty "${AWS_S3_DIRECTORY}"); then
        AWS_S3_URL="s3://${AWS_S3_BUCKET}/"
    else
        AWS_S3_URL="s3://${AWS_S3_BUCKET}/${AWS_S3_DIRECTORY}/"
    fi
    if(is_empty "${AWS_S3_FLAG_DELETE_OPTION}"); then
        AWS_S3_FLAG_DELETE_OPTION=no
    fi
    if(is_empty "${AWS_S3_FLAG_EXACTTIMESTAMPS_OPTION}"); then
        AWS_S3_FLAG_EXACTTIMESTAMPS_OPTION=no
    fi
    if(is_empty "${AWS_S3_FLAG_SERVERSIDENECRYPTION_OPTION}"); then
        AWS_S3_FLAG_SERVERSIDENECRYPTION_OPTION=yes
    fi
    if(is_empty "${AWS_S3_FLAG_REDUCEDREDUNDANCY_OPTION}"); then
        AWS_S3_FLAG_REDUCEDREDUNDANCY_OPTION=yes
    fi
    if option_enabled AWS_S3_FLAG_DELETE_OPTION; then
        AWS_S3_FLAG_DELETE_STRING="--delete"
    fi
    if option_enabled AWS_S3_FLAG_EXACTTIMESTAMPS_OPTION; then
        AWS_S3_FLAG_EXACTTIMESTAMPS_STRING="--exact-timestamps"
    fi
    if option_enabled AWS_S3_FLAG_SERVERSIDENECRYPTION_OPTION; then
        AWS_S3_FLAG_SERVERSIDENECRYPTION_STRING="--sse"
    fi
    if option_enabled AWS_S3_FLAG_REDUCEDREDUNDANCY_OPTION; then
        AWS_S3_FLAG_REDUCEDREDUNDANCY_STRING="--storage-class REDUCED_REDUNDANCY"
    fi
    if (! is_empty "${EXCLUDES_LIST}"); then
        TMP_ARRAY=(${EXCLUDES_LIST//;/ })
        for TMP_STRING in "${TMP_ARRAY[@]}"; do
            AWS_S3_EXCLUDES="${AWS_S3_EXCLUDES} --exclude ${TMP_STRING} "
        done
    fi
    if (! is_empty "${INCLUDES_LIST}"); then
        TMP_ARRAY=(${INCLUDES_LIST//;/ })
        for TMP_STRING in "${TMP_ARRAY[@]}"; do
            AWS_S3_INCLUDES="${AWS_S3_INCLUDES} --exclude ${TMP_STRING} "
        done
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    log "${FUNCTION_DESCRIPTION}: started"
    log "- Source:             [${AWS_SYNC_LOCAL_DIRECTORY}]"
    log "- Destination:        [${AWS_S3_URL}]"
    log "- Delete Flag:        [${AWS_S3_FLAG_DELETE_OPTION}]"
    log "- Exact Timestamps:   [${AWS_S3_FLAG_EXACTTIMESTAMPS_OPTION}]"
    log "- Server Side Enc:    [${AWS_S3_FLAG_SERVERSIDENECRYPTION_OPTION}]"
    log "- Reduced Redundancy: [${AWS_S3_FLAG_REDUCEDREDUNDANCY_OPTION}]"
    log "- Exclude:            [${EXCLUDES_LIST}]"
    log "- Include:            [${INCLUDES_LIST}]"
    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_S3_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_S3_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_S3_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        log "${FUNCTION_DESCRIPTION}: Uploading files (Attempt::${COUNTER} of ${AWS_S3_DEFAULT_RETRY_COUNT})"
        $(which aws) s3 sync "${AWS_SYNC_LOCAL_DIRECTORY}" "${AWS_S3_URL}" ${AWS_S3_FLAG_DELETE_STRING} ${AWS_S3_FLAG_EXACTTIMESTAMPS_STRING} ${AWS_S3_FLAG_SERVERSIDENECRYPTION_STRING} ${AWS_S3_FLAG_REDUCEDREDUNDANCY_STRING} ${AWS_S3_EXCLUDES} ${AWS_S3_INCLUDES} >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
        RETURNVAL="$?"
        # Fix Response File
        $(which sed) -i "s/\o15/_AWS_BAD_IGNORE\\n/g" "${AWS_FILE_RESPONSE}"
        $(which sed) -i '/_AWS_BAD_IGNORE/d' "${AWS_FILE_RESPONSE}"
        if [ ${RETURNVAL} -ne 0 ]; then
            log_add_from_file "${AWS_FILE_ERROR}" "${FUNCTION_DESCRIPTION}: Data containing error"
            log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Data"
            log_error "${FUNCTION_DESCRIPTION}: operation failed (aws_cli_exit_code::${RETURNVAL}])"
            RETRY_ENABLED=yes
            # Reset Files
            > ${AWS_FILE_RESPONSE}
            > ${AWS_FILE_ERROR}
        else
            log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Data"
            log "${FUNCTION_DESCRIPTION}: successfully uploaded files to S3"
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: s3_verify()
## - Verifies connectivity to specified S3 bucket
## - Arguments
##   - $1: S3 Bucket
##   - $2: S3 Directory (optional, if left empty it will attempt to check against the root of the bucket)
##   - $3: Display Output (optional, yes/no, defaults to no)
function s3_verify() {
    local FUNCTION_DESCRIPTION="AWS S3 (Verify)"
    local AWS_S3_BUCKET="${1}"
    local AWS_S3_DIRECTORY="${2}"
    local DISPLAY_OUTPUT="${3}"

    local RETURNVAL=""
    local TMP_STRING=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    local AWS_S3_URL=""

    if(is_empty "${AWS_S3_BUCKET}"); then
        log_error "${FUNCTION_DESCRIPTION}: S3 Bucket not specified"
        return $E_BAD_ARGS
    fi

    if(is_empty "${AWS_S3_DIRECTORY}"); then
        AWS_S3_URL="s3://${AWS_S3_BUCKET}/"
    else
        AWS_S3_URL="s3://${AWS_S3_BUCKET}/${AWS_S3_DIRECTORY}/"
    fi
    if(is_empty "${DISPLAY_OUTPUT}"); then
        DISPLAY_OUTPUT=no
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    log "${FUNCTION_DESCRIPTION}: started"
    log "- S3 URL:           [${AWS_S3_URL}]"

    $(which aws) s3 ls "${AWS_S3_URL}" > ${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
    RETURNVAL="$?"
    # Fix Response File
        $(which sed) -i "s/\o15/_AWS_BAD_IGNORE\\n/g" "${AWS_FILE_RESPONSE}"
        $(which sed) -i '/_AWS_BAD_IGNORE/d' "${AWS_FILE_RESPONSE}"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_add_from_file "${AWS_FILE_ERROR}" "${FUNCTION_DESCRIPTION}: Data containing error"
        log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Data"
        log_error "${FUNCTION_DESCRIPTION}: operation failed (aws_cli_exit_code::${RETURNVAL}])"
        return $E_AWS_FAILURE
    fi
    if option_enabled DISPLAY_OUTPUT; then
        log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Data"
    fi
    log "${FUNCTION_DESCRIPTION}: verified"
    return 0
}
