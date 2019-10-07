#!/bin/bash
###------------------------------------------------------------------------------------------------
# alias:   aws_builder.utility.readme_generator
# script:  readme_generator.sh
# purpose: Scans AWS Builder application directory and generates an appropriate README.md
# version: 1.0.0
###------------------------------------------------------------------------------------------------

###------------------------------------------------------------------------------------------------
## Config
declare -r SELF_IDENTITY="readme_generator"
declare -r SELF_IDENTITY_H="README Generator"
declare -a ARGUMENTS_DESCRIPTION=(
    '-f <filename> : Output file (example: /tmp/source/README.md'
)

###------------------------------------------------------------------------------------------------
## Load Defaults
declare -r GLOBAL_CONFIG_FILE="${DIRECTORY_AWS_BUILDER:-/opt/aws_builder}/config/global.config"
source "${GLOBAL_CONFIG_FILE}" || exit 3

###------------------------------------------------------------------------------------------------
## Declare dependencies
REQUIRED_SOURCE_FILES+=(
    "${LIB_REFERENCE_DETAILS}"
)

###------------------------------------------------------------------------------------------------
## Variables
VARIABLES_REQUIRED=(
    'DETAILS_NAME'
    'DETAILS_NAME_ABBR'
    'FILE_README'
)

TMP_FILE_TOC=""
TMP_FILE_FUNCTIONS=""
TMP_FILE_SCRIPTS=""
TMP_FILE_BASE=""
TMP_FILE_BASE_ANCHOR=""
TMP_DIRECTORY_BASE=""
TMP_DIRECTORY_BASE_ANCHOR=""
TMP_ALIAS=""

###------------------------------------------------------------------------------------------------
## Main
# Process Arguments
while getopts "hVf:" OPTION; do
    case $OPTION in
        f) FILE_README=$OPTARG;;
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

if ! $(which touch) "${FILE_README}"  > /dev/null 2>&1; then
    TMP_ERROR_MSG="Cannot write to specified Output file [${FILE_README}]"
    log_error "${SELF_IDENTITY_H}: ${TMP_ERROR_MSG}"
    exit_logic $E_OBJECT_FAILED_TO_CREATE "${TMP_ERROR_MSG}"
fi

generate_temp_file TMP_FILE_TOC "Temp Dynamic Table of Contents file"
generate_temp_file TMP_FILE_FUNCTIONS "Temp functions file"
generate_temp_file TMP_FILE_SCRIPTS "Temp scripts file"

line_break
log "- Application Name: [${DETAILS_NAME}]"
log "- Application Abbr: [${DETAILS_NAME_ABBR}]"
log "- README.md:        [${FILE_README}]"
## END Initialize
###------------------------------------------------------------------------------------------------

###------------------------------------------------------------------------------------------------
## START Execution
line_break
log_highlight "Execution"

log_notice "${SELF_IDENTITY_H}: Processing Scripts"
echo "   * [Scripts](#scripts)" >> ${TMP_FILE_TOC}
echo "## Scripts" >> ${TMP_FILE_SCRIPTS}
echo "Available Scripts" >> ${TMP_FILE_SCRIPTS}
echo "> The following arguments are available to all scripts" >> ${TMP_FILE_SCRIPTS}
echo "" >> ${TMP_FILE_SCRIPTS}
echo "| Argument | Description |" >> ${TMP_FILE_SCRIPTS}
echo "| --- | --- |" >> ${TMP_FILE_SCRIPTS}
echo "| -h | Show help message" >> ${TMP_FILE_SCRIPTS}
echo "| -V | Show version number and exit" >> ${TMP_FILE_SCRIPTS}

