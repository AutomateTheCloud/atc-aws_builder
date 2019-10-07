###------------------------------------------------------------------------------------------------
# script:  ami.inc
# purpose: Collection of functions related to AMI
# version: 1.0.0
#
# function list:
#          - ami_copy
#          - ami_create
#          - ami_description
#          - ami_id_exists
#          - ami_name
#          - ami_name_exists
#          - ami_state
#          - get_latest_ami_amazon_linux_hvm
#          - get_latest_ami_amazon_linux2_hvm
#          - instance_id_exists
###------------------------------------------------------------------------------------------------
REQUIRED_EXECUTABLES+=('aws')

###------------------------------------------------------------------------------------------------
## FUNCTION: ami_copy()
## - Creates AMI based on Image ID
## - Arguments
##   - $1: Variable name to pass newly created AMI ID to
##   - $2: Source AMI ID
##   - $3: AMI Name
##   - $4: AMI Description
##   - $5: Source Region
##   - $6: Destination Region
##   - $7: Encrypt AMI (default: yes)
function ami_copy() {
    local FUNCTION_DESCRIPTION="AMI (Copy)"
    local __RESULTVAR=$1
    local TMP_SOURCE_AMI_ID="${2}"
    local TMP_AMI_NAME="${3}"
    local TMP_AMI_DESCRIPTION="${4}"
    local TMP_AWS_SOURCE_REGION="${5}"
    local TMP_AWS_DESTINATION_REGION="${6}"
    local TMP_AWS_USE_ENCRYPTION="${7}"

    local RETURNVAL=""
    local TMP_STRING=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_ENCRYPTION_FLAG_STRING=""

    if(is_empty "${TMP_AWS_USE_ENCRYPTION}"); then
        TMP_AWS_USE_ENCRYPTION=yes
    fi
    if option_enabled TMP_AWS_USE_ENCRYPTION; then
        AWS_ENCRYPTION_FLAG_STRING="--encrypted"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    log_notice "${FUNCTION_DESCRIPTION}: started"
    log "- Source AMI ID:      [${TMP_SOURCE_AMI_ID}]"
    log "- AMI Name:           [${TMP_AMI_NAME}]"
    log "- AMI Desc:           [${TMP_AMI_DESCRIPTION}]"
    log "- Source Region:      [${TMP_AWS_SOURCE_REGION}]"
    log "- Destination Region: [${TMP_AWS_DESTINATION_REGION}]"
    log "- Use Encryption:     [${TMP_AWS_USE_ENCRYPTION}]"

    $(which aws) --region ${TMP_AWS_DESTINATION_REGION} ec2 copy-image --source-image-id ${TMP_SOURCE_AMI_ID} --name "${TMP_AMI_NAME}" --description "${TMP_AMI_DESCRIPTION}" ${AWS_ENCRYPTION_FLAG_STRING} --source-region "${TMP_AWS_SOURCE_REGION}" >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
    RETURNVAL="$?"
    $(which sed) -i "s/\o15/_AWS_BAD_IGNORE\\n/g" "${AWS_FILE_RESPONSE}"
    $(which sed) -i '/_AWS_BAD_IGNORE/d' "${AWS_FILE_RESPONSE}"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_add_from_file "${AWS_FILE_ERROR}" "${FUNCTION_DESCRIPTION}: Data containing error" 200000
        log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Data" 200000
        log_error "${FUNCTION_DESCRIPTION}: Failed to copy AMI"
        return $E_AWS_FAILURE
    fi
    TMP_STRING="$(cat ${AWS_FILE_RESPONSE} | grep "ImageId" | sed -e 's/{//g' -e 's/}//g' | cut -d',' -f1 | awk -F ": " '{print $2}' | cut -d '"' -f2)"
    line_break
    log "- New AMI ID:         [${TMP_STRING}]"
    eval $__RESULTVAR="'${TMP_STRING}'"
    return 0
}

