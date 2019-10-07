#!/bin/bash
###------------------------------------------------------------------------------------------------
# alias:   aws_builder.utility.install
# script:  install.sh
# purpose: Installs AWS Builder Functionality
# version: 1.0.0
###------------------------------------------------------------------------------------------------

###------------------------------------------------------------------------------------------------
## Config
declare -r SELF_IDENTITY="install"
declare -r SELF_IDENTITY_H="Install"
declare -a ARGUMENTS_DESCRIPTION=(
    '-d <alias_links_directory> : Alias Links Directory (optional, if not specified, no aliases will be generated'
)

###------------------------------------------------------------------------------------------------
## Load Defaults
declare -r GLOBAL_CONFIG_FILE="${DIRECTORY_AWS_BUILDER:-/opt/aws_builder}/config/global.config"
source "${GLOBAL_CONFIG_FILE}" || exit 3

###------------------------------------------------------------------------------------------------
## Declare dependencies
REQUIRED_SOURCE_FILES+=()

###------------------------------------------------------------------------------------------------
## Variables
DIRECTORY_ALIAS_LINKS=""
TMP_ALIAS=""

###------------------------------------------------------------------------------------------------
## Main
# Process Arguments
while getopts "hVd:" OPTION; do
    case $OPTION in
        d) DIRECTORY_ALIAS_LINKS=$OPTARG;;
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

DIRECTORY_ALIAS_LINKS="$(echo "${DIRECTORY_ALIAS_LINKS}" | sed 's:/*$::')"
## END Initialize
###------------------------------------------------------------------------------------------------

###------------------------------------------------------------------------------------------------
## START Execution
line_break
log_highlight "Execution"

log_notice "${SELF_IDENTITY_H}: Setting Permissions"
find ${DIRECTORY_BASE} -type d -exec chmod 775 '{}' \;
find ${DIRECTORY_BASE} -type f -exec chmod 664 '{}' \;
find ${DIRECTORY_BASE}/scripts -type f -exec chmod 755 '{}' \;

if(! is_empty "${DIRECTORY_ALIAS_LINKS}"); then
    if [ -d "${DIRECTORY_ALIAS_LINKS}" ]; then
        log_notice "${SELF_IDENTITY_H}: Generating Alias Links [${DIRECTORY_ALIAS_LINKS}]"
        for TMP_FILE in $(eval "find ${DIRECTORY_BASE}/scripts -type f | sort 2>/dev/null"); do
            TMP_ALIAS=""
            return_header_string "alias" "${TMP_FILE}" TMP_ALIAS
            if(! is_empty "${TMP_ALIAS}"); then
                log "- [${TMP_ALIAS}] => [${TMP_FILE}]"
                ln -sf ${TMP_FILE} ${DIRECTORY_ALIAS_LINKS}/${TMP_ALIAS}
            fi
        done
    else
        TMP_ERROR_MSG="Alias Links directory does not exist [${DIRECTORY_ALIAS_LINKS}]"
        log_error "- ${TMP_ERROR_MSG}"
        exit_logic $E_OBJECT_NOT_FOUND "${TMP_ERROR_MSG}"
    fi
else
    log_warning "- Alias Links directory for symlinks not specified, skipping operation"
fi
## END Execution
###------------------------------------------------------------------------------------------------

log_success "${SELF_IDENTITY_H}: Finished"
exit_logic 0