for TMP_DIRECTORY in $(eval "find ${DIRECTORY_SCRIPTS}/ -type f -name '*.sh' 2>/dev/null | sed 's#\(.*\)/.*#\1#' | sort | uniq"); do
    TMP_DIRECTORY_BASE="${TMP_DIRECTORY/${DIRECTORY_SCRIPTS}\//}"
    TMP_DIRECTORY_BASE_ANCHOR="${TMP_DIRECTORY_BASE//.}"
    TMP_DIRECTORY_BASE_ANCHOR="${TMP_DIRECTORY_BASE//\/}"
    log "- [${TMP_DIRECTORY_BASE}]"
    echo "      * [${TMP_DIRECTORY_BASE}](#${TMP_DIRECTORY_BASE_ANCHOR})" >> ${TMP_FILE_TOC}
    echo "***" >> ${TMP_FILE_SCRIPTS}
    echo "### ${TMP_DIRECTORY_BASE}" >> ${TMP_FILE_SCRIPTS}

    for TMP_FILE in $(eval "find ${TMP_DIRECTORY} -type f -name '*.sh' 2>/dev/null | sort"); do
        TMP_FILE_BASE="$($(which basename) "${TMP_FILE}")"
        TMP_FILE_BASE_ANCHOR="${TMP_FILE_BASE//.}"
        TMP_FILE_BASE_ANCHOR="${TMP_FILE_BASE_ANCHOR//\/}"
        TMP_ALIAS=""
        log "-  - ${TMP_FILE_BASE}"
        echo "         * [${TMP_FILE_BASE}](#${TMP_FILE_BASE_ANCHOR})" >> ${TMP_FILE_TOC}
        echo "#### \`${TMP_FILE_BASE}\`" >> ${TMP_FILE_SCRIPTS}
        echo "$(return_script_purpose "${TMP_FILE}")" >> ${TMP_FILE_SCRIPTS}
        TMP_ARRAY="$(cat "${TMP_FILE}" | grep -v "^#" | grep -v "ARGUMENTS_DESCRIPTION=()" | sed -n '/^declare -a ARGUMENTS_DESCRIPTION=(/,/^)/p' | grep -v "^declare -a ARGUMENTS_DESCRIPTION" | grep -v "^)" | sed -e 's/^ *//g' -e 's/ *$//g' -e 's/^"//' -e 's/"$//' -e "s/^'\(.*\)'\$/\1/" 2>/dev/null)"
        if (! is_empty "${TMP_ARRAY}"); then
            echo "" >> ${TMP_FILE_SCRIPTS}
            echo "| Argument | Description |" >> ${TMP_FILE_SCRIPTS}
            echo "| --- | --- |" >> ${TMP_FILE_SCRIPTS}
            OLD_IFS="${IFS}"
            IFS=$'\n'
            TMP_ARRAY=(${TMP_ARRAY})
            IFS="${OLD_IFS}"
            for TMP_STRING in "${TMP_ARRAY[@]}"; do
                if (! is_empty "${TMP_STRING}"); then
                    TMP_STRING_1="$(echo "${TMP_STRING}" | awk -F: '{print $1}' | sed -e 's/</\&lt;/g' -e 's/>/\&gt;/g' -e 's/^ *//g' -e 's/ *$//g')"
                    TMP_STRING_2="$(echo "${TMP_STRING}" | awk -F: '{print $2}' | sed -e 's/</\&lt;/g' -e 's/>/\&gt;/g' -e 's/^ *//g' -e 's/ *$//g')"
                    echo "| ${TMP_STRING_1} | ${TMP_STRING_2}" >> ${TMP_FILE_SCRIPTS}
                fi
            done
        fi
        return_header_string "alias" "${TMP_FILE}" TMP_ALIAS
        echo "\`\`\`" >> ${TMP_FILE_SCRIPTS}
        echo "script: ${DIRECTORY_SCRIPTS}/${TMP_DIRECTORY_BASE}/${TMP_FILE_BASE} <arguments>" >> ${TMP_FILE_SCRIPTS}
        if (! is_empty "${TMP_ALIAS}"); then
            echo "alias:  ${TMP_ALIAS} <arguments>" >> ${TMP_FILE_SCRIPTS}
        fi
        echo "\`\`\`" >> ${TMP_FILE_SCRIPTS}
    done