###------------------------------------------------------------------------------------------------
## FUNCTION: ami_create()
## - Creates AMI based on Image ID
## - Arguments
##   - $1: Variable name to pass newly created AMI ID to
##   - $2: Image ID
##   - $3: AMI Name Prepend String
##   - $4: AMI Description Prepend String
##   - $5: Region
function ami_create() {
    local FUNCTION_DESCRIPTION="AMI (Create)"
    local __RESULTVAR=$1
    local TMP_IMAGE_ID="${2}"
    local TMP_AMI_NAME_PREPEND="${3}"
    local TMP_AMI_DESCRIPTION_PREPEND="${4}"
    local TMP_AWS_REGION="${5}"

    local RETURNVAL=""
    local TMP_STRING=""
    local AWS_FILE_RESPONSE=""
    local AWS_FILE_ERROR=""
    local AWS_REGION_STRING=""
    local AMI_NAME="${TMP_AMI_NAME_PREPEND}_$(date +"%Y-%m-%d")_$(date +"%H-%M")"
    local AMI_DESCRIPTION="${TMP_AMI_DESCRIPTION_PREPEND} ($(date +"%Y-%m-%d") $(date +"%H:%M"))"

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    generate_temp_file AWS_FILE_RESPONSE "aws_cli response file"
    generate_temp_file AWS_FILE_ERROR "aws error log"

    log "${FUNCTION_DESCRIPTION}: started"
    log "- Image ID: [${TMP_IMAGE_ID}]"
    log "- AMI Name: [${AMI_NAME}]"
    log "- AMI Desc: [${AMI_DESCRIPTION}]"
    log "- Region:   [${TMP_AWS_REGION}]"

    $(which aws) ${AWS_REGION_STRING} ec2 create-image --instance-id ${TMP_IMAGE_ID} --name "${AMI_NAME}" --description "${AMI_DESCRIPTION}" >${AWS_FILE_RESPONSE} 2>${AWS_FILE_ERROR}
    RETURNVAL="$?"
    $(which sed) -i "s/\o15/_AWS_BAD_IGNORE\\n/g" "${AWS_FILE_RESPONSE}"
    $(which sed) -i '/_AWS_BAD_IGNORE/d' "${AWS_FILE_RESPONSE}"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_add_from_file "${AWS_FILE_ERROR}" "${FUNCTION_DESCRIPTION}: Data containing error" 200000
        log_add_from_file "${AWS_FILE_RESPONSE}" "${FUNCTION_DESCRIPTION}: Data" 200000
        log_error "${FUNCTION_DESCRIPTION}: Failed to create AMI"
        return $E_AWS_FAILURE
    fi
    TMP_STRING="$(cat ${AWS_FILE_RESPONSE} | grep "ImageId" | sed -e 's/{//g' -e 's/}//g' | cut -d',' -f1 | awk -F ": " '{print $2}' | cut -d '"' -f2)"
    log_success "- Successfully created AMI [${TMP_STRING}]"
    eval $__RESULTVAR="'${TMP_STRING}'"
    return 0
}

###------------------------------------------------------------------------------------------------
## FUNCTION: ami_description()
## - Obtains description for AMI
## - Arguments
##   - $1: Variable name to pass info to
##   - $2: AMI ID
##   - $3: Region
function ami_description() {
    local __RESULTVAR=$1
    local TMP_AMI_ID="${2}"
    local TMP_AWS_REGION="${3}"

    local TMP_STRING=""
    local AWS_REGION_STRING=""

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    TMP_STRING="$($(which aws) ${AWS_REGION_STRING} ec2 describe-images --image-ids ${TMP_AMI_ID} --query 'Images[*].Description' --output text | sed -e 's/^ *//g' -e 's/ *$//g')"
    eval $__RESULTVAR="'${TMP_STRING}'"
}

###------------------------------------------------------------------------------------------------
## FUNCTION: ami_id_exists()
## - Returns true if ami_id exists
## - Arguments
##   - $1: AMI ID
##   - $2: Region
function ami_id_exists() {
    local TMP_AMI_ID="${1}"
    local TMP_AWS_REGION="${2}"

    local RETURNVAL=""
    local AWS_REGION_STRING=""

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    $(which aws) ${AWS_REGION_STRING} ec2 describe-images --image-ids ${TMP_AMI_ID} >/dev/null 2>&1
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        return $E_AWS_RESOURCE_NOT_FOUND
    fi
    return 0
}

###------------------------------------------------------------------------------------------------
## FUNCTION: ami_name()
## - Obtains name for AMI
## - Arguments
##   - $1: Variable name to pass info to
##   - $2: AMI ID
##   - $3: Region
function ami_name() {
    local __RESULTVAR=$1
    local TMP_AMI_ID="${2}"
    local TMP_AWS_REGION="${3}"

    local TMP_STRING=""
    local AWS_REGION_STRING=""

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    TMP_STRING="$($(which aws) ${AWS_REGION_STRING} ec2 describe-images --image-ids ${TMP_AMI_ID} --query 'Images[*].Name' --output text | sed -e 's/^ *//g' -e 's/ *$//g')"
    eval $__RESULTVAR="'${TMP_STRING}'"
}

