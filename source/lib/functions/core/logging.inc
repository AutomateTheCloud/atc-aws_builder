###------------------------------------------------------------------------------------------------
# script:  logging.inc
# purpose: Collection of logging related functions
# version: 1.0.0
#
# function list:
#          - line_break
#          - log
#          - log_add_from_file
#          - log_error
#          - log_file_content
#          - log_highlight
#          - log_load_color_child
#          - log_load_color_parent
#          - log_notice
#          - log_quiet
#          - log_success
#          - log_warning
#          - verbose_display_date
###------------------------------------------------------------------------------------------------

###------------------------------------------------------------------------------------------------
# Variables
LOG_DATE_FORMAT="%Y-%m-%d %H:%M:%S.%3N %Z" # example: 2014-05-08 11:54:12.298 EDT
LOG_PAD="$(printf '%0.1s' "."{1..60})"

MIN_SELF_IDENTITY_LENGTH=24

VERBOSE_TRUNCATE_MIN_COLUMN_LENGTH=120
DEFAULT_ADD_TO_LOG_LINE_LIMIT=1000
DEFAULT_SCREEN_WIDTH=120

if [ ${#SELF_IDENTITY} -gt ${MIN_SELF_IDENTITY_LENGTH} ]; then
    LOG_INDENT_LENGTH=$((${#SELF_IDENTITY}+4))
else
    LOG_INDENT_LENGTH=$((${MIN_SELF_IDENTITY_LENGTH}+4))
fi

if option_enabled ENABLE_TIMESTAMP; then
    if option_enabled ENABLE_COLOR; then DETAILS_INDENT=$((${LOG_INDENT_LENGTH}+60)); else DETAILS_INDENT=$((${LOG_INDENT_LENGTH}+18)); fi
else
    if option_enabled ENABLE_COLOR; then DETAILS_INDENT=$((${LOG_INDENT_LENGTH}+26)); else DETAILS_INDENT=$((${LOG_INDENT_LENGTH}+4)); fi
fi

LOG_COLOR_DATE=""
LOG_COLOR_RCODE=""
LOG_COLOR_IDENT=""
LOG_COLOR_MESSAGE_IMPORTANT=""
LOG_COLOR_MESSAGE=""
LOG_COLOR_NOTICE=""
LOG_COLOR_WARNING=""
LOG_COLOR_ERROR=""
LOG_COLOR_SUCCESS=""
LOG_COLOR_FILE_HEADER=""
LOG_COLOR_FILE=""
LOG_COLOR_HIGHLIGHT=""

###------------------------------------------------------------------------------------------------
## FUNCTION: line_break()
## - prints to screen a line that spans across the full width (columns) of the open terminal
## - serves no purpose other than to be a pretty, pretty princess
function line_break() {
    local TMP_SCREEN_WIDTH=$($(which tput) cols 2>/dev/null)
    if (is_empty "${TMP_SCREEN_WIDTH}"); then
        TMP_SCREEN_WIDTH=${DEFAULT_SCREEN_WIDTH}
    fi
    printf '%*s\n' "${TMP_SCREEN_WIDTH}" '' | tr ' ' -
}

###------------------------------------------------------------------------------------------------
## FUNCTION: log()
## - echo log entry to screen
## - Arguments
##   - $1: entry string
##   - $2: Message Type
##   - $3: Message Content Color Override
function log() {
    local TMP_LOG_PREPEND=""
    local LOGDATE=$(date +"${LOG_DATE_FORMAT}")
    local TMP_LOG_COLOR="${LOG_COLOR_MESSAGE}"
    local TMP_LOG_STRING="${1}"
    local TMP_LOG_TYPE="$(to_upper "${2}")"
    local TMP_LOG_COLOR_CONTENT="${3}"
    local TMP_LOG_DATE=""
    local TMP_LOG_QUIET=no
    local TMP_VERBOSE_DETAILS=""
    local TMP_SCREEN_WIDTH=""
    local VERBOSE_MESSAGE=""
    if (is_empty "${TMP_LOG_TYPE}"); then
        TMP_LOG_TYPE="MESG"
    fi

    case $TMP_LOG_TYPE in
        MESG) TMP_LOG_COLOR="${LOG_COLOR_MESSAGE}";           TMP_LOG_PREPEND="";;
        NOTC) TMP_LOG_COLOR="${LOG_COLOR_NOTICE}";            TMP_LOG_PREPEND="";;
        SUCC) TMP_LOG_COLOR="${LOG_COLOR_SUCCESS}";           TMP_LOG_PREPEND="SUCCESS: ";;
        WARN) TMP_LOG_COLOR="${LOG_COLOR_WARNING}";           TMP_LOG_PREPEND="WARNING: ";;
        ERRO) TMP_LOG_COLOR="${LOG_COLOR_ERROR}";             TMP_LOG_PREPEND="ERROR: ";;
        HIGH) TMP_LOG_COLOR="${LOG_COLOR_HIGHLIGHT}";         TMP_LOG_PREPEND="";;
        SSTR) TMP_LOG_COLOR="${LOG_COLOR_MESSAGE_IMPORTANT}"; TMP_LOG_PREPEND="SCRIPT_START: ";;
        SFIN) TMP_LOG_COLOR="${LOG_COLOR_MESSAGE_IMPORTANT}"; TMP_LOG_PREPEND="SCRIPT_FINISHED: ";;
        SNFO) TMP_LOG_COLOR="${LOG_COLOR_MESSAGE_IMPORTANT}"; TMP_LOG_PREPEND="SCRIPT_INFO: ";;
        SINT) TMP_LOG_COLOR="${LOG_COLOR_MESSAGE_IMPORTANT}"; TMP_LOG_PREPEND="SCRIPT_INFO: ";    TMP_LOG_QUIET=yes;;
        SHLT) TMP_LOG_COLOR="${LOG_COLOR_MESSAGE_IMPORTANT}"; TMP_LOG_PREPEND="SCRIPT_HALT: ";    TMP_LOG_QUIET=yes;;
        FILE) TMP_LOG_COLOR="${LOG_COLOR_FILE}";              TMP_LOG_PREPEND="FILE_CONTENT: ";;
        QUIE) TMP_LOG_COLOR="${LOG_COLOR_MESSAGE}";           TMP_LOG_PREPEND="";                 TMP_LOG_QUIET=yes;;
        SCLI) TMP_LOG_COLOR="${COLOR_LIGHT_CYAN}";            TMP_LOG_PREPEND="CLI: ";;
        *)    TMP_LOG_COLOR="${LOG_COLOR_MESSAGE}";           TMP_LOG_PREPEND="";;
        ?)    TMP_LOG_COLOR="${LOG_COLOR_MESSAGE}";           TMP_LOG_PREPEND="";;
    esac

    if option_enabled ENABLE_COLOR; then
        if (is_empty "${TMP_LOG_COLOR_CONTENT}"); then
            TMP_LOG_COLOR_CONTENT="${TMP_LOG_COLOR}"
        fi
        if option_enabled ENABLE_TIMESTAMP; then
            TMP_LOG_DATE="$(color_text "${LOG_COLOR_DATE}" "$(verbose_display_date "${LOGDATE}")") "
        fi
        TMP_VERBOSE_DETAILS="$(printf "%s%s" "${TMP_LOG_DATE}" "$(printf "${COLOR_DARK_GRAY}")[$(color_text "${LOG_COLOR_IDENT}" "${SELF_IDENTITY}")$(printf "${COLOR_DARK_GRAY}")]")"
        TMP_VERBOSE_MESSAGE="$(color_text "${TMP_LOG_COLOR}" "${TMP_LOG_PREPEND}")$(color_text "${TMP_LOG_COLOR_CONTENT}" "${TMP_LOG_STRING}")"
    else
        if option_enabled ENABLE_TIMESTAMP; then
            TMP_LOG_DATE="$(verbose_display_date "${LOGDATE}") "
        fi
        TMP_VERBOSE_DETAILS="$(printf "%s%s" "${TMP_LOG_DATE}" "[${SELF_IDENTITY}]")"
        TMP_VERBOSE_MESSAGE="${TMP_LOG_PREPEND}${TMP_LOG_STRING}"
    fi
    if ! option_enabled TMP_LOG_QUIET; then
        printf "%s%*.*s %s\n" "${TMP_VERBOSE_DETAILS}" 0 $((${DETAILS_INDENT} - ${#TMP_VERBOSE_DETAILS} )) "${LOG_PAD}" "${TMP_VERBOSE_MESSAGE}"
    fi
}

###------------------------------------------------------------------------------------------------
## FUNCTION: log_add_from_file()
## - echo content of a file  to screen
## - Arguments
##   - $1: file containing data to add to log
##   - $2: string description of contents
##   - $3: number of lines to display (optional, defaults to 99999)
function log_add_from_file() {
    local ADD_TO_LOG_FILE="${1}"
    if (is_empty "${ADD_TO_LOG_FILE}"); then
        log_warning "Log Add from File Content: Log file not specified"
        return 1
    fi
    local ADD_TO_LOG_DESC=""
    if (! is_empty "${2}"); then
        ADD_TO_LOG_DESC=" / Description::${2}"
    fi
    local ADD_TO_LOG_LINE_LIMIT="${3}"
    local ADD_TO_LOG_LINE_LIMIT_STRING=""
    if (is_empty "${ADD_TO_LOG_LINE_LIMIT}"); then
        ADD_TO_LOG_LINE_LIMIT="${DEFAULT_ADD_TO_LOG_LINE_LIMIT}"
    else
        ADD_TO_LOG_LINE_LIMIT_STRING=" / Content_Line_Limit::${ADD_TO_LOG_LINE_LIMIT}"
    fi

    log "(File::[${ADD_TO_LOG_FILE}]${ADD_TO_LOG_DESC}${ADD_TO_LOG_LINE_LIMIT_STRING})" FILE
    # Check if file exists, if not, note it in the log but do not abort entire script
    if [ -f ${ADD_TO_LOG_FILE} ]; then
        log_file_content 0 "------------------------------------------------------------"
        local COUNTER=1
        while read -r LINE; do
            if [ ${COUNTER} -gt ${ADD_TO_LOG_LINE_LIMIT} ]; then
                log_file_content ${COUNTER} "----- TRUNCATED AT LINE [$((${COUNTER}-1))] -----"
                break
            fi
            log_file_content ${COUNTER} "${LINE}"
            COUNTER=$((${COUNTER}+1))
        done < "${ADD_TO_LOG_FILE}"
        log_file_content 0 "------------------------------------------------------------"
    else
        log "ERROR: Could not load file contents [${ADD_TO_LOG_FILE}]" FILE "${LOG_COLOR_ERROR}"
    fi
}

###------------------------------------------------------------------------------------------------
## FUNCTION: log_error()
## - echo log error entry to screen
## - this function prepends "ERROR:" to the beginning of the log entry to ensure consistency in the logs
## - Arguments
##   - $1: entry string
##   - $2: Message Content Color Override
function log_error() {
    local TMP_LOG_COLOR_CONTENT="${2}"
    log "${1}" ERRO "${TMP_LOG_COLOR_CONTENT}"
}

###--------------------------------------------------------------------------------
## FUNCTION: log_file_content()
## - logs file content
## - Arguments
##   - $1: Line Number
##   - $2: Entry string
function log_file_content() {
    local TMP_LINE_NUMBER="${1}"
    local TMP_LOG_STRING="${2}"
    
    if [ ${TMP_LINE_NUMBER} -eq 0 ]; then
        if option_enabled ENABLE_COLOR; then
            echo "$(color_text "${COLOR_DARK_GRAY}" "$(line_break)")"
        else
            line_break
        fi
    else
        if option_enabled ENABLE_COLOR; then
            echo "$(color_text "${COLOR_DARK_GRAY}" "[$(printf "%05d%s" "${TMP_LINE_NUMBER}")]") $(color_text "${LOG_COLOR_FILE}" "${TMP_LOG_STRING}")"
        else
            echo "[$(printf "%05d%s" "${TMP_LINE_NUMBER}")] ${TMP_LOG_STRING}"
        fi
    fi
}

###------------------------------------------------------------------------------------------------
## FUNCTION: log_highlight()
## - echo log highlight entry to screen
## - Arguments
##   - $1: entry string
function log_highlight() {
    log "[[ ${1} ]]" HIGH
}

###------------------------------------------------------------------------------------------------
## FUNCTION: log_load_color_child()
## - loads color palette for CHILD type scripts
function log_load_color_child() {
    LOG_COLOR_DATE="${COLOR_CYAN}"
    LOG_COLOR_RCODE="${COLOR_MAGENTA}"
    LOG_COLOR_IDENT="${COLOR_GREEN}"
    LOG_COLOR_MESSAGE_IMPORTANT="${COLOR_MAGENTA}"
    LOG_COLOR_MESSAGE="${COLOR_LIGHT_GRAY}"
    LOG_COLOR_NOTICE="${COLOR_CYAN}"
    LOG_COLOR_WARNING="${COLOR_LIGHT_YELLOW}"
    LOG_COLOR_ERROR="${COLOR_LIGHT_RED}"
    LOG_COLOR_SUCCESS="${COLOR_GREEN}"
    LOG_COLOR_FILE_HEADER="${COLOR_DARK_GRAY}"
    LOG_COLOR_FILE="${COLOR_CYAN}"
    LOG_COLOR_HIGHLIGHT="${COLOR_YELLOW}"
}

###------------------------------------------------------------------------------------------------
## FUNCTION: log_load_color_parent()
## - loads color palette for PARENT type scripts
function log_load_color_parent() {
    LOG_COLOR_DATE="${COLOR_LIGHT_CYAN}"
    LOG_COLOR_RCODE="${COLOR_LIGHT_MAGENTA}"
    LOG_COLOR_IDENT="${COLOR_LIGHT_GREEN}"
    LOG_COLOR_MESSAGE_IMPORTANT="${COLOR_LIGHT_MAGENTA}"
    LOG_COLOR_MESSAGE="${COLOR_WHITE}"
    LOG_COLOR_NOTICE="${COLOR_LIGHT_CYAN}"
    LOG_COLOR_WARNING="${COLOR_YELLOW}"
    LOG_COLOR_ERROR="${COLOR_RED}"
    LOG_COLOR_SUCCESS="${COLOR_LIGHT_GREEN}"
    LOG_COLOR_FILE_HEADER="${COLOR_DARK_GRAY}"
    LOG_COLOR_FILE="${COLOR_LIGHT_CYAN}"
    LOG_COLOR_HIGHLIGHT="${COLOR_LIGHT_YELLOW}"
}

###------------------------------------------------------------------------------------------------
## FUNCTION: log_notice()
## - echo log notice entry to screen
## - this function prepends "NOTICE:" to the beginning of the log entry to ensure consistency in the logs
## - Arguments
##   - $1: entry string
##   - $2: Message Content Color Override
function log_notice() {
    local TMP_LOG_COLOR_CONTENT="${2}"
    log "${1}" NOTC "${TMP_LOG_COLOR_CONTENT}"
}

###------------------------------------------------------------------------------------------------
## FUNCTION: log_quiet()
## - suppresses echoing log entry to screen
## - Arguments
##   - $1: entry string
function log_quiet() {
    log "${1}" QUIE
}

###------------------------------------------------------------------------------------------------
## FUNCTION: log_success()
## - echo log success entry to screen
## - this function prepends "SUCCESS:" to the beginning of the log entry to ensure consistency in the logs
## - Arguments
##   - $1: entry string
##   - $2: Message Content Color Override
function log_success() {
    local TMP_LOG_COLOR_CONTENT="${2}"
    log "${1}" SUCC "${TMP_LOG_COLOR_CONTENT}"
}

###------------------------------------------------------------------------------------------------
## FUNCTION: log_warning()
## - echo log warning entry to screen
## - this function prepends "WARNING:" to the beginning of the log entry to ensure consistency in the logs
## - Arguments
##   - $1: entry string
##   - $2: Message Content Color Override
function log_warning() {
    local TMP_LOG_COLOR_CONTENT="${2}"
    log "${1}" WARN "${TMP_LOG_COLOR_CONTENT}"
}

###------------------------------------------------------------------------------------------------
## FUNCTION: verbose_display_date()
## - truncates log date timestamp to HH:MM:SS only
## - Arguments
##   - $1: timestamp
function verbose_display_date() {
    local VERBOSE_TIMESTAMP="$(echo -n "${1}" | awk '{print $1" "$2}')"
    echo -n "${VERBOSE_TIMESTAMP:0:19}"
}
