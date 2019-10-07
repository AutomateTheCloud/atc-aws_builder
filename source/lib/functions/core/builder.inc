###--------------------------------------------------------------------------------
# script:  builder.inc
# purpose: Collection of functions related to Builder
# version: 1.0.0
#
# function list:
#          - display_variables_object_lists
#          - display_variables_parameter
#          - display_variables_project
#          - generate_file_details
#          - generate_file_parameters
#          - generate_file_tags
#          - generate_file_targets
#          - generate_file_template
#          - load_object_files
#          - load_objects
#          - load_paradigm
#          - load_parameters
#          - load_project
#          - load_project_sam
#          - load_secrets
#          - load_targets
#          - load_templates
#          - manifest_get_checksum
#          - manifest_put_checksum
###------------------------------------------------------------------------------------------------
REQUIRED_EXECUTABLES+=()

###--------------------------------------------------------------------------------
# Variables
INDENT_JSON="  "
INDENT_YAML="  "

EXTENSION_PROJECT="project"
EXTENSION_SECRETS="secrets"
EXTENSION_DETAILS="details"
EXTENSION_TEMPLATE="template.yaml"
EXTENSION_PARAMETERS="parameters.json"
EXTENSION_TAGS="tags.json"
EXTENSION_TARGETS="targets"
EXTENSION_CHECKSUM="checksum"
EXTENSION_BUILDER_TEMPLATE="template"
EXTENSION_BUILDER_OBJECT="obj"

if [[ -z "${DIRECTORY_BUILDER_BASE}" ]]; then
    DIRECTORY_BUILDER_BASE="${DIRECTORY_LIB_DEPENDENCIES}/builder"
fi
if [ ! -d ${DIRECTORY_BUILDER_BASE} ]; then
    log_error "${FUNCTION_DESCRIPTION}: Builder Base directory does not exist [${DIRECTORY_BUILDER_BASE}]"
    exit_logic $E_MISSING_DEPENDENCIES
fi
DIRECTORY_BUILDER_OBJECTS_BASE="${DIRECTORY_BUILDER_BASE}/objects"
if [ ! -d ${DIRECTORY_BUILDER_OBJECTS_BASE} ]; then
    log_error "${FUNCTION_DESCRIPTION}: Builder Objects directory does not exist [${DIRECTORY_BUILDER_OBJECTS_BASE}]"
    exit_logic $E_MISSING_DEPENDENCIES
fi
DIRECTORY_BUILDER_TEMPLATES_BASE="${DIRECTORY_BUILDER_BASE}/templates"
if [ ! -d ${DIRECTORY_BUILDER_TEMPLATES_BASE} ]; then
    log_error "${FUNCTION_DESCRIPTION}: Builder Templates directory does not exist [${DIRECTORY_BUILDER_TEMPLATES_BASE}]"
    exit_logic $E_MISSING_DEPENDENCIES
fi

DIRECTORY_BUILDER_PROJECT_BASE=""
DIRECTORY_BUILDER_PROJECT_OBJECTS=""
DIRECTORY_TRACKING_SECRETS_BASE=""
DIRECTORY_TRACKING_BUILDER_BASE=""
DIRECTORY_TRACKING_CLOUDFORMATION_BASE=""

FILE_BUILDER_PROJECT=""
FILENAME_BUILDER_PROJECT=""
BUILDER_PROJECT_NAME=""

declare -a ARRAY_OBJECTS_PARAMETERS=()
declare -a ARRAY_OBJECTS_MAPPINGS=()
declare -a ARRAY_OBJECTS_CONDITIONS=()
declare -a ARRAY_OBJECTS_RESOURCES=()
declare -a ARRAY_OBJECTS_OUTPUTS=()
declare -a ARRAY_OBJECTS_TARGETS=()
declare -a ARRAY_OBJECTS_TEMPLATES=()

declare -a ARRAY_OBJECTS_FILES_PARAMETERS=()
declare -a ARRAY_OBJECTS_FILES_MAPPINGS=()
declare -a ARRAY_OBJECTS_FILES_CONDITIONS=()
declare -a ARRAY_OBJECTS_FILES_RESOURCES=()
declare -a ARRAY_OBJECTS_FILES_OUTPUTS=()
declare -a ARRAY_OBJECTS_FILES_TARGETS=()
declare -a ARRAY_OBJECTS_FILES_TEMPLATES=()

declare -a ARRAY_OBJECTS_LOADED_PARAMETERS=()
declare -a ARRAY_OBJECTS_LOADED_MAPPINGS=()
declare -a ARRAY_OBJECTS_LOADED_CONDITIONS=()
declare -a ARRAY_OBJECTS_LOADED_RESOURCES=()
declare -a ARRAY_OBJECTS_LOADED_OUTPUTS=()
declare -a ARRAY_OBJECTS_LOADED_TARGETS=()

declare -a ARRAY_TARGETS_RESOURCES=()

declare -a ARRAY_ERROR_LOG=()

SECRETS_VARIABLES_REQUIRED+=(
    'project_name'
    'project_abbr'
    'function_name'
    'function_abbr'
    'region'
    'environment'
    'account_number'
    'account_name'
    'account_abbr'
    'profile'
    'stack_reference_vpc'
    'last_update'
)

###--------------------------------------------------------------------------------
## FUNCTION: display_variables_object_lists()
## - Displays Object Lists variables
function display_variables_object_lists() {
    local TMP_OBJECT=""
    line_break
    log_highlight "Object List: Templates"
    for TMP_OBJECT in "${ARRAY_OBJECTS_TEMPLATES[@]}"; do
        log "- [${TMP_OBJECT}]"
    done
    line_break
    log_highlight "Object List: Parameters"
    for TMP_OBJECT in "${ARRAY_OBJECTS_PARAMETERS[@]}"; do
        log "- [${TMP_OBJECT}]"
    done
    line_break
    log_highlight "Object List: Mappings"
    for TMP_OBJECT in "${ARRAY_OBJECTS_MAPPINGS[@]}"; do
        log "- [${TMP_OBJECT}]"
    done
    line_break
    log_highlight "Object List: Conditions"
    for TMP_OBJECT in "${ARRAY_OBJECTS_CONDITIONS[@]}"; do
        log "- [${TMP_OBJECT}]"
    done
    line_break
    log_highlight "Object List: Resources"
    for TMP_OBJECT in "${ARRAY_OBJECTS_RESOURCES[@]}"; do
        log "- [${TMP_OBJECT}]"
    done
    line_break
    log_highlight "Object List: Outputs"
    for TMP_OBJECT in "${ARRAY_OBJECTS_OUTPUTS[@]}"; do
        log "- [${TMP_OBJECT}]"
    done
    log_highlight "Object List: Targets"
    for TMP_OBJECT in "${ARRAY_OBJECTS_TARGETS[@]}"; do
        log "- [${TMP_OBJECT}]"
    done
    line_break
}

