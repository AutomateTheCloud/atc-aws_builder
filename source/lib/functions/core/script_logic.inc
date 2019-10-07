###------------------------------------------------------------------------------------------------
# script:  script_logic.inc
# purpose: Collection of Script Logic related functions.
# version: 1.0.0
#
# function list:
#          - cleanup
#          - exec_script
#          - exit_logic
#          - exit_logic_skip_cleanup
#          - exit_trap
#          - exit_trap_ensure_graceful
#          - lookup_exit_code
#          - return_script_purpose
#          - return_script_version
#          - sort_array_required_executeables
#          - sort_array_required_source_files
#          - start_logic
#          - usage
#          - usage_banner
#          - usage_normalized_arguments
#          - verify_dependencies
###------------------------------------------------------------------------------------------------

###------------------------------------------------------------------------------------------------
# Variables
SELF_EXECUTE_SITUATION=""

###------------------------------------------------------------------------------------------------
## FUNCTION: cleanup()
## - attempts to perform cleanup
## - removes temporary files as specified in array TEMPORARY_FILES
## - this will only remove files in a recognized temp directory, otherwise it issues a warning and skips the file
## - performs disk sync once finished
function cleanup() {
    log "Cleanup: Running"
    if (! is_empty "${PROCESSING_WORKING_FILE}"); then
        log "Cleanup: Detected incomplete processing/working file [${PROCESSING_WORKING_FILE}]. Removing file"
        > ${PROCESSING_WORKING_FILE}
        rm -f ${PROCESSING_WORKING_FILE}
    fi
    if (! is_empty "${TEMPORARY_FILES}"); then
        log "Cleanup: Removing generated temporary files"
        for ELEMENT in "${TEMPORARY_FILES[@]}"; do
            if [[ ${ELEMENT} == /tmp/* ]] || [[ ${ELEMENT} == ${DIRECTORY_TEMPORARY}/* ]]; then
                rm -f "${ELEMENT}"
                log_quiet "Cleanup: File removed [${ELEMENT}]"
            else
                log_warning "Cleanup: Could not remove generated temporary file for safety reasons, file is not located in a known temporary directory [${ELEMENT}]"
            fi
        done
    else
        log_quiet "Cleanup: No generated temporary file cleanup required"
    fi
    if (! is_empty "${TEMPORARY_DIRECTORIES}"); then
        log "Cleanup: Removing generated temporary directories"
        for ELEMENT in "${TEMPORARY_DIRECTORIES[@]}"; do
            if [[ ${ELEMENT} == /tmp/* ]] || [[ ${ELEMENT} == ${DIRECTORY_TEMPORARY}/* ]]; then
                rm -rf "${ELEMENT}"
                log "Cleanup: Directory removed [${ELEMENT}]"
            else
                log_warning "Cleanup: Could not remove generated temporary directory for safety reasons, file is not located in a known temporary directory [${ELEMENT}]"
            fi
        done
    else
        log_quiet "Cleanup: No generated temporary directory cleanup required"
    fi
    sync_disks
}

###------------------------------------------------------------------------------------------------
## FUNCTION: exec_script()
## - handles logic for calling a script from within a script (inception?)
## - returns the exit status of the script which was ran
## - Arguments
##   - $1: script to execute (use variables as supplied in reference_script_definitions.inc)
##   - $2: arguments to pass to script
function exec_script() {
    local FUNCTION_DESCRIPTION="Execute Script"
    log "${FUNCTION_DESCRIPTION}: Attempting to execute script (Called::${*})"
    if (is_empty "${1}"); then
        log_error "${FUNCTION_DESCRIPTION}: no script defined"
        return $E_MISSING_DEPENDENCIES
    fi
    if [ -f ${1} ]; then
        log "${FUNCTION_DESCRIPTION}: Script found (File::$($(which basename) "${1}") / Version::$(return_script_version "${1}"))"
    else
        log_error "${FUNCTION_DESCRIPTION}: Could not locate script [${1}]"
        return $E_OBJECT_NOT_FOUND
    fi
    sync_disks
    log "${FUNCTION_DESCRIPTION}: Running script [${*}]"
    export RUN_SCRIPT_AS_CHILD="yes"
    ${*}
    local RETURNVAL="$?"
    unset RUN_SCRIPT_AS_CHILD
    if [ ${RETURNVAL} -ne 0 ]; then
        log_error "${FUNCTION_DESCRIPTION}: Script failed to run (Script::$($(which basename) "${1}") / Return_Code::${RETURNVAL} [$(lookup_exit_code ${RETURNVAL})])"
        call_sleep 1
        return $RETURNVAL
    else
        log "${FUNCTION_DESCRIPTION}: Script returned success (Script::$($(which basename) "${1}") / Return_Code::${RETURNVAL})"
        call_sleep 1
        return $RETURNVAL
    fi
}

###------------------------------------------------------------------------------------------------
## FUNCTION: exit_logic()
## - handles exit logic
## - attempts to run all required cleanup
## - Arguments
##   - $1: Exit code to return
##   - $2: Exit message (optional)
function exit_logic() {
    local EXIT_CODE="${1}"
    local EXIT_MSG=""
    if (! is_empty "${2}"); then
        EXIT_MSG=" / Exit_Message::${2}"
    fi
    if (is_empty "${EXIT_CODE}"); then
        cleanup
        log "(Duration::$(time_elapsed ${RUNCODE}) / Exit_Code::0 [success]${EXIT_MSG})" SFIN "${LOG_COLOR_SUCCESS}"
        log "${RUNCODE}::${SELF_EXECUTE_SITUATION}::${SELF_IDENTITY}::0" SHLT
        GRACEFUL_EXIT=yes
        if option_enabled ENABLE_COLOR; then echo -ne "${COLOR_NONE}"; fi
        $(which stty) susp ^z > /dev/null 2>&1 # Re-enable CTRL+Z to suspend processes
        wait
        exit 0
    elif [ ${EXIT_CODE} -eq ${E_SUCCESS_ALT} ]; then
        cleanup
        log "(Duration::$(time_elapsed ${RUNCODE}) / Exit_Code::${EXIT_CODE} [$(lookup_exit_code ${EXIT_CODE})]${EXIT_MSG})" SFIN "${LOG_COLOR_SUCCESS}"
        log "${RUNCODE}::${SELF_EXECUTE_SITUATION}::\${SELF_IDENTITY}::${EXIT_CODE}" SHLT
        GRACEFUL_EXIT=yes
        if option_enabled ENABLE_COLOR; then echo -ne "${COLOR_NONE}"; fi
        $(which stty) susp ^z > /dev/null 2>&1 # Re-enable CTRL+Z to suspend processes
        wait
        exit ${EXIT_CODE}
    else
        log_error "Terminating script due to error"
        cleanup
        log "(Duration::$(time_elapsed ${RUNCODE}) / Exit_Code::${EXIT_CODE} [$(lookup_exit_code ${EXIT_CODE})]${EXIT_MSG})" SFIN "${LOG_COLOR_ERROR}"
        log "${RUNCODE}::${SELF_EXECUTE_SITUATION}::\${SELF_IDENTITY}::${EXIT_CODE}" SHLT
        GRACEFUL_EXIT=yes
        if option_enabled ENABLE_COLOR; then echo -ne "${COLOR_NONE}"; fi
        $(which stty) susp ^z > /dev/null 2>&1 # Re-enable CTRL+Z to suspend processes
        wait
        exit ${EXIT_CODE}
    fi
}

###------------------------------------------------------------------------------------------------
## FUNCTION: exit_logic_skip_cleanup()
## - handles exit logic
## - skips cleanup
## - Arguments
##   - $1: Exit code to return
##   - $2: Exit message (optional)
function exit_logic_skip_cleanup() {
    local EXIT_CODE="${1}"
    local EXIT_MSG=""
    if (! is_empty "${2}"); then
        EXIT_MSG=" / Exit Message: ${2}"
    fi
    if (is_empty "${EXIT_CODE}"); then
        log_warning "Cleanup: Skipping (exit conditions explicitly requested we skip cleanup)"
        log "(Duration::$(time_elapsed ${RUNCODE}) / Exit_Code::0 [success]${EXIT_MSG})" SFIN "${LOG_COLOR_SUCCESS}"
        log "${RUNCODE}::${SELF_EXECUTE_SITUATION}::\${SELF_IDENTITY}::0" SHLT
        GRACEFUL_EXIT=yes
        if option_enabled ENABLE_COLOR; then echo -ne "${COLOR_NONE}"; fi
        $(which stty) susp ^z > /dev/null 2>&1 # Re-enable CTRL+Z to suspend processes
        wait
        exit 0
    else
        log_error "Terminating script due to error"
        log_warning "Cleanup: Skipping (exit conditions explicitly requested we skip cleanup)"
        log "(Duration::$(time_elapsed ${RUNCODE}) / Exit_Code::${EXIT_CODE} [$(lookup_exit_code ${EXIT_CODE})]${EXIT_MSG})" SFIN "${LOG_COLOR_ERROR}"
        log "${RUNCODE}::${SELF_EXECUTE_SITUATION}::\${SELF_IDENTITY}::${EXIT_CODE}" SHLT
        GRACEFUL_EXIT=yes
        if option_enabled ENABLE_COLOR; then echo -ne "${COLOR_NONE}"; fi
        $(which stty) susp ^z > /dev/null 2>&1 # Re-enable CTRL+Z to suspend processes
        wait
        exit ${EXIT_CODE}
    fi
}

###------------------------------------------------------------------------------------------------
## FUNCTION: exit_trap()
## - handles exit logic when a trap is caught
## - attempts to run all required cleanup
## - Arguments
##   - $1: Exit code to return
##   - $2: Exit message (optional)
function exit_trap() {
    local EXIT_CODE="${1}"
    local EXIT_MSG=""
    if (! is_empty "${2}"); then
        EXIT_MSG=" / Exit Message: ${2}"
    fi
    log_error "Terminating script prematurely [caught trap]"
    cleanup
    log "(Duration::$(time_elapsed ${RUNCODE}) / Exit_Code::${EXIT_CODE} [$(lookup_exit_code ${EXIT_CODE})]${EXIT_MSG})" SFIN "${LOG_COLOR_ERROR}"
    log "${RUNCODE}::${SELF_EXECUTE_SITUATION}::\${SELF_IDENTITY}::${EXIT_CODE}" SHLT
    GRACEFUL_EXIT=yes
    if option_enabled ENABLE_COLOR; then echo -ne "${COLOR_NONE}"; fi
    $(which stty) susp ^z > /dev/null 2>&1 # Re-enable CTRL+Z to suspend processes
    wait
    exit ${EXIT_CODE}
}

###------------------------------------------------------------------------------------------------
## FUNCTION: exit_trap_ensure_graceful()
## - handles exit logic when trap EXIT is caught
## - attempts to run all required cleanup
## - this is a safety feature in case a graceful exit is not already defined in the script (ie: [exit_logic 0] not included as the last line of a script)
function exit_trap_ensure_graceful() {
    local CAUGHT_EXIT=$?
    if ! option_enabled GRACEFUL_EXIT; then
        exit_logic ${CAUGHT_EXIT}
    fi
}

###------------------------------------------------------------------------------------------------
## FUNCTION: lookup_exit_code()
## - Returns human-friendly string information based on exit code integer (will return 'UNKNOWN' if it is unable to parse exitcode from file)
## - Arguments
##   - $1: Exit Code
##   - $2: File containing exit codes, optional (if not supplied, we will use the default REFERENCE_EXIT_CODES
function lookup_exit_code() {
    local LOOKUP_CODE="${1}"
    local REFERENCE_FILE=""
    if (is_empty "${2}"); then
        REFERENCE_FILE="${REFERENCE_EXIT_CODES}"
    else
        REFERENCE_FILE="${2}"
    fi
    local STRING_EXIT="$(cat ${REFERENCE_FILE} | grep -v "^#" | sed 's/declare //g' | sed 's/ # /=/g'| grep "=${LOOKUP_CODE}" | awk -F"=" '{print $1}' | head -1)"
    local STRING_EXIT_DEFINITION="$(cat ${REFERENCE_FILE} | grep -v "^#" | sed 's/declare //g' | sed 's/ # /=/g'| grep "=${LOOKUP_CODE}" | awk -F"=" '{print $3}' | head -1)"
    if (is_empty "${STRING_EXIT}"); then
        echo "UNKNOWN"
    else
        if (is_empty "${STRING_EXIT_DEFINITION}"); then
            echo "${STRING_EXIT}"
        else
            echo "${STRING_EXIT} / ${STRING_EXIT_DEFINITION}"
        fi
    fi
}

###------------------------------------------------------------------------------------------------
## FUNCTION: return_script_purpose()
## - Returns version number of the selected script ($1)
## - This keys off of the '# purpose: ' comment line found at the top of the scripts
## - Normally called in self-reference [via: $(return_script_version "${0}")]
## - Arguments
##   - $1: File to inspect
function return_script_purpose() {
    local TARGET_FILE="${1}"
    if [ -f "${TARGET_FILE}.info" ]; then
        TARGET_FILE="${TARGET_FILE}.info"
    fi
    local TMP_STRING="$(return_header_string "purpose" "${TARGET_FILE}")"
    if(is_empty "${TMP_STRING}"); then
        TMP_STRING="unknown"
    fi
    echo -n "${TMP_STRING}" | sed -e 's/^ *//g' -e 's/ *$//g'
}

###------------------------------------------------------------------------------------------------
## FUNCTION: return_script_version()
## - Returns version number of the selected script ($1)
## - This keys off of the '# version: ' comment line found at the top of the scripts
## - Normally called in self-reference [via: $(return_script_version "${0}")]
## - Arguments
##   - $1: File to inspect
function return_script_version() {
    local TARGET_FILE="${1}"
    if [ -f "${TARGET_FILE}.info" ]; then
        TARGET_FILE="${TARGET_FILE}.info"
    fi
    local TMP_STRING="$(return_header_string "version" "${TARGET_FILE}")"
    if(is_empty "${TMP_STRING}"); then
        TMP_STRING="unknown"
    fi
    echo -n "${TMP_STRING}" | sed -e 's/^ *//g' -e 's/ *$//g'
}

###------------------------------------------------------------------------------------------------
## FUNCTION: sort_array_required_executeables()
## - sorts / removes duplicates in the REQUIRED_EXECUTABLES global array
function sort_array_required_executeables() {
    readarray -t REQUIRED_EXECUTABLES < <(for a in "${REQUIRED_EXECUTABLES[@]}"; do echo "${a}"; done | sort | uniq)
}

###------------------------------------------------------------------------------------------------
## FUNCTION: sort_array_required_source_files()
## - sorts / removes duplicates in the REQUIRED_SOURCE_FILES global array
function sort_array_required_source_files() {
    readarray -t REQUIRED_SOURCE_FILES < <(for a in "${REQUIRED_SOURCE_FILES[@]}"; do echo "${a}"; done | sort | uniq)
}

###------------------------------------------------------------------------------------------------
## FUNCTION: start_logic()
## - handles all script startup logic
## - reference code handling
## - log initialization
## - loads required source files (as defined in array REQUIRED_SOURCE_FILES)
## - verifies dependencies (as defined in array REQUIRED_EXECUTABLES)
## - should be called right after argument collection in every script
function start_logic() {
    if option_enabled ENABLE_COLOR; then
        if ! option_enabled RUN_SCRIPT_AS_CHILD; then
            log_load_color_parent
        else
            log_load_color_child
        fi
    fi

    log "Exec::${SELF_EXEC}" SSTR
    log "Runcode::${RUNCODE} / PID::$$" SSTR

    ## Traps
    trap "exit_trap $E_TRAP_SIGHUP" SIGHUP
    trap "exit_trap $E_TRAP_SIGINT" SIGINT
    trap "exit_trap $E_TRAP_SIGQUIT" SIGQUIT
    trap "exit_trap $E_TRAP_SIGTERM" SIGTERM
    trap "exit_trap $E_TRAP_SIGTSTP" SIGTSTP
    trap "exit_trap_ensure_graceful" EXIT

    ## Disable CTRL+Z
    ## - we cannot properly catch the SIGTSTP or SIGSTOP signals if it is sent with CTRL+Z, so we're going to disable it altogether
    ## - this will be re-enabled during exit routines
    $(which stty) susp ^- > /dev/null 2>&1

    sort_array_required_source_files
    if (! is_empty "${REQUIRED_SOURCE_FILES}"); then
        for ELEMENT in "${REQUIRED_SOURCE_FILES[@]}"; do
            source_include "${ELEMENT}"
        done
    fi
    verify_dependencies
}

###------------------------------------------------------------------------------------------------
## FUNCTION: usage()
## - Displays help / usage
function usage() {
    local TMP_ARG=""
cat << EOF
$(usage_banner "${0}")

ARGUMENTS:
EOF
    if [ ${#ARGUMENTS_DESCRIPTION[@]} -ne 0 ]; then
        for TMP_ARG in "${ARGUMENTS_DESCRIPTION[@]}"; do
            echo "  ${TMP_ARG}"
        done
        echo ""
    fi
cat << EOF
$(usage_normalized_arguments)

EOF
}

###------------------------------------------------------------------------------------------------
## FUNCTION: usage_banner()
## - Displays Usage Banner (Details, Version info, etc)
## - Arguments
##   - $1: File to inspect
function usage_banner() {
    local FILE_TO_INSPECT="${1}"
cat << EOF

USAGE:       $(basename ${FILE_TO_INSPECT}) <optional arguments>
DESCRIPTION: $(return_script_purpose "${FILE_TO_INSPECT}")
VERSION:     $(return_script_version "${FILE_TO_INSPECT}")
EOF
}

###------------------------------------------------------------------------------------------------
## FUNCTION: usage_normalized_arguments()
## - displays argument list which is common for all scripts
function usage_normalized_arguments() {
cat << EOF
  -h : Show help message
  -V : Show version number and exit
EOF
}

###------------------------------------------------------------------------------------------------
## FUNCTION: verify_dependencies()
## - validates that all required executables for the script to run are present (and accessible via PATH)
## - based on elements from REQUIRED_EXECUTABLES array
function verify_dependencies() {
    local IS_ERROR=0
    sort_array_required_executeables
    if (! is_empty "${REQUIRED_EXECUTABLES}"); then
        for ELEMENT in "${REQUIRED_EXECUTABLES[@]}"; do
            if ! which "${ELEMENT}" > /dev/null 2>&1; then
                log_error "Verify dependencies: Could not locate required executable: ${ELEMENT}"
                IS_ERROR=1
            fi
        done
        if [[ ${IS_ERROR} -eq 1 ]]; then
            exit_logic $E_MISSING_DEPENDENCIES "one or more executables could not be found"
        fi
    fi
    return 0
}