###------------------------------------------------------------------------------------------------
## FUNCTION: ami_name_exists()
## - Returns true if ami_name exists
## - Arguments
##   - $1: AMI Name
##   - $2: Region
function ami_name_exists() {
    local TMP_AMI_NAME="${1}"
    local TMP_AWS_REGION="${2}"

    local TMP_STRING=""
    local AWS_REGION_STRING=""

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    TMP_STRING="$($(which aws) ${AWS_REGION_STRING} ec2 describe-images --filters "Name=name,Values=${TMP_AMI_NAME}" --query 'Images[*].{Name:Name,ImageId:ImageId}' --output text 2>/dev/null | grep -v '.rc' | sort -k2 | tail -1 | awk '{print $1}' | sed -e 's/{//g' -e 's/}//g')"
    if(is_empty "${TMP_STRING}"); then
        return $E_AWS_RESOURCE_NOT_FOUND
    fi
    return 0
}

###------------------------------------------------------------------------------------------------
## FUNCTION: ami_state()
## - Obtains state for AMI
## - Arguments
##   - $1: Variable name to pass info to
##   - $2: AMI ID
##   - $3: Region
function ami_state() {
    local __RESULTVAR=$1
    local TMP_AMI_ID="${2}"
    local TMP_AWS_REGION="${3}"

    local TMP_STRING=""
    local AWS_REGION_STRING=""

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    TMP_STRING="$($(which aws) ${AWS_REGION_STRING} ec2 describe-images --image-ids ${TMP_AMI_ID} --query 'Images[*].State' --output text | sed -e 's/^ *//g' -e 's/ *$//g')"
    eval $__RESULTVAR="'${TMP_STRING}'"
}

###------------------------------------------------------------------------------------------------
## FUNCTION: get_latest_ami_amazon_linux_hvm()
## - Retrieves latest AMI ID [Amazon Linux (HVM)]
## - Arguments
##   - $1: Variable name to pass AMI ID to
##   - $2: Region
function get_latest_ami_amazon_linux_hvm() {
    local FUNCTION_DESCRIPTION="AMI (Get Latest - Amazon Linux HVM)"
    local __RESULTVAR=$1
    local TMP_AWS_REGION="${2}"

    local TMP_STRING=""
    local AWS_REGION_STRING=""

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    TMP_STRING="$($(which aws) ${AWS_REGION_STRING} ec2 describe-images --filters "Name=name,Values=amzn-ami-hvm*x86_64-gp2" --owners amazon --query 'Images[*].{Name:Name,ImageId:ImageId}' --output text 2>/dev/null | grep -v '.rc' | grep -v 'test' | sort -k2 | tail -1 | awk '{print $1}' | sed -e 's/{//g' -e 's/}//g')"
    if(is_empty "${TMP_STRING}"); then
        log_error "${FUNCTION_DESCRIPTION}: Failed to retrieve AMI ID"
        return $E_AWS_RESOURCE_NOT_FOUND
    fi

    eval $__RESULTVAR="'${TMP_STRING}'"
    return 0
}

###------------------------------------------------------------------------------------------------
## FUNCTION: get_latest_ami_amazon_linux2_hvm()
## - Retrieves latest AMI ID [Amazon Linux 2 (HVM)]
## - Arguments
##   - $1: Variable name to pass AMI ID to
##   - $2: Region
function get_latest_ami_amazon_linux2_hvm() {
    local FUNCTION_DESCRIPTION="AMI (Get Latest - Amazon Linux 2 HVM)"
    local __RESULTVAR=$1
    local TMP_AWS_REGION="${2}"

    local TMP_STRING=""
    local AWS_REGION_STRING=""

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi

    TMP_STRING="$($(which aws) ${AWS_REGION_STRING} ec2 describe-images --filters "Name=name,Values=amzn2-ami-hvm*x86_64-gp2" --owners amazon --query 'Images[*].{Name:Name,ImageId:ImageId}' --output text 2>/dev/null | grep -v '.rc' | grep -v 'test' | sort -k2 | tail -1 | awk '{print $1}' | sed -e 's/{//g' -e 's/}//g')"
    if(is_empty "${TMP_STRING}"); then
        log_error "${FUNCTION_DESCRIPTION}: Failed to retrieve AMI ID"
        return $E_AWS_RESOURCE_NOT_FOUND
    fi
    eval $__RESULTVAR="'${TMP_STRING}'"
    return 0
}

###------------------------------------------------------------------------------------------------
## FUNCTION: instance_id_exists()
## - Returns true if instance_id exists
## - Arguments
##   - $1: Instance ID
##   - $2: Region
function instance_id_exists() {
    local TMP_INSTANCE_ID="${1}"
    local TMP_AWS_REGION="${2}"

    local RETURNVAL=""
    local AWS_REGION_STRING=""

    if(! is_empty "${TMP_AWS_REGION}"); then
        AWS_REGION_STRING="--region ${TMP_AWS_REGION}"
    fi
    $(which aws) ${AWS_REGION_STRING} ec2 describe-instances --instance-ids "${TMP_INSTANCE_ID}" >/dev/null 2>&1
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        return $E_AWS_RESOURCE_NOT_FOUND
    fi
    return 0
}
