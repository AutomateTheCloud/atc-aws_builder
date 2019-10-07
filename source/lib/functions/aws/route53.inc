###------------------------------------------------------------------------------------------------
# script:  route53.inc
# purpose: Collection of functions related to AWS (Route53)
# version: 1.0.0
#
# function list:
#          - route53_delete_record
#          - route53_delete_record_call
#          - route53_get_record
#          - route53_upsert_record
###------------------------------------------------------------------------------------------------
REQUIRED_EXECUTABLES+=('aws')

###------------------------------------------------------------------------------------------------
# Variables
AWS_ROUTE53_DEFAULT_RETRY_COUNT=3
AWS_ROUTE53_DEFAULT_RETRY_TIMER_MAX_SEC=30
AWS_ROUTE53_DEFAULT_TTL=600

###------------------------------------------------------------------------------------------------
## FUNCTION: route53_delete_record()
## - Route53: Performs Delete on Record
## - Arguments
##   - $1: Zone ID
##   - $2: Record Name
##   - $3: Region
function route53_delete_record() {
    local FUNCTION_DESCRIPTION="Route53 (Delete Record)"
    local TMP_ZONE_ID="${1}"
    local TMP_RECORD_NAME="${2}"
    local TMP_AWS_REGION="${3}"

    local RETURNVAL=""

    local TMP_ARRAY_RECORDS=()
    local TMP_RECORD_TYPE=""
    local TMP_RECORD_TTL=""
    local TMP_RECORD_VALUE=""
    local TMP_RECORD_WEIGHT=""
    local TMP_RECORD_SET_IDENTIFIER=""

    if(is_empty "${TMP_ZONE_ID}"); then log_error "${FUNCTION_DESCRIPTION}: Zone ID not specified"; return $E_BAD_ARGS; fi
    if(is_empty "${TMP_RECORD_NAME}"); then log_error "${FUNCTION_DESCRIPTION}: Record Name not specified"; return $E_BAD_ARGS; fi

    log_notice "${FUNCTION_DESCRIPTION}: started"
    log "- Name:    [${TMP_RECORD_NAME}]"
    log "- Zone ID: [${TMP_ZONE_ID}]"

    log "${FUNCTION_DESCRIPTION}: Retrieving current record information"
    route53_get_record TMP_ARRAY_RECORDS "${TMP_ZONE_ID}" "${TMP_RECORD_NAME}" "${TMP_AWS_REGION}"
    RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then return $RETURNVAL; fi

    if [ ${#TMP_ARRAY_RECORDS[@]} -eq 0 ]; then
        log "${FUNCTION_DESCRIPTION}: No Records found, no need to perform delete"
        return 0
    fi

    log "${FUNCTION_DESCRIPTION}: Starting Deletion process"
    for TMP_KEY in "${TMP_ARRAY_RECORDS[@]}"; do
        TMP_RECORD_TYPE="$(echo -n "${TMP_KEY}" | awk -F'|' '{print $2}')"
        TMP_RECORD_TTL="$(echo -n "${TMP_KEY}" | awk -F'|' '{print $3}')"
        TMP_RECORD_VALUE="$(echo -n "${TMP_KEY}" | awk -F'|' '{print $4}')"
        TMP_RECORD_WEIGHT="$(echo -n "${TMP_KEY}" | awk -F'|' '{print $5}')"
        TMP_RECORD_SET_IDENTIFIER="$(echo -n "${TMP_KEY}" | awk -F'|' '{print $6}')"
        route53_delete_record_call "${TMP_ZONE_ID}" "${TMP_RECORD_NAME}" "${TMP_RECORD_TYPE}" "${TMP_RECORD_TTL}" "${TMP_RECORD_VALUE}" "${TMP_RECORD_WEIGHT}" "${TMP_RECORD_SET_IDENTIFIER}" "${TMP_AWS_REGION}"
        RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then return $RETURNVAL; fi
    done

    log "${FUNCTION_DESCRIPTION}: finished"
    return 0
}

###------------------------------------------------------------------------------------------------
## FUNCTION: route53_delete_record_call()
## - Route53: Performs Delete on Record (the actual CLI call to perform delete)
## - Arguments
##   - $1: Zone ID
##   - $2: Record Name
##   - $3: Record Type
##   - $4: Record TTL
##   - $5: Record Value
##   - $6: Record Weight (optional, used for weighted records)
##   - $7: Record Set Identifier (optional, used for weighted records)
##   - $8: Region
function route53_delete_record_call() {
    local FUNCTION_DESCRIPTION="Route53 (Delete Record)"
    local TMP_ZONE_ID="${1}"
    local TMP_RECORD_NAME="${2}"
    local TMP_RECORD_TYPE="${3}"
    local TMP_RECORD_TTL="${4}"
    local TMP_RECORD_VALUE="${5}"
    local TMP_RECORD_WEIGHT="${6}"
    local TMP_RECORD_SET_IDENTIFIER="${7}"
    local TMP_AWS_REGION="${8}"

    local RETURNVAL=""
    local TMP_STRING=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local TMP_RECORD_WEIGHT_JSON=""
    local TMP_RECORD_SET_IDENTIFIER_JSON=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    local AWS_TMP_JSON=""

    if(is_empty "${TMP_ZONE_ID}"); then log_error "${FUNCTION_DESCRIPTION}: Zone ID not specified"; return $E_BAD_ARGS; fi
    if(is_empty "${TMP_RECORD_NAME}"); then log_error "${FUNCTION_DESCRIPTION}: Record Name not specified"; return $E_BAD_ARGS; fi
    if(is_empty "${TMP_RECORD_TYPE}"); then log_error "${FUNCTION_DESCRIPTION}: Record Type not specified"; return $E_BAD_ARGS; fi
    if(is_empty "${TMP_RECORD_TTL}"); then log_error "${FUNCTION_DESCRIPTION}: Record TTL not specified"; return $E_BAD_ARGS; fi
    if(is_empty "${TMP_RECORD_VALUE}"); then log_error "${FUNCTION_DESCRIPTION}: Record Value not specified"; return $E_BAD_ARGS; fi

    if [[ "ZZ_$(to_upper "${TMP_RECORD_WEIGHT}")" == "ZZ_NONE" ]]; then TMP_RECORD_WEIGHT=""; fi
    if [[ "ZZ_$(to_upper "${TMP_RECORD_SET_IDENTIFIER}")" == "ZZ_NONE" ]]; then TMP_RECORD_SET_IDENTIFIER=""; fi

    if(! is_empty "${TMP_AWS_REGION}"); then AWS_REGION_STRING="--region ${TMP_AWS_REGION}"; fi

    if(! is_empty "${TMP_RECORD_WEIGHT}"); then
        TMP_RECORD_WEIGHT_JSON="\"Weight\": ${TMP_RECORD_WEIGHT},"
    fi
    if(! is_empty "${TMP_RECORD_SET_IDENTIFIER}"); then
        TMP_RECORD_SET_IDENTIFIER_JSON="\"SetIdentifier\": \"${TMP_RECORD_SET_IDENTIFIER}\","
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"
    generate_temp_file AWS_TMP_JSON "aws temp json"

    log_notice "${FUNCTION_DESCRIPTION}: started"
    log "- Name:          [${TMP_RECORD_NAME}]"
    log "- Type:          [${TMP_RECORD_TYPE}]"
    log "- Resource:      [${TMP_RECORD_VALUE}]"
    log "- TTL:           [${TMP_RECORD_TTL}]"
    log "- Weight:        [${TMP_RECORD_WEIGHT}]"
    log "- SetIdentifier: [${TMP_RECORD_SET_IDENTIFIER}]"
    log "- Zone ID:       [${TMP_ZONE_ID}]"

cat << ZZEOF > ${AWS_TMP_JSON}
{
  "Changes": [
    {
      "Action": "DELETE",
      "ResourceRecordSet": {
        "Name": "${TMP_RECORD_NAME}",
        "Type": "${TMP_RECORD_TYPE}",
        "TTL": ${TMP_RECORD_TTL},
        ${TMP_RECORD_WEIGHT_JSON}
        ${TMP_RECORD_SET_IDENTIFIER_JSON}
        "ResourceRecords": [
          { "Value": "${TMP_RECORD_VALUE}" }
        ]
      }
    }
  ]
}
ZZEOF
    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_ROUTE53_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_ROUTE53_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_ROUTE53_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current curl run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        log "${FUNCTION_DESCRIPTION}: Attempting to Delete Record (Attempt::${COUNTER} of ${AWS_ROUTE53_DEFAULT_RETRY_COUNT})"
        $(which aws) ${AWS_REGION_STRING} route53 change-resource-record-sets --hosted-zone-id ${TMP_ZONE_ID} --change-batch file://${AWS_TMP_JSON} >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
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
            log "${FUNCTION_DESCRIPTION}: Successfully deleted record"
            call_sleep 1
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: route53_get_record()
## - Retrieves current Record Information and returns value list as an Array Set
## - Arguments
##   - $1: Array name to pass info to
##   - $2: Zone ID
##   - $3: Record Name
##   - $4: Region
function route53_get_record() {
    local FUNCTION_DESCRIPTION="Route53 (Get Record)"
    local __RESULTVAR=$1
    local TMP_ZONE_ID="${2}"
    local TMP_RECORD_NAME="${3}"
    local TMP_AWS_REGION="${4}"

    local RETURNVAL=""
    local TMP_STRING=""
    local TMP_KEY=""
    local TMP_ARRAY_RECORD=()
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    if(is_empty "${TMP_ZONE_ID}"); then
        log_error "${FUNCTION_DESCRIPTION}: Zone ID not specified"
        return $E_BAD_ARGS
    fi
    if(is_empty "${TMP_RECORD_NAME}"); then
        log_error "${FUNCTION_DESCRIPTION}: Record Name not specified"
        return $E_BAD_ARGS
    fi

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_RDS_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_RDS_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_RDS_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current aws run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        $(which aws) ${AWS_REGION_STRING} route53 list-resource-record-sets \
          --hosted-zone-id "${TMP_ZONE_ID}" \
          --no-paginate \
          --query "ResourceRecordSets[].[Name,Type,TTL,ResourceRecords[0].Value,Weight,SetIdentifier]" \
          --output text 2>${AWS_FILE_ERROR} \
          | awk "\$1 ~ \"^${TMP_RECORD_NAME}\"" | sort -r | sed -e 's/[[:space:]]\+/|/g' >${AWS_FILE_RESPONSE}
        RETURNVAL="$?"
        # Fix Response File
        $(which sed) -i "s/\o15/_AWS_BAD_IGNORE\\n/g" "${AWS_FILE_RESPONSE}"
        $(which sed) -i '/_AWS_BAD_IGNORE/d' "${AWS_FILE_RESPONSE}"
        if [ ${RETURNVAL} -ne 0 ]; then
            if grep -q "DBInstanceNotFound" "${AWS_FILE_ERROR}"; then
                eval $__RESULTVAR=""
                return 0
            fi
            log_add_from_file "${AWS_FILE_ERROR}" "${FUNCTION_DESCRIPTION}: Data containing error"
            log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Data"
            log_error "${FUNCTION_DESCRIPTION}: operation failed (aws_cli_exit_code::${RETURNVAL}])"
            RETRY_ENABLED=yes
            # Reset Files
            > ${AWS_FILE_RESPONSE}
            > ${AWS_FILE_ERROR}
        else
            TMP_STRING="$(cat ${AWS_FILE_RESPONSE}  2>/dev/null | grep -v "^#\|^;" | sed -e 's/[ \t]*$//' -e '/^$/d' -e '$ s/,$//g' -e 's/[ \t]*$//' -e '/^$/d')"
            if(! is_empty "${TMP_STRING}"); then
                local OLD_IFS="${IFS}"
                IFS=$'\n'
                for TMP_KEY in $(cat ${AWS_FILE_RESPONSE}  2>/dev/null | grep -v "^#\|^;" | sed -e 's/[ \t]*$//' -e '/^$/d' -e '$ s/,$//g' -e 's/[ \t]*$//' -e '/^$/d'); do
                    if(! is_empty "${TMP_KEY}"); then
                        TMP_ARRAY_RECORD+=("${TMP_KEY}")
                    fi
                done
                IFS="${OLD_IFS}"
                readarray -t TMP_ARRAY_RECORD < <(for a in "${TMP_ARRAY_RECORD[@]}"; do echo "${a}"; done | grep -v '^$' | sort | uniq)
                for TMP_KEY in "${TMP_ARRAY_RECORD[@]}"; do
                    eval $__RESULTVAR+="('${TMP_KEY}')"
                done

            fi
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: route53_upsert_record()
## - Route53: Performs Upsert on Record
## - Arguments
##   - $1: Zone ID
##   - $2: Record Name
##   - $3: Record Type
##   - $4: Record TTL (optional, defaults to AWS_ROUTE53_DEFAULT_TTL)
##   - $5: Record Value (semicolon delimited)
##   - $6: Record Weight (optional, used for weighted records)
##   - $7: Record Set Identifier (optional, used for weighted records)
##   - $8: Region
function route53_upsert_record() {
    local FUNCTION_DESCRIPTION="Route53 (Upsert Record)"
    local TMP_ZONE_ID="${1}"
    local TMP_RECORD_NAME="${2}"
    local TMP_RECORD_TYPE="${3}"
    local TMP_RECORD_TTL="${4}"
    local TMP_RECORD_VALUE="${5}"
    local TMP_RECORD_WEIGHT="${6}"
    local TMP_RECORD_SET_IDENTIFIER="${7}"
    local TMP_AWS_REGION="${8}"

    local RETURNVAL=""
    local TMP_STRING=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    local TMP_RECORD_WEIGHT_JSON=""
    local TMP_RECORD_SET_IDENTIFIER_JSON=""
    local TMP_ARRAY_AWS_ROUTE53_RECORD_VALUE=""
    local TMP_RECORD_VALUE_JSON=""

    local RUN=1
    local COUNTER=0
    local RETRY_ENABLED=no
    local HAS_ERROR=no

    local AWS_TMP_JSON=""

    if(is_empty "${TMP_ZONE_ID}"); then log_error "${FUNCTION_DESCRIPTION}: Zone ID not specified"; return $E_BAD_ARGS; fi
    if(is_empty "${TMP_RECORD_NAME}"); then log_error "${FUNCTION_DESCRIPTION}: Record Name not specified"; return $E_BAD_ARGS; fi
    if(is_empty "${TMP_RECORD_TYPE}"); then log_error "${FUNCTION_DESCRIPTION}: Record Type not specified"; return $E_BAD_ARGS; fi
    if(is_empty "${TMP_RECORD_TTL}"); then log_error "${FUNCTION_DESCRIPTION}: Record TTL not specified"; return $E_BAD_ARGS; fi
    if(is_empty "${TMP_RECORD_VALUE}"); then log_error "${FUNCTION_DESCRIPTION}: Record Value not specified"; return $E_BAD_ARGS; fi

    if [[ "ZZ_$(to_upper "${TMP_RECORD_WEIGHT}")" == "ZZ_NONE" ]]; then TMP_RECORD_WEIGHT=""; fi
    if [[ "ZZ_$(to_upper "${TMP_RECORD_SET_IDENTIFIER}")" == "ZZ_NONE" ]]; then TMP_RECORD_SET_IDENTIFIER=""; fi

    if(! is_empty "${TMP_AWS_REGION}"); then AWS_REGION_STRING="--region ${TMP_AWS_REGION}"; fi

    if(! is_empty "${TMP_RECORD_WEIGHT}"); then
        TMP_RECORD_WEIGHT_JSON="\"Weight\": ${TMP_RECORD_WEIGHT},"
    fi
    if(! is_empty "${TMP_RECORD_SET_IDENTIFIER}"); then
        TMP_RECORD_SET_IDENTIFIER_JSON="\"SetIdentifier\": \"${TMP_RECORD_SET_IDENTIFIER}\","
    fi

    TMP_ARRAY_AWS_ROUTE53_RECORD_VALUE=(${TMP_RECORD_VALUE//;/ })
    for TMP_STRING in "${TMP_ARRAY_AWS_ROUTE53_RECORD_VALUE[@]}"; do
        if(! is_empty "${TMP_STRING}"); then
            TMP_RECORD_VALUE_JSON="${TMP_RECORD_VALUE_JSON}{ \"Value\": \"${TMP_STRING}\" },"
        fi
    done
    TMP_RECORD_VALUE_JSON="$(echo -n "${TMP_RECORD_VALUE_JSON}" | sed -e 's/,*$//g')"

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"
    generate_temp_file AWS_TMP_JSON "aws temp json"

    log_notice "${FUNCTION_DESCRIPTION}: started"
    log "- Name:          [${TMP_RECORD_NAME}]"
    log "- Type:          [${TMP_RECORD_TYPE}]"
    log "- Resource:      [${TMP_RECORD_VALUE}]"
    log "- TTL:           [${TMP_RECORD_TTL}]"
    log "- Weight:        [${TMP_RECORD_WEIGHT}]"
    log "- SetIdentifier: [${TMP_RECORD_SET_IDENTIFIER}]"
    log "- Zone ID:       [${TMP_ZONE_ID}]"

cat << ZZEOF > ${AWS_TMP_JSON}
{
  "Changes": [
    {
      "Action": "UPSERT",
      "ResourceRecordSet": {
        "Name": "${TMP_RECORD_NAME}",
        "Type": "${TMP_RECORD_TYPE}",
        "TTL": ${TMP_RECORD_TTL},
        ${TMP_RECORD_WEIGHT_JSON}
        ${TMP_RECORD_SET_IDENTIFIER_JSON}
        "ResourceRecords": [
          ${TMP_RECORD_VALUE_JSON}
        ]
      }
    }
  ]
}
ZZEOF
    while [ ${RUN} == 1 ]; do
        COUNTER=$((${COUNTER}+1))
        if [ ${COUNTER} -gt ${AWS_ROUTE53_DEFAULT_RETRY_COUNT} ]; then
            log_error "${FUNCTION_DESCRIPTION}: retry count [${AWS_ROUTE53_DEFAULT_RETRY_COUNT}] exceeded, aborting operation"
            return $E_AWS_FAILURE
        fi
        if option_enabled RETRY_ENABLED; then
            call_sleep_random ${AWS_ROUTE53_DEFAULT_RETRY_TIMER_MAX_SEC}
        fi

        # Reset Temporary Variables for the current curl run
        HAS_ERROR=no
        RETURNVAL=""
        RETRY_ENABLED=no

        log "${FUNCTION_DESCRIPTION}: Attempting to Create Record (Attempt::${COUNTER} of ${AWS_ROUTE53_DEFAULT_RETRY_COUNT})"
        $(which aws) ${AWS_REGION_STRING} route53 change-resource-record-sets --hosted-zone-id ${TMP_ZONE_ID} --change-batch file://${AWS_TMP_JSON} >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
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
            log "${FUNCTION_DESCRIPTION}: Successfully created record"
            call_sleep 1
            return 0
        fi
    done
    # Theoretically, we should never make it here, however, if we do for whatever reason, it is because of a failure. This is here to serve as a catch-all
    return $E_AWS_FAILURE
}
