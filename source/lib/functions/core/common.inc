###------------------------------------------------------------------------------------------------
# script:  common.inc
# purpose: Collection of common functions
# version: 1.0.0
#
# function list:
#          - add_element_to_array
#          - base64_decode
#          - base64_encode
#          - base64_encode_and_compress
#          - call_sleep
#          - call_sleep_random
#          - does_array_contain_element
#          - echo_key_value
#          - explode_key_value_to_file
#          - filesize_bytes_to_human_readable
#          - file_update_owner
#          - file_update_permissions
#          - function_exists
#          - generate_temp_directory
#          - generate_temp_file
#          - generate_uuid
#          - is_empty
#          - is_int
#          - is_server_up
#          - json_safe
#          - load_array_key_values_from_file
#          - load_array_key_values_from_yaml_file
#          - load_array_properties_from_file
#          - load_key_value_from_file
#          - load_property_from_file
#          - option_enabled
#          - return_file_last_modify_timestamp
#          - return_file_md5sum
#          - return_file_mime
#          - return_file_sha1sum
#          - return_file_sha256sum
#          - return_filesize_of_file
#          - return_filesize_of_file_in_bytes
#          - return_header_string
#          - return_parameter_string
#          - return_yaml_string
#          - source_include
#          - sync_disks
#          - time_elapsed
#          - timestamp_to_human_readable
#          - to_lower
#          - to_upper
#          - trim
#          - uptime_greater_than
#          - url_decode
#          - url_encode
#          - verify_array_key_values
#          - version_to_integer
#          - write_key_value_to_file
###------------------------------------------------------------------------------------------------

###------------------------------------------------------------------------------------------------
# Variables
VARIABLE_SEPARATOR_EXPLODE=";"

###------------------------------------------------------------------------------------------------
## FUNCTION: add_element_to_array()
## - Arguments
##   - $1: Array Name
##   - $2: Element
function add_element_to_array() {
    local R=$1 A
    shift
    for A; do
        eval "$R[\${#$R[@]}]=\$A"
    done
}

###------------------------------------------------------------------------------------------------
## FUNCTION: base64_decode()
## - Decodes a string to base64
## - Arguments
##   - $1: Variable name to pass info to
##   - $2: encoded string
function base64_decode() {
    local  __RESULTVAR=$1
    local ENCODED_STRING="${2}"
    local STRING="$(echo -n "$(echo "${ENCODED_STRING}" | $(which base64) -d | sed -e 's/^ *//g' -e 's/ *$//g')")"
    eval $__RESULTVAR="'${STRING}'"
}

###------------------------------------------------------------------------------------------------
## FUNCTION: base64_encode()
## - Encodes a string to base64
## - Arguments
##   - $1: Variable name to pass info to
##   - $2: string
function base64_encode() {
    local  __RESULTVAR=$1
    local STRING="${2}"
    local ENCODED_STRING="$(echo -n "$(echo "${STRING}" | $(which base64) --wrap 0 | sed -e 's/^ *//g' -e 's/ *$//g')")"
    eval $__RESULTVAR="'${ENCODED_STRING}'"
}

###------------------------------------------------------------------------------------------------
## FUNCTION: base64_encode_and_compress()
## - Encodes a string to base64 and compresses it (using gzip -9)
#   This was created to compress POST data before sending it to the API server
## - Arguments
##   - $1: Variable name to pass info to
##   - $2: string
function base64_encode_and_compress() {
    local  __RESULTVAR=$1
    local STRING="${2}"
    local ENCODED_STRING="$(echo -n "$(echo "${STRING}" | $(which gzip) -cf  | $(which base64) --wrap 0 | sed -e 's/^ *//g' -e 's/ *$//g')")"
    eval $__RESULTVAR="'${ENCODED_STRING}'"
}

###------------------------------------------------------------------------------------------------
## FUNCTION: call_sleep()
## - used to call sleep
## - Arguments
##   - $1: Number of seconds to sleep
##   - $2: Reason (optional)
function call_sleep() {
    local TMP_SECONDS="${1}"
    local TMP_REASON="${2}"
    local TMP_REASON_STRING=""
    if (! is_empty "${TMP_REASON}"); then
        TMP_REASON_STRING=" [${TMP_REASON}]"
    fi
    local STRING_SECOND="second"
    if [ ${TMP_SECONDS} -gt 1 ]; then
        STRING_SECOND="seconds"
    fi
    log "- Sleeping for ${TMP_SECONDS} ${STRING_SECOND}${TMP_REASON_STRING}"
    sleep ${TMP_SECONDS}
}

###------------------------------------------------------------------------------------------------
## FUNCTION: call_sleep_random()
## - used to call sleep, will randomize the amount of seconds based on Max sleep time
## - Arguments
##   - $1: Max sleep time (optional, defaults to 60 seconds if not specified)
function call_sleep_random() {
    local TIMER_WINDOW=60
    local STRING_SECOND="second"
    if (! is_empty "$1"); then
        TIMER_WINDOW=$1
    fi
    if [ "${TIMER_WINDOW}" == 0 2> /dev/null ]; then
        SLEEP_TIMER=0
    else
        SLEEP_TIMER=$[ ( $RANDOM % ${TIMER_WINDOW} )  + 1 ]
    fi
    if [ ${SLEEP_TIMER} -gt 1 ]; then
        STRING_SECOND="seconds"
    fi
    log "- Sleeping for ${SLEEP_TIMER} ${STRING_SECOND} (random, Base_Timer_Window::${TIMER_WINDOW})"
    sleep ${SLEEP_TIMER}
}