done

log_notice "${SELF_IDENTITY_H}: Processing Functions"
echo "   * [Functions](#functions)" >> ${TMP_FILE_TOC}
echo "## Functions" >> ${TMP_FILE_FUNCTIONS}
echo "Available Functions" >> ${TMP_FILE_FUNCTIONS}
for TMP_FILE in $(eval "find ${DIRECTORY_LIB_FUNCTIONS} -type f -name '*.inc' 2>/dev/null | sort"); do
    TMP_FILE_BASE="${TMP_FILE/${DIRECTORY_LIB_FUNCTIONS}\//}"
    TMP_FILE_BASE_ANCHOR="${TMP_FILE_BASE//.}"
    TMP_FILE_BASE_ANCHOR="${TMP_FILE_BASE_ANCHOR//\/}"
    log "- [${TMP_FILE_BASE}]"
    echo "      * [${TMP_FILE_BASE}](#${TMP_FILE_BASE_ANCHOR})" >> ${TMP_FILE_TOC}
    echo "***" >> ${TMP_FILE_FUNCTIONS}
    echo "### ${TMP_FILE_BASE}" >> ${TMP_FILE_FUNCTIONS}
    echo "$(return_script_purpose "${TMP_FILE}")" >> ${TMP_FILE_FUNCTIONS}
    for TMP_FUNCTION in $(cat "${TMP_FILE}" | grep '## FUNCTION' | awk -F: '{print $2}' | sed -e 's/()//g' -e 's/^ *//;s/ *$//'); do
        log "-  - ${TMP_FUNCTION}"
        echo "         * [${TMP_FUNCTION}()](#${TMP_FUNCTION//.})" >> ${TMP_FILE_TOC}
    done
    cat "${TMP_FILE}" | grep "^##" | grep -v "^###" | sed -e 's/## FUNCTION: /\n#### `/g' -e 's/()$/()`/g' -e 's/## - Arguments/\n| Arg | Description |\n| --- | --- |/g' -e 's/##   - /| /g' -e 's/: / | /g' -e 's/## - /- /g' -e 's/</\&lt;/g' -e 's/>/\&gt;/g' >> ${TMP_FILE_FUNCTIONS}
done

log_notice "${SELF_IDENTITY_H}: Building README"
cat << ZZEOF > ${FILE_README}
# ${DETAILS_NAME}
$(for TMP_STRING in "${DETAILS_DESCRIPTION[@]}"; do echo "- ${TMP_STRING}"; done)
- Documentation Updated: $(date +"%Y-%m-%d %H:%M:%S %Z")

## Table of Contents
   * [Getting Started](#getting-started)
      * [Prerequisites](#prerequisites)
      * [Installing](#installing)
$(cat "${TMP_FILE_TOC}")
***

## Getting Started
These instructions describe how to get this project running on your machines for development and/or production use.

### Prerequisites
$(for TMP_STRING in "${DETAILS_PREREQUISITES[@]}"; do echo "- ${TMP_STRING}"; done)

### Dependencies
$(for TMP_STRING in "${DETAILS_DEPENDENCIES[@]}"; do echo "- ${TMP_STRING}"; done)

### Installing
Extract files
\`\`\`
mkdir -p ${DIRECTORY_BASE}
tar zxf ${DETAILS_NAME_ABBR}-latest.tgz -C ${DIRECTORY_BASE}
\`\`\`

Initialize Application
\`\`\`
bash ${SCRIPTS_UTILITY_INSTALL} -d /usr/local/sbin
\`\`\`

$(cat "${TMP_FILE_SCRIPTS}")

$(cat "${TMP_FILE_FUNCTIONS}")
ZZEOF
## END Execution
###------------------------------------------------------------------------------------------------

log_success "${SELF_IDENTITY_H}: Finished"
exit_logic 0