###--------------------------------------------------------------------------------
## FUNCTION: display_variables_parameter()
## - Displays Parameter variables
function display_variables_parameter() {
    local TMP_KEY=""
    local TMP_VALUE=""
    local KEY_MAX_LENGTH=0
    line_break
    log_highlight "Loaded Parameters"

    for TMP_KEY in "${ARRAY_OBJECTS_LOADED_PARAMETERS[@]}"; do
        if [ ${#TMP_KEY} -gt $KEY_MAX_LENGTH ]; then
            KEY_MAX_LENGTH=${#TMP_KEY}
        fi
    done
    KEY_MAX_LENGTH=$((${KEY_MAX_LENGTH}+1))

    for TMP_KEY in "${ARRAY_OBJECTS_LOADED_PARAMETERS[@]}"; do
        TMP_VAR="$(to_upper "REF_PARAMETER_${TMP_KEY}")"
        log "$(printf "%-1s %-${KEY_MAX_LENGTH}s %s" "-" "${TMP_KEY}:" "[${!TMP_VAR}]")"
    done
    line_break
}

###--------------------------------------------------------------------------------
## FUNCTION: display_variables_project()
## - Displays Project variables
function display_variables_project() {
    line_break
    log_highlight "Project"
    log "- Project Name:      [${BUILDER_PROJECT_NAME}]"
    log "- Project Directory: [${DIRECTORY_BUILDER_PROJECT_BASE}]"
    line_break
}

###--------------------------------------------------------------------------------
## FUNCTION: generate_file_details()
## - Generates Details file
## - Arguments
##   - $1: Output file
function generate_file_details() {
    local FUNCTION_DESCRIPTION="Generate File (Details)"
    local TMP_OUTPUT_FILE="${1}"
    if ! $(which touch) ${TMP_OUTPUT_FILE} >/dev/null 2>&1; then
        log_error "${FUNCTION_DESCRIPTION}: Output file cannot be created [${TMP_OUTPUT_FILE}]"
        return $E_OBJECT_FAILED_TO_CREATE
    fi

    log_notice "${FUNCTION_DESCRIPTION}: generating"
cat > ${TMP_OUTPUT_FILE} << ZZEOF
[stack_name]=${DETAILS_STACK_NAME}
[stack_description]=${DETAILS_STACK_DESCRIPTION}
[deployment_timeout]=${DETAILS_DEPLOYMENT_TIMEOUT}
[account_name]=${DETAILS_ACCOUNT_NAME}
[account_abbr]=${DETAILS_ACCOUNT_ABBR}
[account_number]=${DETAILS_ACCOUNT_NUMBER}
[organization_name]=${DETAILS_ORGANIZATION_NAME}
[organization_abbr]=${DETAILS_ORGANIZATION_ABBR}
[region]=${DETAILS_REGION}
[environment]=${DETAILS_ENVIRONMENT}
[project_name]=${DETAILS_PROJECT_NAME}
[project_abbr]=${DETAILS_PROJECT_ABBR}
[function_name]=${DETAILS_FUNCTION_NAME}
[function_abbr]=${DETAILS_FUNCTION_ABBR}
[owner]=${DETAILS_OWNER}
[contact]=${DETAILS_CONTACT}
ZZEOF
    return 0
}

###--------------------------------------------------------------------------------
## FUNCTION: generate_file_parameters()
## - Generates Parameters file
## - Arguments
##   - $1: Output file
function generate_file_parameters() {
    local FUNCTION_DESCRIPTION="Generate File (Parameters)"
    local TMP_OUTPUT_FILE="${1}"

    local RETURNVAL=""
    local TMP_OBJECT=""
    local TMP_PARAMETER=""
    local TMP_VALUE=""
    local TMP_FILE_PARAMETERS=""

    if ! $(which touch) ${TMP_OUTPUT_FILE} >/dev/null 2>&1; then
        log_error "${FUNCTION_DESCRIPTION}: Output file cannot be created [${TMP_OUTPUT_FILE}]"
        return $E_OBJECT_FAILED_TO_CREATE
    fi

    generate_temp_file DISCARDABLE_VARIABLE "placeholder response file"
    TMP_FILE_PARAMETERS="${DISCARDABLE_VARIABLE}"

    log_notice "${FUNCTION_DESCRIPTION}: generating"
    > ${TMP_FILE_PARAMETERS}
    echo "[" >> ${TMP_FILE_PARAMETERS}
    for TMP_PARAMETER in "${ARRAY_OBJECTS_LOADED_PARAMETERS[@]}"; do
        TMP_VALUE="$(eval echo \${$(to_upper "REF_PARAMETER_${TMP_PARAMETER}")})"
        echo "${INDENT_JSON}{ \"ParameterKey\": \"${TMP_PARAMETER}\", \"ParameterValue\": \"${TMP_VALUE}\" }," >> ${TMP_FILE_PARAMETERS}
    done
    echo "]" >> ${TMP_FILE_PARAMETERS}

    # This little abomination snippet for sed is to remove the very last comma from a file
    cat "${TMP_FILE_PARAMETERS}" | sed -e '1h;1!H;$!d;${s/.*//;x};s/\(.*\),/\1 /' > ${TMP_OUTPUT_FILE}
    return 0
}

###--------------------------------------------------------------------------------
## FUNCTION: generate_file_tags()
## - Generates Tags file
## - Arguments
##   - $1: Output file
function generate_file_tags() {
    local FUNCTION_DESCRIPTION="Generate File (Tags)"
    local TMP_OUTPUT_FILE="${1}"

    local RETURNVAL=""
    local TMP_OBJECT=""
    local TMP_KEY=""
    local TMP_VALUE=""

    if ! $(which touch) ${TMP_OUTPUT_FILE} >/dev/null 2>&1; then
        log_error "${FUNCTION_DESCRIPTION}: Output file cannot be created [${TMP_OUTPUT_FILE}]"
        return $E_OBJECT_FAILED_TO_CREATE
    fi

    log_notice "${FUNCTION_DESCRIPTION}: generating"
cat > ${TMP_OUTPUT_FILE} << ZZEOF
[
  { "Key": "Organization", "Value": "${DETAILS_ORGANIZATION_NAME}" },
  { "Key": "Project", "Value": "${DETAILS_PROJECT_NAME}" },
  { "Key": "Function", "Value": "${DETAILS_FUNCTION_NAME}" },
  { "Key": "Environment", "Value": "${DETAILS_ENVIRONMENT}" },
  { "Key": "Region", "Value": "${DETAILS_REGION}" },
  { "Key": "Owner", "Value": "${DETAILS_OWNER}" },
  { "Key": "Contact", "Value": "${DETAILS_CONTACT}" }
]
ZZEOF
    return 0
}

###--------------------------------------------------------------------------------
## FUNCTION: generate_file_targets()
## - Generates CloudFormation Targets file
## - Arguments
##   - $1: Output file
function generate_file_targets() {
    local FUNCTION_DESCRIPTION="Generate File (CloudFormation Targets)"
    local TMP_OUTPUT_FILE="${1}"

    local RETURNVAL=""
    local TMP_OBJECT_FILE=""
    local TMP_FILE_TEMPLATE=""

    if ! $(which touch) ${TMP_OUTPUT_FILE} >/dev/null 2>&1; then
        log_error "${FUNCTION_DESCRIPTION}: Output file cannot be created [${TMP_OUTPUT_FILE}]"
        return $E_OBJECT_FAILED_TO_CREATE
    fi

    log_notice "${FUNCTION_DESCRIPTION}: generating"
    if [ ${#ARRAY_OBJECTS_FILES_TARGETS[@]} -gt 0 ]; then
        for TMP_OBJECT_FILE in "${ARRAY_OBJECTS_FILES_TARGETS[@]}"; do
            if(! is_empty "${TMP_OBJECT_FILE}"); then
                cat "${TMP_OBJECT_FILE}" | grep -v "^#\|^;" | sed -e 's/[ \t]*$//' -e '/^$/d' -e '$ s/,$//g' -e 's/[ \t]*$//' -e '/^$/d' >> ${TMP_OUTPUT_FILE}
            fi
        done
    fi
    return 0
}

###--------------------------------------------------------------------------------
## FUNCTION: generate_file_template()
## - Generates CloudFormation Template file
## - Arguments
##   - $1: Output file
function generate_file_template() {
    local FUNCTION_DESCRIPTION="Generate File (CloudFormation Template)"
    local TMP_OUTPUT_FILE="${1}"

    local RETURNVAL=""
    local TMP_OBJECT_FILE=""
    local TMP_FILE_TEMPLATE=""

    if ! $(which touch) ${TMP_OUTPUT_FILE} >/dev/null 2>&1; then
        log_error "${FUNCTION_DESCRIPTION}: Output file cannot be created [${TMP_OUTPUT_FILE}]"
        return $E_OBJECT_FAILED_TO_CREATE
    fi

    generate_temp_file DISCARDABLE_VARIABLE "placeholder response file"
    TMP_FILE_TEMPLATE="${DISCARDABLE_VARIABLE}"

    log_notice "${FUNCTION_DESCRIPTION}: generating"
    > ${TMP_FILE_TEMPLATE}
    log_notice "- Template Header"
    echo "AWSTemplateFormatVersion: \"2010-09-09\"" >> ${TMP_FILE_TEMPLATE}
    echo "Description: \"${DETAILS_STACK_DESCRIPTION}\"" >> ${TMP_FILE_TEMPLATE}

    log_notice "- Parameters"
    if [ ${#ARRAY_OBJECTS_FILES_PARAMETERS[@]} -gt 0 ]; then
        echo "##-------------------------------------------------------------------------------------------------" >> ${TMP_FILE_TEMPLATE}
        echo "Parameters:" >> ${TMP_FILE_TEMPLATE}
        for TMP_OBJECT_FILE in "${ARRAY_OBJECTS_FILES_PARAMETERS[@]}"; do
            cat "${TMP_OBJECT_FILE}" | sed -e 's/[ \t]*$//' -e '/^$/d' -e '$ s/,$//g' -e 's/[ \t]*$//' -e '/^$/d' -e "s/^/${INDENT_YAML}/" >> ${TMP_FILE_TEMPLATE}; echo "" >> ${TMP_FILE_TEMPLATE}
        done
    fi

    log_notice "- Mappings"
    if [ ${#ARRAY_OBJECTS_FILES_MAPPINGS[@]} -gt 0 ]; then
        echo "##-------------------------------------------------------------------------------------------------" >> ${TMP_FILE_TEMPLATE}
        echo "Mappings:" >> ${TMP_FILE_TEMPLATE}
        for TMP_OBJECT_FILE in "${ARRAY_OBJECTS_FILES_MAPPINGS[@]}"; do
            cat "${TMP_OBJECT_FILE}" | sed -e 's/[ \t]*$//' -e '/^$/d' -e '$ s/,$//g' -e 's/[ \t]*$//' -e '/^$/d' -e "s/^/${INDENT_YAML}/" >> ${TMP_FILE_TEMPLATE}; echo "" >> ${TMP_FILE_TEMPLATE}
        done
    fi

    log_notice "- Conditions"
    if [ ${#ARRAY_OBJECTS_FILES_CONDITIONS[@]} -gt 0 ]; then
        echo "##-------------------------------------------------------------------------------------------------" >> ${TMP_FILE_TEMPLATE}
        echo "Conditions:" >> ${TMP_FILE_TEMPLATE}
        for TMP_OBJECT_FILE in "${ARRAY_OBJECTS_FILES_CONDITIONS[@]}"; do
            cat "${TMP_OBJECT_FILE}" | sed -e 's/[ \t]*$//' -e '/^$/d' -e '$ s/,$//g' -e 's/[ \t]*$//' -e '/^$/d' -e "s/^/${INDENT_YAML}/" >> ${TMP_FILE_TEMPLATE}; echo "" >> ${TMP_FILE_TEMPLATE}
        done
    fi

    log_notice "- Resources"
    if [ ${#ARRAY_OBJECTS_FILES_RESOURCES[@]} -gt 0 ]; then
        echo "##-------------------------------------------------------------------------------------------------" >> ${TMP_FILE_TEMPLATE}
        echo "Resources:" >> ${TMP_FILE_TEMPLATE}
        for TMP_OBJECT_FILE in "${ARRAY_OBJECTS_FILES_RESOURCES[@]}"; do
            cat "${TMP_OBJECT_FILE}" | sed -e 's/[ \t]*$//' -e '/^$/d' -e '$ s/,$//g' -e 's/[ \t]*$//' -e '/^$/d' -e "s/^/${INDENT_YAML}/" >> ${TMP_FILE_TEMPLATE}; echo "" >> ${TMP_FILE_TEMPLATE}
        done
    fi

    log_notice "- Outputs"
    if [ ${#ARRAY_OBJECTS_FILES_OUTPUTS[@]} -gt 0 ]; then
        echo "##-------------------------------------------------------------------------------------------------" >> ${TMP_FILE_TEMPLATE}
        echo "Outputs:" >> ${TMP_FILE_TEMPLATE}
        for TMP_OBJECT_FILE in "${ARRAY_OBJECTS_FILES_OUTPUTS[@]}"; do
            cat "${TMP_OBJECT_FILE}" | sed -e 's/[ \t]*$//' -e '/^$/d' -e '$ s/,$//g' -e 's/[ \t]*$//' -e '/^$/d' -e "s/^/${INDENT_YAML}/" >> ${TMP_FILE_TEMPLATE}; echo "" >> ${TMP_FILE_TEMPLATE}
        done
    fi

    cat "${TMP_FILE_TEMPLATE}" > ${TMP_OUTPUT_FILE}
    return 0
}

###--------------------------------------------------------------------------------
## FUNCTION: load_object_files()
## - Loads Files from ARRAY_OBJECTS_<type> into Corresponding Files Array
## - Arguments
##   - $1: Object Type (conditions / mappings / parameters / outputs / resources)
function load_object_files() {
    local FUNCTION_DESCRIPTION="Load Objects Info"
    local TMP_OBJECT_TYPE="${1}"

    local FUNCTION_RETURNVAL=0
    local TMP_OBJECT=""
    local TMP_OBJECT_CATEGORY=""
    local TMP_OBJECT_NAME=""
    local TMP_OBJECT_DIRECTORY_BASE=""
    local TMP_OBJECT_FILE=""

    if(is_empty "${TMP_OBJECT_TYPE}"); then
        ARRAY_ERROR_LOG+=("${FUNCTION_DESCRIPTION}: Object Type not specified")
        FUNCTION_RETURNVAL=$E_BAD_ARGS
    fi
    if [ ${FUNCTION_RETURNVAL} -ne 0 ]; then return ${FUNCTION_RETURNVAL}; fi

    for TMP_OBJECT in $(eval echo \${$(to_upper "ARRAY_OBJECTS_${TMP_OBJECT_TYPE}")[@]}); do
        TMP_OBJECT_CATEGORY="$($(which cut) -d '/' -f 1 <<< "${TMP_OBJECT}" | sed -e 's/^ *//g' -e 's/ *$//g')"
        TMP_OBJECT_NAME="$($(which cut) -d '/' -f 2- <<< "${TMP_OBJECT}" | sed -e 's/^ *//g' -e 's/ *$//g')"
        if [[ "ZZ_$(to_upper "${TMP_OBJECT_CATEGORY}")" == "ZZ_BASE" ]]; then
            TMP_OBJECT_DIRECTORY_BASE="${DIRECTORY_BUILDER_OBJECTS_BASE}"
        else
            TMP_OBJECT_DIRECTORY_BASE="${DIRECTORY_BUILDER_PROJECT_OBJECTS}"
        fi
        TMP_OBJECT_FILE="${TMP_OBJECT_DIRECTORY_BASE}/${TMP_OBJECT_TYPE}/${TMP_OBJECT_NAME}.${EXTENSION_BUILDER_OBJECT}"
        if [ ! -f "${TMP_OBJECT_FILE}" ]; then
            ARRAY_ERROR_LOG+=("${FUNCTION_DESCRIPTION} (${TMP_OBJECT_TYPE}): File not found [${TMP_OBJECT_FILE}]")
            FUNCTION_RETURNVAL=$E_OBJECT_NOT_FOUND
        fi
        add_element_to_array "$(to_upper "ARRAY_OBJECTS_FILES_${TMP_OBJECT_TYPE}")" "${TMP_OBJECT_FILE}"
    done
    return $FUNCTION_RETURNVAL
}

###--------------------------------------------------------------------------------
## FUNCTION: load_objects()
## - Loads Object Names into Corresponding Objects Array
## - Arguments
##   - $1: Object Type (conditions / mappings / parameters / outputs / resources)
##   - $2: Object Source File
function load_objects() {
    local FUNCTION_DESCRIPTION="Load Objects"
    local TMP_OBJECT_TYPE="${1}"
    local TMP_FILE_OBJECT_SOURCE="${2}"

    local FUNCTION_RETURNVAL=0
    local TMP_OBJECT=""
    local TMP_OBJECT_NAME=""

    if(is_empty "${TMP_OBJECT_TYPE}"); then
        ARRAY_ERROR_LOG+=("${FUNCTION_DESCRIPTION}: Object Type not specified")
        FUNCTION_RETURNVAL=$E_BAD_ARGS
    fi
    if(is_empty "${TMP_FILE_OBJECT_SOURCE}"); then
        ARRAY_ERROR_LOG+=("${FUNCTION_DESCRIPTION} (${TMP_OBJECT_TYPE}): Object source file not specified")
        FUNCTION_RETURNVAL=$E_BAD_ARGS
    fi
    if [ ! -f "${TMP_FILE_OBJECT_SOURCE}" ]; then
        ARRAY_ERROR_LOG+=("${FUNCTION_DESCRIPTION} (${TMP_OBJECT_TYPE}): Object source file not found [${TMP_FILE_OBJECT_SOURCE}]")
        FUNCTION_RETURNVAL=$E_OBJECT_NOT_FOUND
    fi
    if [ ${FUNCTION_RETURNVAL} -ne 0 ]; then return ${FUNCTION_RETURNVAL}; fi
    local TMP_ARRAY=($(cat "${TMP_FILE_OBJECT_SOURCE}" 2>/dev/null | grep -v "#" | awk "/^${INDENT_YAML}${TMP_OBJECT_TYPE}:/ {flag=1;next} /^${INDENT_YAML}[A-Za-z]/{flag=0} flag {print}" | sed -e 's/- //g' -e 's/^ *//g' -e 's/ *$//g' | sed -e 's/^ *//g' -e 's/ *$//g'))
    for TMP_OBJECT in "${TMP_ARRAY[@]}"; do
        TMP_OBJECT_NAME="$($(which cut) -d '/' -f 2- <<< "${TMP_OBJECT}" | sed -e 's/^ *//g' -e 's/ *$//g')"
        add_element_to_array "$(to_upper "ARRAY_OBJECTS_${TMP_OBJECT_TYPE}")" "${TMP_OBJECT}"
        add_element_to_array "$(to_upper "ARRAY_OBJECTS_LOADED_${TMP_OBJECT_TYPE}")" "${TMP_OBJECT_NAME}"
    done
    return $FUNCTION_RETURNVAL
}

###--------------------------------------------------------------------------------
## FUNCTION: load_objects_base()
## - Loads Object Names into Corresponding Objects Array (using basename of object)
## - Arguments
##   - $1: Object Type (parameters)
##   - $2: Object Source File
function load_objects_base() {
    local FUNCTION_DESCRIPTION="Load Objects"
    local TMP_OBJECT_TYPE="${1}"
    local TMP_FILE_OBJECT_SOURCE="${2}"

    local FUNCTION_RETURNVAL=0
    local TMP_OBJECT=""
    local TMP_OBJECT_NAME=""

    if(is_empty "${TMP_OBJECT_TYPE}"); then
        ARRAY_ERROR_LOG+=("${FUNCTION_DESCRIPTION}: Object Type not specified")
        FUNCTION_RETURNVAL=$E_BAD_ARGS
    fi
    if(is_empty "${TMP_FILE_OBJECT_SOURCE}"); then
        ARRAY_ERROR_LOG+=("${FUNCTION_DESCRIPTION} (${TMP_OBJECT_TYPE}): Object source file not specified")
        FUNCTION_RETURNVAL=$E_BAD_ARGS
    fi
    if [ ! -f "${TMP_FILE_OBJECT_SOURCE}" ]; then
        ARRAY_ERROR_LOG+=("${FUNCTION_DESCRIPTION} (${TMP_OBJECT_TYPE}): Object source file not found [${TMP_FILE_OBJECT_SOURCE}]")
        FUNCTION_RETURNVAL=$E_OBJECT_NOT_FOUND
    fi
    if [ ${FUNCTION_RETURNVAL} -ne 0 ]; then return ${FUNCTION_RETURNVAL}; fi
    local TMP_ARRAY=($(cat "${TMP_FILE_OBJECT_SOURCE}" 2>/dev/null | grep -v "#" | awk "/^${INDENT_YAML}${TMP_OBJECT_TYPE}:/ {flag=1;next} /^${INDENT_YAML}[A-Za-z]/{flag=0} flag {print}" | sed -e 's/- //g' -e 's/^ *//g' -e 's/ *$//g' | sed -e 's/^ *//g' -e 's/ *$//g'))
    for TMP_OBJECT in "${TMP_ARRAY[@]}"; do
        TMP_OBJECT_NAME="${TMP_OBJECT##*/}"
        add_element_to_array "$(to_upper "ARRAY_OBJECTS_${TMP_OBJECT_TYPE}")" "${TMP_OBJECT}"
        add_element_to_array "$(to_upper "ARRAY_OBJECTS_LOADED_${TMP_OBJECT_TYPE}")" "${TMP_OBJECT_NAME}"
    done
    return $FUNCTION_RETURNVAL
}

###--------------------------------------------------------------------------------
## FUNCTION: load_paradigm()
## - Loads Paradigm Model into memory
function load_paradigm() {
    local FUNCTION_DESCRIPTION="Load Paradigm"
    local TMP_TEMPLATE=""
    local TMP_FILE_TEMPLATE_SOURCE=""
    local RETURNVAL=""
    local PROGRESS_RETURNVAL=0
    local TMP_ERROR=""

    log_notice "${FUNCTION_DESCRIPTION}:"
    log_notice "- Loading Templates"
    load_templates "${FILE_BUILDER_PROJECT}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        for TMP_ERROR in "${ARRAY_ERROR_LOG[@]}"; do
            log_error "- ${TMP_ERROR}"
        done
        return $RETURNVAL
    fi
    readarray -t ARRAY_OBJECTS_TEMPLATES < <(for a in "${ARRAY_OBJECTS_TEMPLATES[@]}"; do echo "${a}"; done | grep -v '^$' | sort | uniq)
    readarray -t ARRAY_OBJECTS_FILES_TEMPLATES < <(for a in "${ARRAY_OBJECTS_FILES_TEMPLATES[@]}"; do echo "${a}"; done | grep -v '^$' | sort | uniq)

    log_notice "- Loading Objects"
    load_objects_base "Parameters" "${FILE_BUILDER_PROJECT}"; RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then PROGRESS_RETURNVAL=${RETURNVAL}; fi
    load_objects "Mappings" "${FILE_BUILDER_PROJECT}";   RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then PROGRESS_RETURNVAL=${RETURNVAL}; fi
    load_objects "Conditions" "${FILE_BUILDER_PROJECT}"; RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then PROGRESS_RETURNVAL=${RETURNVAL}; fi
    load_objects "Resources" "${FILE_BUILDER_PROJECT}";  RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then PROGRESS_RETURNVAL=${RETURNVAL}; fi
    load_objects "Outputs" "${FILE_BUILDER_PROJECT}";    RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then PROGRESS_RETURNVAL=${RETURNVAL}; fi
    load_objects "Targets" "${FILE_BUILDER_PROJECT}";    RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then PROGRESS_RETURNVAL=${RETURNVAL}; fi
    for TMP_FILE_TEMPLATE_SOURCE in "${ARRAY_OBJECTS_FILES_TEMPLATES[@]}"; do
        load_objects_base "Parameters" "${TMP_FILE_TEMPLATE_SOURCE}"; RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then PROGRESS_RETURNVAL=${RETURNVAL}; fi
        load_objects "Mappings" "${TMP_FILE_TEMPLATE_SOURCE}";   RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then PROGRESS_RETURNVAL=${RETURNVAL}; fi
        load_objects "Conditions" "${TMP_FILE_TEMPLATE_SOURCE}"; RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then PROGRESS_RETURNVAL=${RETURNVAL}; fi
        load_objects "Resources" "${TMP_FILE_TEMPLATE_SOURCE}";  RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then PROGRESS_RETURNVAL=${RETURNVAL}; fi
        load_objects "Outputs" "${TMP_FILE_TEMPLATE_SOURCE}";    RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then PROGRESS_RETURNVAL=${RETURNVAL}; fi
        load_objects "Targets" "${TMP_FILE_TEMPLATE_SOURCE}";    RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then PROGRESS_RETURNVAL=${RETURNVAL}; fi
    done
    ARRAY_OBJECTS_PARAMETERS+=(
        "Base/Details/Account"
        "Base/Details/Organization"
        "Base/Details/Project"
        "Base/Details/Function"
        "Base/Details/Owner"
        "Base/Details/LastUpdate"
    )
    ARRAY_OBJECTS_LOADED_PARAMETERS+=(
        "Account"
        "Organization"
        "Project"
        "Function"
        "Owner"
        "LastUpdate"
    )
    ARRAY_OBJECTS_OUTPUTS+=(
        "Base/Details/OrganizationAbbr"
        "Base/Details/OrganizationName"
        "Base/Details/ProjectAbbr"
        "Base/Details/ProjectName"
        "Base/Details/FunctionAbbr"
        "Base/Details/FunctionName"
        "Base/Details/Owner"
        "Base/Details/Contact"
        "Base/Details/Environment"
        "Base/Details/LastUpdate"
    )
    ARRAY_OBJECTS_LOADED_OUTPUTS+=(
        "OrganizationAbbr"
        "OrganizationName"
        "ProjectAbbr"
        "ProjectName"
        "FunctionAbbr"
        "FunctionName"
        "Owner"
        "Contact"
        "Environment"
        "LastUpdate"
    )

    readarray -t ARRAY_OBJECTS_PARAMETERS < <(for a in "${ARRAY_OBJECTS_PARAMETERS[@]}"; do echo "${a}"; done | grep -v '^$' | sort | uniq)
    readarray -t ARRAY_OBJECTS_MAPPINGS < <(for a in "${ARRAY_OBJECTS_MAPPINGS[@]}"; do echo "${a}"; done | grep -v '^$' | sort | uniq)
    readarray -t ARRAY_OBJECTS_CONDITIONS < <(for a in "${ARRAY_OBJECTS_CONDITIONS[@]}"; do echo "${a}"; done | grep -v '^$' | sort | uniq)
    readarray -t ARRAY_OBJECTS_RESOURCES < <(for a in "${ARRAY_OBJECTS_RESOURCES[@]}"; do echo "${a}"; done | grep -v '^$' | sort | uniq)
    readarray -t ARRAY_OBJECTS_OUTPUTS < <(for a in "${ARRAY_OBJECTS_OUTPUTS[@]}"; do echo "${a}"; done | grep -v '^$' | sort | uniq)
    readarray -t ARRAY_OBJECTS_TARGETS < <(for a in "${ARRAY_OBJECTS_TARGETS[@]}"; do echo "${a}"; done | grep -v '^$' | sort | uniq)
    readarray -t ARRAY_OBJECTS_LOADED_PARAMETERS < <(for a in "${ARRAY_OBJECTS_LOADED_PARAMETERS[@]}"; do echo "${a}"; done | grep -v '^$' | sort | uniq)
    readarray -t ARRAY_OBJECTS_LOADED_MAPPINGS < <(for a in "${ARRAY_OBJECTS_LOADED_MAPPINGS[@]}"; do echo "${a}"; done | grep -v '^$' | sort | uniq)
    readarray -t ARRAY_OBJECTS_LOADED_CONDITIONS < <(for a in "${ARRAY_OBJECTS_LOADED_CONDITIONS[@]}"; do echo "${a}"; done | grep -v '^$' | sort | uniq)
    readarray -t ARRAY_OBJECTS_LOADED_RESOURCES < <(for a in "${ARRAY_OBJECTS_LOADED_RESOURCES[@]}"; do echo "${a}"; done | grep -v '^$' | sort | uniq)
    readarray -t ARRAY_OBJECTS_LOADED_OUTPUTS < <(for a in "${ARRAY_OBJECTS_LOADED_OUTPUTS[@]}"; do echo "${a}"; done | grep -v '^$' | sort | uniq)
    readarray -t ARRAY_OBJECTS_LOADED_TARGETS < <(for a in "${ARRAY_OBJECTS_LOADED_TARGETS[@]}"; do echo "${a}"; done | grep -v '^$' | sort | uniq)
    if [ ${PROGRESS_RETURNVAL} -ne 0 ]; then
        for TMP_ERROR in "${ARRAY_ERROR_LOG[@]}"; do log_error "- ${TMP_ERROR}"; done; return $PROGRESS_RETURNVAL
    fi

    log_notice "- Loading Object Files"
    load_object_files "Parameters"; RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then PROGRESS_RETURNVAL=${RETURNVAL}; fi
    load_object_files "Mappings";   RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then PROGRESS_RETURNVAL=${RETURNVAL}; fi
    load_object_files "Conditions"; RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then PROGRESS_RETURNVAL=${RETURNVAL}; fi
    load_object_files "Resources";  RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then PROGRESS_RETURNVAL=${RETURNVAL}; fi
    load_object_files "Outputs";    RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then PROGRESS_RETURNVAL=${RETURNVAL}; fi
    load_object_files "Targets";    RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then PROGRESS_RETURNVAL=${RETURNVAL}; fi
    readarray -t ARRAY_OBJECTS_FILES_PARAMETERS < <(for a in "${ARRAY_OBJECTS_FILES_PARAMETERS[@]}"; do echo "${a}"; done | grep -v '^$' | sort | uniq)
    readarray -t ARRAY_OBJECTS_FILES_MAPPINGS < <(for a in "${ARRAY_OBJECTS_FILES_MAPPINGS[@]}"; do echo "${a}"; done | grep -v '^$' | sort | uniq)
    readarray -t ARRAY_OBJECTS_FILES_CONDITIONS < <(for a in "${ARRAY_OBJECTS_FILES_CONDITIONS[@]}"; do echo "${a}"; done | grep -v '^$' | sort | uniq)
    readarray -t ARRAY_OBJECTS_FILES_RESOURCES < <(for a in "${ARRAY_OBJECTS_FILES_RESOURCES[@]}"; do echo "${a}"; done | grep -v '^$' | sort | uniq)
    readarray -t ARRAY_OBJECTS_FILES_OUTPUTS < <(for a in "${ARRAY_OBJECTS_FILES_OUTPUTS[@]}"; do echo "${a}"; done | grep -v '^$' | sort | uniq)
    readarray -t ARRAY_OBJECTS_FILES_TARGETS < <(for a in "${ARRAY_OBJECTS_FILES_TARGETS[@]}"; do echo "${a}"; done | grep -v '^$' | sort | uniq)
    if [ ${PROGRESS_RETURNVAL} -ne 0 ]; then
        for TMP_ERROR in "${ARRAY_ERROR_LOG[@]}"; do log_error "- ${TMP_ERROR}"; done; return $PROGRESS_RETURNVAL
    fi

    display_variables_object_lists

    load_parameters "${FILE_BUILDER_PROJECT}"
    RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then return $RETURNVAL; fi

    display_variables_parameter
    return 0
}

###--------------------------------------------------------------------------------
## FUNCTION: load_parameters()
## - Loads Parameters into memory from Projects file
## - Arguments
##   - $1: Project File
function load_parameters() {
    local FUNCTION_DESCRIPTION="Load Parameters"
    local TMP_FILE_PROJECT="${1}"

    if(is_empty "${TMP_FILE_PROJECT}"); then
        log_error "${FUNCTION_DESCRIPTION}: Project file not specified"
        return $E_BAD_ARGS
    fi
    if [ ! -f "${TMP_FILE_PROJECT}" ]; then
        log_error "${FUNCTION_DESCRIPTION}: Project file does not exist [${TMP_FILE_PROJECT}]"
        return $E_OBJECT_NOT_FOUND
    fi
    if [ "${TMP_FILE_PROJECT##*.}" != "${EXTENSION_PROJECT}" ]; then
        log_error "${FUNCTION_DESCRIPTION}: Specified file does not appear to be a project file, does not end with '.${EXTENSION_PROJECT}' [${TMP_FILE_PROJECT}]"
        return $E_OBJECT_NOT_FOUND
    fi

    load_array_key_values_from_yaml_file "ARRAY_OBJECTS_LOADED_PARAMETERS[@]" "${TMP_FILE_PROJECT}" "REF_PARAMETER"
    REF_PARAMETER_LASTUPDATE="$($(which date) +'%Y-%m-%d %H:%M:%S.%3N %Z' -u)"
    return 0
}

###--------------------------------------------------------------------------------
## FUNCTION: load_project()
## - loads Project Information
## - Arguments
##   - $1: Project File
function load_project() {
    local FUNCTION_DESCRIPTION="Load Project"
    local TMP_FILE_PROJECT="${1}"

    local TMP_FILENAME_PROJECT=""

    if(is_empty "${TMP_FILE_PROJECT}"); then
        log_error "${FUNCTION_DESCRIPTION}: Project file not specified"
        return $E_BAD_ARGS
    fi
    if [ ! -f "${TMP_FILE_PROJECT}" ]; then
        log_error "${FUNCTION_DESCRIPTION}: Project file does not exist [${TMP_FILE_PROJECT}]"
        return $E_OBJECT_NOT_FOUND
    fi
    if [ "${TMP_FILE_PROJECT##*.}" != "${EXTENSION_PROJECT}" ]; then
        log_error "${FUNCTION_DESCRIPTION}: Specified file does not appear to be a project file, does not end with '.${EXTENSION_PROJECT}' [${TMP_FILE_PROJECT}]"
        return $E_OBJECT_NOT_FOUND
    fi

    FILE_BUILDER_PROJECT="${TMP_FILE_PROJECT}"
    DIRECTORY_BUILDER_PROJECT_BASE="${FILE_BUILDER_PROJECT%/*}"
    DIRECTORY_BUILDER_PROJECT_OBJECTS="${DIRECTORY_BUILDER_PROJECT_BASE}/cfn"
    FILENAME_BUILDER_PROJECT="${FILE_BUILDER_PROJECT##*/}"
    BUILDER_PROJECT_NAME="${FILENAME_BUILDER_PROJECT%.*}"

    log_notice "${FUNCTION_DESCRIPTION}: initializing"
    display_variables_project

    load_variables_details_from_file "${FILE_BUILDER_PROJECT}"
    RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then return ${RETURNVAL};fi
    display_variables_details

    load_paradigm
    RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then return ${RETURNVAL};fi

    return 0
}

###--------------------------------------------------------------------------------
## FUNCTION: load_project_sam()
## - loads SAM Project Information
## - Arguments
##   - $1: Project File
##   - $2: SAM File
function load_project_sam() {
    local FUNCTION_DESCRIPTION="Load Project (SAM)"
    local TMP_FILE_PROJECT="${1}"
    local TMP_FILE_SAM_TEMPLATE="${2}"

    local TMP_FILENAME_PROJECT=""

    if(is_empty "${TMP_FILE_PROJECT}"); then
        log_error "${FUNCTION_DESCRIPTION}: Project file not specified"
        return $E_BAD_ARGS
    fi
    if [ ! -f "${TMP_FILE_PROJECT}" ]; then
        log_error "${FUNCTION_DESCRIPTION}: Project file does not exist [${TMP_FILE_PROJECT}]"
        return $E_OBJECT_NOT_FOUND
    fi
    if [ "${TMP_FILE_PROJECT##*.}" != "${EXTENSION_PROJECT}" ]; then
        log_error "${FUNCTION_DESCRIPTION}: Specified file does not appear to be a project file, does not end with '.${EXTENSION_PROJECT}' [${TMP_FILE_PROJECT}]"
        return $E_OBJECT_NOT_FOUND
    fi
    if(is_empty "${TMP_FILE_SAM_TEMPLATE}"); then
        log_error "${FUNCTION_DESCRIPTION}: SAM Template file not specified"
        return $E_BAD_ARGS
    fi
    if [ ! -f "${TMP_FILE_SAM_TEMPLATE}" ]; then
        log_error "${FUNCTION_DESCRIPTION}: SAM Template file does not exist [${TMP_FILE_SAM_TEMPLATE}]"
        return $E_OBJECT_NOT_FOUND
    fi

    FILE_BUILDER_PROJECT="${TMP_FILE_PROJECT}"
    DIRECTORY_BUILDER_PROJECT_BASE="${FILE_BUILDER_PROJECT%/*}"
    DIRECTORY_BUILDER_PROJECT_OBJECTS="${DIRECTORY_BUILDER_PROJECT_BASE}/cfn"
    FILENAME_BUILDER_PROJECT="${FILE_BUILDER_PROJECT##*/}"
    BUILDER_PROJECT_NAME="${FILENAME_BUILDER_PROJECT%.*}"

    log_notice "${FUNCTION_DESCRIPTION}: initializing"
    display_variables_project

    load_variables_details_from_file "${FILE_BUILDER_PROJECT}"
    RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then return ${RETURNVAL};fi

    display_variables_details

    log_notice "- Loading Parameters"

    local TMP_ARRAY=($(cat "${TMP_FILE_SAM_TEMPLATE}" 2>/dev/null | grep -v "#" | awk "/^Parameters:/ {flag=1;next} /^[A-Za-z]/{flag=0} flag {print}" | grep "^  [A-Za-z]" | sed -e 's/- //g' -e 's/^ *//g' -e 's/ *$//g' | sed -e 's/^ *//g' -e 's/ *$//g' -e 's/:$//g'))

    for TMP_OBJECT in "${TMP_ARRAY[@]}"; do
        add_element_to_array "ARRAY_OBJECTS_LOADED_PARAMETERS" "${TMP_OBJECT}"
    done
    readarray -t ARRAY_OBJECTS_LOADED_PARAMETERS < <(for a in "${ARRAY_OBJECTS_LOADED_PARAMETERS[@]}"; do echo "${a}"; done | grep -v '^$' | sort | uniq)

    load_parameters "${FILE_BUILDER_PROJECT}"
    RETURNVAL="$?"; if [ ${RETURNVAL} -ne 0 ]; then return $RETURNVAL; fi

    display_variables_parameter

    return 0
}

###--------------------------------------------------------------------------------
## FUNCTION: load_secrets()
## - loads Secrets Information
## - Arguments
##   - $1: Project
function load_secrets() {
    local FUNCTION_DESCRIPTION="Load Secrets"
    REF_PROJECT="${1}"
    if(is_empty "${REF_PROJECT}"); then
        log_error "${FUNCTION_DESCRIPTION}: Project not defined"
        return $E_BAD_ARGS
    fi
    log "${FUNCTION_DESCRIPTION}: started [${REF_PROJECT}]"

    REF_PROJECT_FILENAME="${CFN_PROJECT_DIRECTORY}/${REF_PROJECT}.${CFN_PROJECT_EXTENSION}"
    if [ ! -f "${REF_PROJECT_FILENAME}" ]; then
        log_error "Project file not found, aborting [${REF_PROJECT_FILENAME}]"
        return $E_OBJECT_NOT_FOUND
    fi
    REF_SECRETS_FILENAME="${CFN_PROJECT_DIRECTORY}/${REF_PROJECT}.${CFN_SECRETS_EXTENSION}"
    if [ ! -f "${REF_SECRETS_FILENAME}" ]; then
        log_error "Secrets file not found, aborting [${REF_SECRETS_FILENAME}]"
        return $E_OBJECT_NOT_FOUND
    fi

    REF_PROJECT_CATEGORY="$($(which basename) "$($(which dirname) ${REF_PROJECT_FILENAME})")"

    DIRECTORY_BUILDER_PROJECT_OBJECTS="$($(which dirname) "${REF_PROJECT_FILENAME}")/cfn"

    return_yaml_string REF_STACK_NAME "StackName" "${REF_PROJECT_FILENAME}"
    return_yaml_string REF_PROJECT_NAME "ProjectName" "${REF_PROJECT_FILENAME}"
    return_yaml_string REF_PROJECT_ABBR "ProjectAbbr" "${REF_PROJECT_FILENAME}"
    return_yaml_string REF_FUNCTION_NAME "FunctionName" "${REF_PROJECT_FILENAME}"
    return_yaml_string REF_FUNCTION_ABBR "FunctionAbbr" "${REF_PROJECT_FILENAME}"
    return_yaml_string REF_ENVIRONMENT "Environment" "${REF_PROJECT_FILENAME}"
    return_yaml_string REF_REGION "Region" "${REF_PROJECT_FILENAME}"
    return_yaml_string REF_STACK_REFERENCE_VPC "StackReferenceVPC" "${REF_PROJECT_FILENAME}"

    load_account_info "${TARGET_ACCOUNT_NAME}"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        return $E_OBJECT_NOT_FOUND
    fi

    REF_LAST_UPDATE="$($(which date) +'%Y-%m-%d %H:%M:%S.%3N %Z' -u)"

    verify_array_key_values "SECRETS_VARIABLES_REQUIRED[@]" "REF"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        TMP_ERROR_MSG="Failed to load required variables"
        log_error "- ${TMP_ERROR_MSG}"
        return $E_OBJECT_NOT_FOUND
    fi

    line_break
    log "- Profile:             [${REF_PROFILE}]"
    log "- Account:             [${REF_ACCOUNT_NAME} / ${REF_ACCOUNT_ABBR} / ${REF_ACCOUNT_NUMBER}]"
    line_break
    log "- Project:             [${REF_PROJECT_NAME} / ${REF_PROJECT_ABBR}]"
    log "- Function:            [${REF_FUNCTION_NAME} / ${REF_FUNCTION_ABBR}]"
    log "- Environment:         [${REF_ENVIRONMENT}]"
    log "- Region:              [${REF_REGION}]"
    line_break
    log "- Stack Name:          [${REF_STACK_NAME}]"
    log "- Stack Reference VPC: [${REF_STACK_REFERENCE_VPC}]"
    line_break

    return 0
}

###--------------------------------------------------------------------------------
## FUNCTION: load_targets()
## - Loads Targets to Targets Object Array
## - Arguments
##   - $1: Target Source File
function load_targets() {
    local TMP_FILE_TARGETS_SOURCE="${1}"
    local TMP_TARGETS_CONTENT=""
    local TMP_OBJECT=""
    local TMP_RESOURCE_NAME=""
    local TMP_RESOURCE_TYPE=""
    local TMP_RESOURCE_TAG=""

    local OLD_IFS="${IFS}"
    IFS=$'\n'
    for TMP_OBJECT in $(cat "${TMP_FILE_TARGETS_SOURCE}" 2>/dev/null | grep -v "^#\|^;"); do
        TMP_TARGETS_CONTENT="$(echo $(eval echo \"${TMP_OBJECT}\"))"
        if(! is_empty "${TMP_TARGETS_CONTENT}"); then
            TMP_RESOURCE_NAME="$(echo "${TMP_TARGETS_CONTENT}" | awk -F '|' '{print $2}')"
            if(! is_empty "${TMP_RESOURCE_NAME}"); then
                ARRAY_TARGETS_RESOURCES+=("${TMP_TARGETS_CONTENT}")
            fi
        fi
    done
    IFS="${OLD_IFS}"

    readarray -t ARRAY_TARGETS_RESOURCES < <(for a in "${ARRAY_TARGETS_RESOURCES[@]}"; do echo "${a}"; done | grep -v '^$' | sort | uniq)
    return 0
}

###--------------------------------------------------------------------------------
## FUNCTION: load_templates()
## - Loads Objects: Templates to CFN Object Array
## - Arguments
##   - $1: Object Source File
function load_templates() {
    local TMP_FILE_OBJECT_SOURCE="${1}"
    local CFN_OBJECT_TYPE="Templates"
    local TMP_OBJECT=""
    local TMP_FILE_TEMPLATE=""
    local FUNCTION_RETURNVAL=0
    local TMP_ARRAY=($(cat "${TMP_FILE_OBJECT_SOURCE}" 2>/dev/null | grep -v "#" | awk "/^${INDENT_YAML}${CFN_OBJECT_TYPE}:/ {flag=1;next} /^${INDENT_YAML}[A-Za-z]/{flag=0} flag {print}" | sed -e 's/- //g' -e 's/^ *//g' -e 's/ *$//g' | sed -e 's/^ *//g' -e 's/ *$//g'))
    for TMP_OBJECT in "${TMP_ARRAY[@]}"; do
        ARRAY_OBJECTS_TEMPLATES+=("${TMP_OBJECT}")
        TMP_FILE_TEMPLATE="${DIRECTORY_BUILDER_TEMPLATES_BASE}/${TMP_OBJECT}.${EXTENSION_BUILDER_TEMPLATE}"
        ARRAY_OBJECTS_FILES_TEMPLATES+=("${TMP_FILE_TEMPLATE}")
        if [ ! -f "${TMP_FILE_TEMPLATE}" ]; then
            ARRAY_ERROR_LOG+=("File (Template) not found [${TMP_FILE_TEMPLATE}]")
            FUNCTION_RETURNVAL=$E_OBJECT_NOT_FOUND
        fi
    done
    readarray -t ARRAY_OBJECTS_TEMPLATES < <(for a in "${ARRAY_OBJECTS_TEMPLATES[@]}"; do echo "${a}"; done | grep -v '^$' | sort | uniq)
    readarray -t ARRAY_OBJECTS_FILES_TEMPLATES < <(for a in "${ARRAY_OBJECTS_FILES_TEMPLATES[@]}"; do echo "${a}"; done | grep -v '^$' | sort | uniq)
    return $FUNCTION_RETURNVAL
}

###--------------------------------------------------------------------------------
## FUNCTION: manifest_get_checksum()
## - Retrieves Checksum of specified Bulder Object from the Manifest DynamoDB Table
## - Arguments
##   - $1: Variable name to pass info to
##   - $2: Stack ID
##   - $3: DynamoDB Table (Manifest)
##   - $4: Region
function manifest_get_checksum() {
    local __RESULTVAR=$1
    local TMP_STACK_ID="${2}"
    local TMP_DYNAMODB_TABLE="${3}"
    local TMP_AWS_REGION="${4}"

    local TMP_STRING=""
    local AWS_REGION_STRING=""

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    TMP_STRING="$($(which aws) ${AWS_REGION_STRING} dynamodb get-item --table-name ${TMP_DYNAMODB_TABLE} --key "{\"StackId\": {\"S\": \"${TMP_STACK_ID}\"}}" --query 'Item.Checksum' --output text 2>/dev/null | sed -e 's/^ *//g' -e 's/ *$//g')"

    eval $__RESULTVAR="'${TMP_STRING}'"
}

###--------------------------------------------------------------------------------
## FUNCTION: manifest_put_checksum()
## - Puts Checksum of specified Bulder Object in the Manifest DynamoDB Table
## - Arguments
##   - $1: Stack ID
##   - $2: Checksum
##   - $3: DynamoDB Table (Manifest)
##   - $4: Region
function manifest_put_checksum() {
    local FUNCTION_DESCRIPTION="Manifest (Put Checksum)"
    local TMP_STACK_ID="${1}"
    local TMP_CHECKSUM="${2}"
    local TMP_DYNAMODB_TABLE="${3}"
    local TMP_AWS_REGION="${4}"

    local RETURNVAL=""
    local TMP_STRING=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    log "${FUNCTION_DESCRIPTION}: started"
    log "- Stack ID: [${TMP_STACK_ID}]"
    log "- Checksum: [${TMP_CHECKSUM}]"
    log "- Table:    [${TMP_DYNAMODB_TABLE}]"
    log "- Region:   [${TMP_AWS_REGION}]"

    $(which aws) ${AWS_REGION_STRING} dynamodb put-item --table-name ${TMP_DYNAMODB_TABLE} --item "{\"StackId\": {\"S\": \"${TMP_STACK_ID}\"}, \"Checksum\": {\"S\": \"${TMP_CHECKSUM}\"}, \"Timestamp\": {\"S\": \"$(date '+%Y-%m-%d %H:%M:%S.%3N %Z')\"}}" >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
    RETURNVAL="$?"
    $(which sed) -i "s/\o15/_AWS_BAD_IGNORE\\n/g" "${AWS_FILE_RESPONSE}"
    $(which sed) -i '/_AWS_BAD_IGNORE/d' "${AWS_FILE_RESPONSE}"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_add_from_file "${AWS_FILE_ERROR}" "${FUNCTION_DESCRIPTION}: Data containing error" 200000
        log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Data" 200000
        log_error "${FUNCTION_DESCRIPTION}: Failed to update Checksum in DynamoDB Table"
        return $E_AWS_FAILURE
    fi
    log_success "- checksum updated"
    return 0
}