###------------------------------------------------------------------------------------------------
## FUNCTION: does_array_contain_element()
## - Arguments
##   - $1: Element string to search for
##   - $2: Array (pass this way: "${array[@]}")
function does_array_contain_element() {
    local E
    for E in "${@:2}"; do [[ "${E}" == "$1" ]] && return 0; done
    return 1
}

###------------------------------------------------------------------------------------------------
## FUNCTION: echo_key_value()
## - Returns Key Value line used in variable output files
## - Arguments
##   - $1: key
##   - $2: value
function echo_key_value() {
    local KEY="$(to_lower $(trim "${1}"))"
    local VALUE="$(trim "${2}")"
    echo "[${KEY}]=${VALUE}"
}

###------------------------------------------------------------------------------------------------
## FUNCTION: explode_variables_to_file()
## - Arguments
##   - $1: File to write to
##   - $2: Delimited Variable
function explode_key_value_to_file() {
    local OLD_IFS=${IFS}
    local VARNAME=""
    local VARVAL=""
    IFS="${VARIABLE_SEPARATOR_EXPLODE}" read -ra VAR1 <<< "${2}"
    for i in "${VAR1[@]}"; do
        if [[ "$i" =~ .*= ]]; then
            VARNAME=${i%%=*}
            VARVAL=${i#*=}
        else
            VARNAME=$i
            VARVAL="true"
        fi
        write_key_value_to_file "${1}" "${VARNAME}" "${VARVAL}"
    done
    IFS=${OLD_IFS}
}

###------------------------------------------------------------------------------------------------
## FUNCTION: file_update_owner()
## - Arguments
##   - $1: file
##   - $2: owner (example: apache:apache)
function file_update_owner() {
    local FILENAME="${1}"
    local OWNER="${2}"
    log "- Updating ownership (File::${FILENAME} / Owner::${OWNER})"
    if [ ! -f "${FILENAME}" ] && [ ! -d "${FILENAME}" ]; then
        log_error "- Could not update ownership, file not found [File::${FILENAME}]"
        return $E_OBJECT_NOT_FOUND
    fi
    $(which chown) ${OWNER} "${FILENAME}"
    local EXEC_EXIT=$?
    if [ "${EXEC_EXIT}" -ne 0 ]; then
        log_error "- Could not update ownership, chown returned ${EXEC_EXIT}"
        return $E_CHOWN_FAILURE
    fi
    return 0
}

###------------------------------------------------------------------------------------------------
## FUNCTION: file_update_permissions()
## - Arguments
##   - $1: file
##   - $2: owner (example: 0400)
function file_update_permissions() {
    local FILENAME="${1}"
    local PERMISSIONS="${2}"
    log "- Updating permissions (File::${FILENAME} / Permissions::${PERMISSIONS})"
    if [ ! -f "${FILENAME}" ] && [ ! -d "${FILENAME}" ]; then
        log_error "- Could not update permissions, file not found [File::${FILENAME}]"
        return $E_OBJECT_NOT_FOUND
    fi
    $(which chmod) ${PERMISSIONS} "${FILENAME}"
    local EXEC_EXIT=$?
    if [ "${EXEC_EXIT}" -ne 0 ]; then
        log_error "- Could not update permissions, chmod returned ${EXEC_EXIT}"
        return $E_CHMOD_FAILURE
    fi
    return 0
}

###------------------------------------------------------------------------------------------------
## FUNCTION: filesize_bytes_to_human_readable()
## - converts bytes into a human readable format (GB / MB / KB)
## - Arguments
##   - $1: filesize in bytes (as integer)
function filesize_bytes_to_human_readable() {
    local TMP_FILESIZE_IN_BYTES="${1}"
    local __RESULTVAR="${2}"
    local TMP_STRING=""
    TMP_STRING="$(awk -v fsize="${TMP_FILESIZE_IN_BYTES}" ' BEGIN {hum[1024^3]="GB"; hum[1024^2]="MB"; hum[1024]="KB"; for (x=1024^3; x>=1024; x/=1024) { if (fsize>=x) { printf "%.2f %s",fsize/x,hum[x]; break; } } if (fsize<1024) printf "%s %s",fsize,"bytes";; } ' | sed 's/.000 bytes/ bytes/g')"
    if (is_empty "${__RESULTVAR}"); then
        printf "${TMP_STRING}" | sed -e 's/^ *//g' -e 's/ *$//g'
    else
        eval $__RESULTVAR="'$(echo "$(trim "${TMP_STRING}")" | sed -e 's/^ *//g' -e 's/ *$//g')'"
    fi
}

###------------------------------------------------------------------------------------------------
## FUNCTION: function_exists()
## - Returns true if specified function name exists, otherwise false
## - Arguments
##   - $1: Function name
function function_exists() {
    if type ${1} 2>/dev/null |grep function; then
        return 0
    else
        return 1
    fi
}

###------------------------------------------------------------------------------------------------
## FUNCTION: generate_temp_directory()
## - creates a temporary directory and returns the name of said directory to the passed variable name
## - Arguments
##   - $1: Variable name to pass info to
##   - $2: Purpose string (optional)
##   - $3: Base Directory override (optional)
function generate_temp_directory() {
    local  __RESULTVAR=$1
    local PURPOSE=""
    local TMP_DIRECTORY=""
    if (is_empty "${2}"); then
        PURPOSE=""
    else
        PURPOSE=" (${2})"
    fi
    if (is_empty "${3}"); then
        TMP_DIRECTORY="${DIRECTORY_TEMPORARY}"
    else
        TMP_DIRECTORY="${3}"
        if [ ! -d "${TMP_DIRECTORY}" ]; then
            log_warning "- Generate Temp Directory: Specified base temp directory does not exist, defaulting to [${DIRECTORY_TEMPORARY}]"
            TMP_DIRECTORY="${DIRECTORY_TEMPORARY}"
        fi
    fi
    local GEN_DIRECTORY="$($(which mktemp) -q -d --tmpdir=${TMP_DIRECTORY})"
    local RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "- Generate Temp Directory: Failed to create temporary directory${PURPOSE}(Base_Location::${TMP_DIRECTORY})"
        exit_logic $E_OBJECT_FAILED_TO_CREATE "temporary directory"
    else
        TEMPORARY_DIRECTORIES+=( "${GEN_DIRECTORY}" )
        log_quiet "- Generate Temp Directory: Successfully created temp directory [${GEN_DIRECTORY}]${PURPOSE}"
    fi
    eval $__RESULTVAR="'${GEN_DIRECTORY}'"
}

###------------------------------------------------------------------------------------------------
## FUNCTION: generate_temp_file()
## - creates a temporary file and returns the name of said file to the passed variable name
## - Arguments
##   - $1: Variable name to pass info to
##   - $2: Purpose string (optional)
##   - $3: Directory override (optional)
function generate_temp_file() {
    local  __RESULTVAR=$1
    local PURPOSE=""
    local TMP_DIRECTORY=""
    if (is_empty "${2}"); then
        PURPOSE=""
    else
        PURPOSE=" (${2})"
    fi
    if (is_empty "${3}"); then
        TMP_DIRECTORY="${DIRECTORY_TEMPORARY}"
    else
        TMP_DIRECTORY="${3}"
        if [ ! -d "${TMP_DIRECTORY}" ]; then
            log_warning "- Generate Temp File: Specified temp directory does not exist, defaulting to [${DIRECTORY_TEMPORARY}]"
            TMP_DIRECTORY="${DIRECTORY_TEMPORARY}"
        fi
    fi
    local GEN_FILENAME="$($(which mktemp) -q --tmpdir=${TMP_DIRECTORY})"
    local RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "- Generate Temp File: Failed to create temporary file${PURPOSE}(Base_Location::${TMP_DIRECTORY})"
        exit_logic $E_OBJECT_FAILED_TO_CREATE "temporary file"
    else
        TEMPORARY_FILES+=( "${GEN_FILENAME}" )
        log_quiet "- Generate Temp File: Successfully created temp file [${GEN_FILENAME}]${PURPOSE}"
    fi

    eval $__RESULTVAR="'${GEN_FILENAME}'"
}

###------------------------------------------------------------------------------------------------
## FUNCTION: generate_uuid()
## - Creates a UUID (Removes hyphens from normal UUID format)
## - Arguments
##   - $1: Variable name to pass output to (optional)
##   - $2: String Length (optional)
function generate_uuid() {
    local __RESULTVAR="${1}"
    local TMP_STRING_LENGTH="${2}"
    local TMP_STRING_UUID="$($(which uuidgen) | sed -e 's/-//g' | tr "[:upper:]" "[:lower:]")"
    if (! is_empty "${TMP_STRING_LENGTH}"); then
        TMP_STRING_UUID="$(echo "${TMP_STRING_UUID}" | cut -c1-${TMP_STRING_LENGTH})"
    fi
    if (is_empty "${__RESULTVAR}"); then
        printf "${TMP_STRING_UUID}" | sed -e 's/^ *//g' -e 's/ *$//g'
    else
        eval $__RESULTVAR="'$(echo "$(trim "${TMP_STRING_UUID}")" | sed -e 's/^ *//g' -e 's/ *$//g')'"
    fi
    return 0
}

###------------------------------------------------------------------------------------------------
## FUNCTION: html_escape()
## - HTML Escapes string
## - Arguments
##   - $1: String
function html_escape () {
    local TMP_STRING="${1}"
    TMP_STRING=${TMP_STRING//&/&amp;}
    TMP_STRING=${TMP_STRING//</&lt;}
    TMP_STRING=${TMP_STRING//>/&gt;}
    TMP_STRING=${TMP_STRING//'"'/&quot;}
    TMP_STRING=${TMP_STRING//\//&#x2F;}
    printf "${TMP_STRING}"
}

###------------------------------------------------------------------------------------------------
## FUNCTION: is_empty()
## - Checks if a variable is set (not null) and has a value.
## - Arguments
##   - $1: Variable passed to check
##   - $2: Strict Checking - will return true if the value exists but is zero (optional, defaults to yes)
function is_empty() {
    local VAR="$1"
    local STRICT_CHECKING="${2}"
    if test -z "${STRICT_CHECKING}"; then
        STRICT_CHECKING=yes
    fi
    if [[ "${STRICT_CHECKING}" == "y" ]] || [[ "${STRICT_CHECKING}" == "yes" ]]; then
        if test -z "${VAR}"; then
            [[ $( echo "1" ) ]]
            return
        elif [ "${VAR}" == 0 2> /dev/null ]; then
            [[ $( echo "1" ) ]]
            return
        elif [ "${VAR}" == 0.0 2> /dev/null ]; then
            [[ $( echo "1" ) ]]
            return
        fi
        [[ $( echo "" ) ]]
    else
        if test -z "${VAR}"; then
            [[ $( echo "1" ) ]]
            return
        fi
        [[ $( echo "" ) ]]
    fi
}

###------------------------------------------------------------------------------------------------
## FUNCTION: is_int()
## - Checks if a variable an integer
## - Arguments
##   - $1: Variable passed to check
function is_int() {
    return $(test "$@" -eq "$@" > /dev/null 2>&1)
}

###------------------------------------------------------------------------------------------------
## FUNCTION: is_server_up()
## - performs ping test against specified server to determine if server is up
## - Arguments
##   - $1: Target server address
function is_server_up() {
    local SERVER_NAME="$1"
    $(which ping) -c 3 ${SERVER_NAME} > /dev/null 2>&1
    return $?
}

###------------------------------------------------------------------------------------------------
## FUNCTION: json_safe()
## - strips out or replaces characters that would mess up the JSON structure
## - Arguments
##   - $1: string to json_safe
function json_safe() {
    if which iconv > /dev/null 2>&1; then
        echo -n "$1" | sed -e 's/^ *//g' -e 's/ *$//g' | sed ':a;N;$!ba;s/\n/_/g' | sed -e "s/'/_/g" -e 's/"/_/g' | $(which iconv) -c -f utf-8 -t ascii
    else
        echo -n "$1" | sed -e 's/^ *//g' -e 's/ *$//g' | sed ':a;N;$!ba;s/\n/_/g' | sed -e "s/'/_/g" -e 's/"/_/g'
    fi
}

###------------------------------------------------------------------------------------------------
## FUNCTION: load_array_key_values_from_file()
## - loads Key values passed via an array from specified file
## - Arguments
##   - $1: array
##   - $2: file to load from
##   - $3: Prepend String (optional, if supplied automatically adds an Underscore)
##   - $4: Parse String (optional)
function load_array_key_values_from_file() {
    declare -a KEY_ARRAY=("${!1}")
    local KEY_VALUE_FILE="${2}"
    local PREPEND_KEY_STRING="${3}"
    local PARSE_STRING="${4}"
    if (! is_empty "${PREPEND_KEY_STRING}"); then
        PREPEND_KEY_STRING="${PREPEND_KEY_STRING}_"
    fi

    local KEY=""
    for KEY in "${KEY_ARRAY[@]}"; do
        local TMP_VALUE=""
        load_key_value_from_file TMP_VALUE "${KEY}" "${KEY_VALUE_FILE}" "${PARSE_STRING}"
        if (! is_empty "${TMP_VALUE}" no); then
            eval $(to_upper "${PREPEND_KEY_STRING}${KEY}")="'$(echo "$(trim "${TMP_VALUE}")" | sed -e 's/^ *//g' -e 's/ *$//g')'"
        fi
        unset TMP_VALUE
    done
}

###------------------------------------------------------------------------------------------------
## FUNCTION: load_array_key_values_from_yaml_file()
## - loads Key values passed via an array from specified YAML file
## - Arguments
##   - $1: array
##   - $2: file to load from
##   - $3: Prepend String (optional, if supplied automatically adds an Underscore)
function load_array_key_values_from_yaml_file() {
    declare -a KEY_ARRAY=("${!1}")
    local YAML_FILE="${2}"
    local PREPEND_KEY_STRING="${3}"
    if (! is_empty "${PREPEND_KEY_STRING}"); then
        PREPEND_KEY_STRING="${PREPEND_KEY_STRING}_"
    fi

    local KEY=""
    for KEY in "${KEY_ARRAY[@]}"; do
        local TMP_VALUE=""
        return_yaml_string TMP_VALUE "${KEY}" "${YAML_FILE}"
        eval $(to_upper "${PREPEND_KEY_STRING}${KEY}")="'$(echo "$(trim "${TMP_VALUE}")" | sed -e 's/^ *//g' -e 's/ *$//g')'"
        unset TMP_VALUE
    done
}

###------------------------------------------------------------------------------------------------
## FUNCTION: load_array_properties_from_file()
## - loads properties variables passed via an array from specified file
## - Arguments
##   - $1: array
##   - $2: file to load from
##   - $3: Prepend String (optional, if supplied automatically adds an Underscore)
function load_array_properties_from_file() {
    declare -a KEY_ARRAY=("${!1}")
    local KEY_VALUE_FILE="${2}"
    local PREPEND_KEY_STRING="${3}"
    if (! is_empty "${PREPEND_KEY_STRING}"); then
        PREPEND_KEY_STRING="${PREPEND_KEY_STRING}_"
    fi

    local KEY=""
    for KEY in "${KEY_ARRAY[@]}"; do
        local TMP_VALUE=""
        load_property_from_file TMP_VALUE "${KEY}" "${KEY_VALUE_FILE}" "${PARSE_STRING}"
        if (! is_empty "${TMP_VALUE}" no); then
            eval $(to_upper "${PREPEND_KEY_STRING}${KEY//./_}")="'$(echo "$(trim "${TMP_VALUE}")" | sed -e 's/^ *//g' -e 's/ *$//g')'"
        fi
        unset TMP_VALUE
    done
}

###------------------------------------------------------------------------------------------------
## FUNCTION: load_key_value_from_file()
## - Loads key value from file
## - Arguments
##   - $1: Variable name to pass info to (optional, if not specified outputs String to STDOUT)
##   - $2: Key name
##   - $3: Filename
##   - $4: Parse String (optional)
function load_key_value_from_file() {
    local __RESULTVAR="${1}"
    local KEY="${2}"
    local KEY_VALUE_FILE="${3}"
    local PARSE_STRING="${4}"
    local TMP_STRING=""
    if (is_empty "${KEY}") || (is_empty "${KEY_VALUE_FILE}"); then
        TMP_STRING=""
    else
        if (is_empty "${PARSE_STRING}"); then
            TMP_STRING="$(cat "${KEY_VALUE_FILE}" 2>/dev/null | grep "^\[${KEY}\]" | awk -F "]=" '{print $2}' | sed '/^\s*$/d' | tr -s ' ' | sed -e 's/^ *//g' -e 's/ *$//g')"
        else
            TMP_STRING="$(cat "${KEY_VALUE_FILE}" 2>/dev/null | grep "^\[${PARSE_STRING}\]" |awk -F "${PARSE_STRING}]" '{print $2}' | grep "^\[${KEY}\]" | awk -F "]=" '{print $2}' | sed '/^\s*$/d' | tr -s ' ' | sed -e 's/^ *//g' -e 's/ *$//g')"
        fi
    fi
    if (is_empty "${__RESULTVAR}"); then
        printf "${TMP_STRING}" | sed -e 's/^ *//g' -e 's/ *$//g'
    else
        eval $__RESULTVAR="'$(echo "$(trim "${TMP_STRING}")" | sed -e 's/^ *//g' -e 's/ *$//g')'"
    fi
}

###------------------------------------------------------------------------------------------------
## FUNCTION: load_property_from_file()
## - Loads property from file
## - Arguments
##   - $1: Variable name to pass info to (optional, if not specified outputs String to STDOUT)
##   - $2: Key name
##   - $3: Filename
function load_property_from_file() {
    local __RESULTVAR="${1}"
    local KEY="${2}"
    local KEY_VALUE_FILE="${3}"
    local PARSE_STRING="${4}"
    local TMP_STRING=""
    if (is_empty "${KEY}") || (is_empty "${KEY_VALUE_FILE}"); then
        TMP_STRING=""
    else
        TMP_STRING="$(cat "${KEY_VALUE_FILE}" | grep -i "^${KEY}=\|^${KEY//_/.}=\|^${KEY//./_}=" | awk '{sub(/=/,"-==-")}1' | awk -F'-==-' '{print $2}' | sed '/^\s*$/d' | tr -s ' ' | sed -e 's/^ *//g' -e 's/ *$//g')"
    fi
    if (is_empty "${__RESULTVAR}"); then
        printf "${TMP_STRING}" | sed -e 's/^ *//g' -e 's/ *$//g'
    else
        eval $__RESULTVAR="'$(echo "$(trim "${TMP_STRING}")" | sed -e 's/^ *//g' -e 's/ *$//g')'"
    fi
}

###------------------------------------------------------------------------------------------------
## FUNCTION: option_enabled()
## - Utilizes yes/no flag for variable. Returns true if 'yes'
## - Usage: "if option_enabled VERBOSE; then"  - NOTE: the argument is the variable name, not the variable (no $)
## - Arguments
##   - $1: Variable passed to check
function option_enabled() {
    VAR="$1"
    VAR_VALUE=$(eval echo \$$VAR)
    if [[ "${VAR_VALUE}" == "y" ]] || [[ "${VAR_VALUE}" == "yes" ]] || [[ "${VAR_VALUE}" == "Y" ]] || [[ "${VAR_VALUE}" == "YES" ]] || [[ "${VAR_VALUE}" == "1" ]] || [[ "${VAR_VALUE}" == "true" ]] || [[ "${VAR_VALUE}" == "TRUE" ]]; then
        return 0
    else
        return 1
    fi
}

###------------------------------------------------------------------------------------------------
## FUNCTION: return_file_last_modify_timestamp()
## - returns last modify timestamp of specified file
## - Arguments
##   - $1: filename
##   - $2: Variable name to pass info to (optional)
function return_file_last_modify_timestamp() {
    local CHECK_FILE="${1}"
    local __RESULTVAR="${2}"
    local TMP_STRING=""
    TMP_STRING="$($(which stat) -c %Y "${CHECK_FILE}" 2>/dev/null | awk '{print $1}')"
    if (is_empty "${__RESULTVAR}"); then
        printf "${TMP_STRING}" | sed -e 's/^ *//g' -e 's/ *$//g'
    else
        eval $__RESULTVAR="'$(echo "$(trim "${TMP_STRING}")" | sed -e 's/^ *//g' -e 's/ *$//g')'"
    fi
}

###------------------------------------------------------------------------------------------------
## FUNCTION: return_file_md5sum()
## - returns md5 checksum of file
## - Arguments
##   - $1: filename
##   - $2: Variable name to pass info to (optional)
function return_file_md5sum() {
    local CHECK_FILE="${1}"
    local __RESULTVAR="${2}"
    local TMP_STRING=""
    TMP_STRING="$($(which md5sum) "${CHECK_FILE}" 2>/dev/null | awk '{print $1}')"
    if (is_empty "${__RESULTVAR}"); then
        printf "${TMP_STRING}" | sed -e 's/^ *//g' -e 's/ *$//g'
    else
        eval $__RESULTVAR="'$(echo "$(trim "${TMP_STRING}")" | sed -e 's/^ *//g' -e 's/ *$//g')'"
    fi
}

###------------------------------------------------------------------------------------------------
## FUNCTION: return_file_mime()
## - returns MIME type of file
## - Arguments
##   - $1: filename
##   - $2: Variable name to pass info to (optional)
function return_file_mime() {
    local CHECK_FILE="${1}"
    local __RESULTVAR="${2}"
    local TMP_STRING=""
    TMP_STRING="$($(which file) --mime-type -b "${CHECK_FILE}" 2>/dev/null | awk '{print $1}')"
    if (is_empty "${__RESULTVAR}"); then
        printf "${TMP_STRING}" | sed -e 's/^ *//g' -e 's/ *$//g'
    else
        eval $__RESULTVAR="'$(echo "$(trim "${TMP_STRING}")" | sed -e 's/^ *//g' -e 's/ *$//g')'"
    fi
}

###------------------------------------------------------------------------------------------------
## FUNCTION: return_file_sha1sum()
## - returns sha1 checksum of file
## - Arguments
##   - $1: filename
##   - $2: Variable name to pass info to (optional)
function return_file_sha1sum() {
    local CHECK_FILE="${1}"
    local __RESULTVAR="${2}"
    local TMP_STRING=""
    TMP_STRING="$($(which sha1sum) "${CHECK_FILE}" 2>/dev/null | awk '{print $1}')"
    if (is_empty "${__RESULTVAR}"); then
        printf "${TMP_STRING}" | sed -e 's/^ *//g' -e 's/ *$//g'
    else
        eval $__RESULTVAR="'$(echo "$(trim "${TMP_STRING}")" | sed -e 's/^ *//g' -e 's/ *$//g')'"
    fi
}

###------------------------------------------------------------------------------------------------
## FUNCTION: return_file_sha256sum()
## - returns sha256 checksum of file
## - Arguments
##   - $1: filename
##   - $2: Variable name to pass info to (optional)
function return_file_sha256sum() {
    local CHECK_FILE="${1}"
    local __RESULTVAR="${2}"
    local TMP_STRING=""
    TMP_STRING="$($(which sha256sum) "${CHECK_FILE}" 2>/dev/null | awk '{print $1}')"
    if (is_empty "${__RESULTVAR}"); then
        printf "${TMP_STRING}" | sed -e 's/^ *//g' -e 's/ *$//g'
    else
        eval $__RESULTVAR="'$(echo "$(trim "${TMP_STRING}")" | sed -e 's/^ *//g' -e 's/ *$//g')'"
    fi
}

###------------------------------------------------------------------------------------------------
## FUNCTION: return_filesize_of_file()
## - returns filesize (in human-readable format) of specified file
## - Arguments
##   - $1: filename
##   - $2: Variable name to pass info to (optional)
function return_filesize_of_file() {
    local CHECK_FILE="${1}"
    local __RESULTVAR="${2}"
    local TMP_STRING=""
    TMP_STRING="$($(which stat) -c %s "${CHECK_FILE}" 2>/dev/null | awk '{print $1}')"
    if (is_empty "${__RESULTVAR}"); then
        printf "$(filesize_bytes_to_human_readable "${TMP_STRING}")" | sed -e 's/^ *//g' -e 's/ *$//g'
    else
        eval $__RESULTVAR="'$(echo "$(trim "$(filesize_bytes_to_human_readable "${TMP_STRING}")")" | sed -e 's/^ *//g' -e 's/ *$//g')'"
    fi
}

###------------------------------------------------------------------------------------------------
## FUNCTION: return_filesize_of_file_in_bytes()
## - returns filesize (in bytes) of specified file
## - Arguments
##   - $1: filename
##   - $2: Variable name to pass info to (optional)
function return_filesize_of_file_in_bytes() {
    local CHECK_FILE="${1}"
    local __RESULTVAR="${2}"
    local TMP_STRING=""
    TMP_STRING="$($(which stat) -c %s "${CHECK_FILE}" 2>/dev/null | awk '{print $1}')"
    if (is_empty "${__RESULTVAR}"); then
        printf "${TMP_STRING}" | sed -e 's/^ *//g' -e 's/ *$//g'
    else
        eval $__RESULTVAR="'$(echo "$(trim "${TMP_STRING}")" | sed -e 's/^ *//g' -e 's/ *$//g')'"
    fi
}

###------------------------------------------------------------------------------------------------
## FUNCTION: return_header_string()
## - Returns Header string information from comments of file
## - This keys off of the '# <header_identifier>: ' comment line found at the top of the scripts
## - Arguments
##   - $1: Header Identifier
##   - $2: File to inspect
##   - $3: Variable name to pass info to (optional)
function return_header_string() {
    local STRING_IDENTIFIER="${1}"
    local TARGET_FILE="${2}"
    local __RESULTVAR="${3}"
    local TMP_STRING="$(cat "${TARGET_FILE}" 2>/dev/null | grep "^# ${STRING_IDENTIFIER}:" | sed -e 's/[[:space:]]\+/ /g' | awk -F"# ${STRING_IDENTIFIER}:" '{print $2}' | sed -e 's/^ *//g' -e 's/ *$//g' | sed -e 's/^ *//g' -e 's/ *$//g')"
    if (is_empty "${__RESULTVAR}"); then
        printf "${TMP_STRING}" | sed -e 's/^ *//g' -e 's/ *$//g'
    else
        eval $__RESULTVAR="'$(echo "$(trim "${TMP_STRING}")")'"
    fi
}

###------------------------------------------------------------------------------------------------
## FUNCTION: return_parameter_string()
## - Returns Parameter string information from comments of file
## - This keys off of the '## <parameter_identifier>: ' comment line found inside the file
## - Arguments
##   - $1: Parameter Identifier
##   - $2: File to inspect
##   - $3: Variable name to pass info to (optional)
function return_parameter_string() {
    local STRING_IDENTIFIER="${1}"
    local TARGET_FILE="${2}"
    local __RESULTVAR="${3}"
    local TMP_STRING="$(cat "${TARGET_FILE}" 2>/dev/null | grep "^## ${STRING_IDENTIFIER}:" | sed -e 's/[[:space:]]\+/ /g' | awk -F"## ${STRING_IDENTIFIER}:" '{print $2}' | sed -e 's/^ *//g' -e 's/ *$//g')"
    if (is_empty "${__RESULTVAR}"); then
        printf "${TMP_STRING}" | sed -e 's/^ *//g' -e 's/ *$//g'
    else
        eval $__RESULTVAR="'$(echo "$(trim "${TMP_STRING}")")'"
    fi
}

###------------------------------------------------------------------------------------------------
## FUNCTION: return_yaml_string()
## - Returns YAML string information from file
## - Arguments
##   - $1: Variable name to pass info to (optional, if not specified outputs String to STDOUT)
##   - $2: String Identifier (Header)
##   - $3: File to inspect
function return_yaml_string() {
    local __RESULTVAR="${1}"
    local STRING_IDENTIFIER="${2}"
    local TARGET_FILE="${3}"
    local TMP_STRING="$(cat "${TARGET_FILE}" 2>/dev/null | sed -e 's/^[ \t]*//' | grep "^${STRING_IDENTIFIER}:" | awk -F"${STRING_IDENTIFIER}:" '{print $2}' | sed -e 's/^ *//g' -e 's/ *$//g' | sed -e 's/^ *//g' -e 's/ *$//g')"
    if (is_empty "${__RESULTVAR}"); then
        printf "${TMP_STRING}" | sed -e 's/^ *//g' -e 's/ *$//g'
    else
        eval $__RESULTVAR="'$(echo "$(trim "${TMP_STRING}")")'"
    fi
}

###------------------------------------------------------------------------------------------------
## FUNCTION: source_include()
## - Loads specified file as source and logs it
## - Arguments
##   - $1: Source file
function source_include() {
    if [ ! -f "${1}" ]; then
        log_error "${1} file not found, aborting"
        exit_logic $E_OBJECT_NOT_FOUND "file not found"
    fi
    source "${1}"
    log_quiet "Loaded source File::${1} [Version::$(return_script_version "${1}")]"
}

###------------------------------------------------------------------------------------------------
## FUNCTION: sync_disks()
## - Performs sync and logs it as an entry
function sync_disks() {
    log "- Flushing file system buffers [sync]"
    $(which sync)
}

###------------------------------------------------------------------------------------------------
## FUNCTION: time_elapsed()
## - calculates the amount of time elapsed between two timestamps
## - this function uses timestamps with microsecond precision
## - Arguments
##   - $1: Time Start (required)
##   - $2: Time End (optional, if not supplied it will calculate based on the current time)
function time_elapsed() {
    local TIME1="${1}"
    local TIME2="${2}"
    if (is_empty "${TIME2}"); then
        TIME2="$($(which date) +%s%6N)"
    fi
    local ELAPSED="$((${TIME2} - ${TIME1}))"
    local TIME_MICROSECONDS=0
    local TIME_MILLISECONDS=0
    local TIME_SECONDS=0

    ((TIME_MICROSECONDS=ELAPSED%1000))
    ((ELAPSED=ELAPSED/1000))
    ((TIME_MILLISECONDS=ELAPSED%1000))
    if((TIME_MICROSECONDS>500)); then
        ((TIME_MILLISECONDS=TIME_MILLISECONDS+1))
    fi
    ((ELAPSED=ELAPSED/1000))
    ((TIME_SECONDS=ELAPSED%60))
    ((ELAPSED=ELAPSED/60))
    echo "$(printf "%01dm %02d.%03ds" "${ELAPSED}" "${TIME_SECONDS}" "${TIME_MILLISECONDS}")"
}

###------------------------------------------------------------------------------------------------
## FUNCTION: timestamp_to_human_readable()
## - converts UNIX timestamp to human readable format
## - trims UNIX timestamp down to 10 characters (this allows us to pass extended timestamps (UNIX+microseconds) and not have an error
## - Arguments
##   - $1: UNIX timestamp
function timestamp_to_human_readable() {
    local TIMESTAMP="${1:0:10}"
    echo "$($(which date) --date @${TIMESTAMP} +'%Y-%m-%d %H:%M:%S %Z')"
}

###------------------------------------------------------------------------------------------------
## FUNCTION: to_lower()
## - Converts string to lowercase
## - Arguments
##   - $1: string to convert
function to_lower() {
   echo -n "${1}" | tr "[:upper:]" "[:lower:]"
}

###------------------------------------------------------------------------------------------------
## FUNCTION: to_upper()
## - Converts string to uppercase
## - Arguments
##   - $1: string to convert
function to_upper() {
   echo -n "${1}" | tr  "[:lower:]" "[:upper:]"
}

###------------------------------------------------------------------------------------------------
## FUNCTION: trim()
## - trims whitespace from left and right side of supplied string
## - Arguments
##   - $1: string to trim
function trim() {
    echo -n "$1" | sed -e 's/^ *//g' -e 's/ *$//g'
}

###------------------------------------------------------------------------------------------------
## FUNCTION: uptime_greater_than()
## - Utilizes yes/no flag for variable. Returns true if 'yes'
## - Arguments
##   - $1: Target uptime in seconds
function uptime_greater_than() {
    TARGET_SECONDS="${1}"
    if (is_empty "${TARGET_SECONDS}"); then
        TARGET_SECONDS=300
    fi
    UPTIME_IN_SECONDS="$(cat /proc/uptime | grep -o '^[0-9]\+')"
    if [ "${UPTIME_IN_SECONDS}" -gt "${TARGET_SECONDS}" ]; then
        return 0
    else
        return 1
    fi
}

###------------------------------------------------------------------------------------------------
## FUNCTION: url_decode()
## - URL Decodes string
## - Arguments
##   - $1: String
function url_decode() {
    local url_encoded="${1//+/ }"
    printf '%b' "${url_encoded//%/\\x}"
}

###------------------------------------------------------------------------------------------------
## FUNCTION: url_encode()
## - URL Encodes string
## - Arguments
##   - $1: String
function url_encode() {
    OLD_LC_COLLATE="${LC_COLLATE}"
    LC_COLLATE=C
    
    local LENGTH="${#1}"
    for (( i = 0; i < LENGTH; i++ )); do
        local c="${1:i:1}"
        case $c in
            [a-zA-Z0-9.~_-]) printf "$c" ;;
            *) printf '%%%02X' "'$c" ;;
        esac
    done
    LC_COLLATE="${OLD_LC_COLLATE}"
}

###------------------------------------------------------------------------------------------------
## FUNCTION: verify_array_key_values()
## - verifies Key values passed via an array are loaded into memory and are not empty
## - Arguments
##   - $1: array
##   - $2: Prepend String (optional, if supplied automatically adds an Underscore)
function verify_array_key_values() {
    declare -a KEY_ARRAY=("${!1}")
    local PREPEND_KEY_STRING="${2}"
    if (! is_empty "${PREPEND_KEY_STRING}"); then
        PREPEND_KEY_STRING="${PREPEND_KEY_STRING}_"
    fi
    local KEY=""
    local RETURNVAL=0
    for KEY in "${KEY_ARRAY[@]}"; do
        local TMP_VALUE="$(eval echo \${$(to_upper "${PREPEND_KEY_STRING}${KEY//./_}")})"
        if(is_empty "${TMP_VALUE}" no); then
            if (is_empty "${PREPEND_KEY_STRING}"); then
                log_warning "Verify Array Key Values: key is not set (Key::${KEY})"
            else
                log_warning "Verify Array Key Values: key is not set (Key::${KEY} / Prepend_Key_String::${PREPEND_KEY_STRING})"
            fi
            RETURNVAL=$E_OBJECT_NOT_FOUND
        fi
        unset TMP_VALUE
    done
    return $RETURNVAL
}

###------------------------------------------------------------------------------------------------
## FUNCTION: version_to_integer()
## - verifies Key values passed via an array are loaded into memory and are not empty
## - Arguments
##   - $1: Version String
##   - $2: Variable name to pass output to (optional)
function version_to_integer() {
    local STRING_VERSION_ORIGINAL="${1}"
    local __RESULTVAR="${2}"
    local STRING_VERSION="${STRING_VERSION_ORIGINAL}"
    local TMP_INT=""
    local RETURNVAL=0
    local V_OS_LEVEL=""
    local V_MAJOR=""
    local V_MINOR=""
    local V_REVISION=""

    # Clean version string (replace non-integers with spaces, theoretically, there should only be dots and dashes in a valid version)
    STRING_VERSION="${STRING_VERSION//./ }"
    STRING_VERSION="${STRING_VERSION//-/ }"

    V_OS_LEVEL="$(echo "${STRING_VERSION}" | cut -d ' ' -f 1)"
    V_MAJOR="$(echo "${STRING_VERSION}" | cut -d ' ' -f 2)"
    V_MINOR="$(echo "${STRING_VERSION}" | cut -d ' ' -f 3)"
    V_REVISION="$(echo "${STRING_VERSION}" | cut -d ' ' -f 4)"

    ( is_empty "${V_OS_LEVEL}" ) && V_OS_LEVEL=0
    ( is_empty "${V_MAJOR}" ) && V_MAJOR=0
    ( is_empty "${V_MINOR}" ) && V_MINOR=0
    ( is_empty "${V_REVISION}" ) && V_REVISION=0

    # Verification that we are only dealing with numbers
    [ -z "${V_OS_LEVEL##*[!0-9]*}" ] && log_error "Version to Integer: Invalid OS Level [string_version::${STRING_VERSION_ORIGINAL} / os_level::${V_OS_LEVEL}]" && return 1
    [ -z "${V_MAJOR##*[!0-9]*}" ] && log_error "Version to Integer: Invalid Major [string_version::${STRING_VERSION_ORIGINAL} / major::${V_MAJOR}]" && return 1
    [ -z "${V_MINOR##*[!0-9]*}" ] && log_error "Version to Integer: Invalid Minor [string_version::${STRING_VERSION_ORIGINAL} / minor::${V_MINOR}]" && return 1
    [ -z "${V_REVISION##*[!0-9]*}" ] && log_error "Version to Integer: Invalid Revision [string_version::${STRING_VERSION_ORIGINAL} / revision::${V_REVISION}]" && return 1

    [ ${V_OS_LEVEL} -ge 1000 ] && log_error "Version to Integer: Invalid OS Level, integer >= 1000 [string_version::${STRING_VERSION_ORIGINAL} / os_level::${V_OS_LEVEL}]" && return 1
    [ ${V_MAJOR} -ge 1000 ] && log_error "Version to Integer: Invalid Major integer >= 1000 [string_version::${STRING_VERSION_ORIGINAL} / major::${V_MAJOR}]" && return 1
    [ ${V_MINOR} -ge 1000 ] && log_error "Version to Integer: Invalid Minor integer >= 1000 [string_version::${STRING_VERSION_ORIGINAL} / minor::${V_MINOR}]" && return 1
    [ ${V_REVISION} -ge 1000 ] && log_error "Version to Integer: Invalid Revision integer >= 1000 [string_version::${STRING_VERSION_ORIGINAL} / revision::${V_REVISION}]" && return 1

    TMP_INT=$(($((${V_OS_LEVEL} * 1000000000)) + $((${V_MAJOR} * 1000000)) + $((${V_MINOR} * 1000)) + $((${V_REVISION} * 1)) ))

    if (is_empty "${__RESULTVAR}"); then
        printf "$(trim "${TMP_INT}")" | sed -e 's/^ *//g' -e 's/ *$//g'
    else
        eval $__RESULTVAR="'$(to_upper "$(trim "${TMP_INT}")" | sed -e 's/^ *//g' -e 's/ *$//g')'"
    fi
    return $RETURNVAL
}

###------------------------------------------------------------------------------------------------
## FUNCTION: write_key_value_to_file()
## - Writes [key]=value line to file
## - Arguments
##   - $1 : File to write to
##   - $2 : Variable Name
##   - $3 : Variable Value
function write_key_value_to_file() {
    echo "[${2}]=${3}" >> ${1}
}
