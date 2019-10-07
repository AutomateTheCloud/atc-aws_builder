###------------------------------------------------------------------------------------------------
# script:  metadata.inc
# purpose: Collection of functions related to AWS (Metadata)
# version: 1.0.0
#
# function list:
#          - aws_metadata_account_id
#          - aws_metadata_account_id_from_cli
#          - aws_metadata_auto_scaling_group_name
#          - aws_metadata_get_tag
#          - aws_metadata_hostname
#          - aws_metadata_hostname_private
#          - aws_metadata_hostname_public
#          - aws_metadata_instance_id
#          - aws_metadata_ipaddress_private
#          - aws_metadata_ipaddress_public
#          - aws_metadata_mac
#          - aws_metadata_region
#          - aws_metadata_route_id_private
#          - aws_metadata_route_id_public
#          - aws_metadata_vpc_cidr
#          - aws_metadata_vpc_dns
#          - aws_metadata_vpc_id
###------------------------------------------------------------------------------------------------
REQUIRED_EXECUTABLES+=('aws' 'curl')

###------------------------------------------------------------------------------------------------
# Variables
URL_AWS_BASE="http://169.254.169.254/latest"

###------------------------------------------------------------------------------------------------
## FUNCTION: aws_metadata_account_id()
## - Returns AWS Account ID
## - Arguments
##   - $1: Variable name to pass info to
function aws_metadata_account_id() {
    local FUNCTION_DESCRIPTION="AWS Metadata (Account ID)"
    local __RESULTVAR=$1
    local TMP_STRING="$($(which curl) --retry 3 --silent --fail ${URL_AWS_BASE}/dynamic/instance-identity/document | awk -F\" '/accountId/ {print $4}' | sed -e 's/^ *//g' -e 's/ *$//g')"
    eval $__RESULTVAR="'${TMP_STRING}'"
}

###------------------------------------------------------------------------------------------------
## FUNCTION: aws_metadata_account_id_from_cli()
## - Returns AWS Account ID
## - Arguments
##   - $1: Variable name to pass info to
function aws_metadata_account_id_from_cli() {
    local FUNCTION_DESCRIPTION="AWS Metadata (Account ID)"
    local __RESULTVAR=$1
    local TMP_STRING="$($(which aws) sts get-caller-identity --output text --query 'Account' | sed -e 's/^ *//g' -e 's/ *$//g')"
    eval $__RESULTVAR="'${TMP_STRING}'"
}

###------------------------------------------------------------------------------------------------
## FUNCTION: aws_metadata_auto_scaling_group_name()
## - Returns AWS VPC ID
## - Arguments
##   - $1: Variable name to pass info to
##   - $2: Region
##   - $3: Instance ID
function aws_metadata_auto_scaling_group_name() {
    local FUNCTION_DESCRIPTION="AWS Metadata (AutoScaling Group Name)"
    local TMP_STRING=""
    local __RESULTVAR=$1
    local TMP_AWS_REGION=$2
    local TMP_AWS_INSTANCE_ID=$3

    if(is_empty "${TMP_AWS_REGION}"); then
        aws_metadata_region TMP_AWS_REGION
    fi
    if(is_empty "${TMP_AWS_INSTANCE_ID}"); then
        aws_metadata_instance_id TMP_AWS_INSTANCE_ID
    fi
    local AWS_FILE_ERROR=""

    generate_temp_file AWS_FILE_ERROR "aws error log"

    TMP_STRING="$($(which aws) --region ${TMP_AWS_REGION} ec2 describe-tags --filters "Name=resource-id,Values=${TMP_AWS_INSTANCE_ID}" "Name=key,Values=aws:autoscaling:groupName" --query 'Tags[*].Value' --output text 2>${AWS_FILE_ERROR})"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_warning "${FUNCTION_DESCRIPTION}: Failed to retrieve AutoScaling Group Name"
        log_add_from_file "${LOG_FILE_AWS_CLI_ERROR}" "aws_cli error file"
        return 1
    fi
    eval $__RESULTVAR="'${TMP_STRING}'"
}

###------------------------------------------------------------------------------------------------
## FUNCTION: aws_metadata_get_tag()
## - Returns Instance Tag
## - Arguments
##   - $1: Variable name to pass info to
##   - $2: Key
##   - $3: Region (optional, defaults to self region)
##   - $4: Instance ID (optional, defaults to self instance ID)
function aws_metadata_get_tag() {
    local FUNCTION_DESCRIPTION="AWS Metadata (Get Tag)"
    local TMP_STRING=""
    local __RESULTVAR=$1
    local TMP_KEY_NAME=$2
    local TMP_AWS_REGION=$3
    local TMP_AWS_INSTANCE_ID=$4

    if(is_empty "${TMP_AWS_REGION}"); then
        aws_metadata_region TMP_AWS_REGION
    fi
    if(is_empty "${TMP_AWS_INSTANCE_ID}"); then
        aws_metadata_instance_id TMP_AWS_INSTANCE_ID
    fi
    local AWS_FILE_ERROR=""

    generate_temp_file AWS_FILE_ERROR "aws error log"

    TMP_STRING="$($(which aws) ec2 describe-instances --region ${TMP_AWS_REGION} --instance-ids ${TMP_AWS_INSTANCE_ID} --query "Reservations[*].Instances[*].Tags[?Key=='${TMP_KEY_NAME}'].{Value:Value}" --output text 2>${AWS_FILE_ERROR})"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_warning "${FUNCTION_DESCRIPTION}: Failed to retrieve VPC ID"
        log_add_from_file "${LOG_FILE_AWS_CLI_ERROR}" "aws_cli error file"
        return 1
    fi
    eval $__RESULTVAR="'${TMP_STRING}'"
}

###------------------------------------------------------------------------------------------------
## FUNCTION: aws_metadata_hostname()
## - Returns Hostname FQDN (attempts to pull public hostname first, the falls back to internal)
## - Arguments
##   - $1: Variable name to pass info to
function aws_metadata_hostname() {
    local FUNCTION_DESCRIPTION="AWS Metadata (Hostname)"
    local __RESULTVAR=$1
    local TMP_STRING_HOSTNAME=""
    aws_metadata_hostname_public TMP_STRING_HOSTNAME
    if(is_empty "${TMP_STRING_HOSTNAME}"); then
        aws_metadata_hostname_private TMP_STRING_HOSTNAME
    fi
    eval $__RESULTVAR="'${TMP_STRING_HOSTNAME}'"
}

###------------------------------------------------------------------------------------------------
## FUNCTION: aws_metadata_hostname_private()
## - Returns local hostname fqdn
## - Arguments
##   - $1: Variable name to pass info to
function aws_metadata_hostname_private() {
    local FUNCTION_DESCRIPTION="AWS Metadata (Local Hostname)"
    local __RESULTVAR=$1
    local TMP_STRING="$($(which curl) --retry 3 --silent --fail ${URL_AWS_BASE}/meta-data/local-hostname | sed -e 's/^ *//g' -e 's/ *$//g')"
    eval $__RESULTVAR="'${TMP_STRING}'"
}

###------------------------------------------------------------------------------------------------
## FUNCTION: aws_metadata_hostname_public()
## - Returns public hostname fqdn
## - Arguments
##   - $1: Variable name to pass info to
function aws_metadata_hostname_public() {
    local FUNCTION_DESCRIPTION="AWS Metadata (Public Hostname)"
    local __RESULTVAR=$1
    local TMP_STRING="$($(which curl) --retry 3 --silent --fail ${URL_AWS_BASE}/meta-data/public-hostname | sed -e 's/^ *//g' -e 's/ *$//g')"
    eval $__RESULTVAR="'${TMP_STRING}'"
}

###------------------------------------------------------------------------------------------------
## FUNCTION: aws_metadata_instance_id()
## - Returns Instance ID
## - Arguments
##   - $1: Variable name to pass info to
function aws_metadata_instance_id() {
    local FUNCTION_DESCRIPTION="AWS Metadata (Instance ID)"
    local __RESULTVAR=$1
    local TMP_STRING="$($(which curl) --retry 3 --silent --fail ${URL_AWS_BASE}/meta-data/instance-id | sed -e 's/^ *//g' -e 's/ *$//g')"
    eval $__RESULTVAR="'${TMP_STRING}'"
}

###------------------------------------------------------------------------------------------------
## FUNCTION: aws_metadata_ipaddress_private()
## - Returns Private IP Address
## - Arguments
##   - $1: Variable name to pass info to
function aws_metadata_ipaddress_private() {
    local FUNCTION_DESCRIPTION="AWS Metadata (Private IP Address)"
    local __RESULTVAR=$1
    local TMP_STRING="$($(which curl) --retry 3 --silent --fail ${URL_AWS_BASE}/meta-data/local-ipv4 | sed -e 's/^ *//g' -e 's/ *$//g')"
    eval $__RESULTVAR="'${TMP_STRING}'"
}

###------------------------------------------------------------------------------------------------
## FUNCTION: aws_metadata_ipaddress_public()
## - Returns Public IP Address
## - Arguments
##   - $1: Variable name to pass info to
function aws_metadata_ipaddress_public() {
    local FUNCTION_DESCRIPTION="AWS Metadata (Public IP Address)"
    local __RESULTVAR=$1
    local TMP_STRING="$($(which curl) --retry 3 --silent --fail ${URL_AWS_BASE}/meta-data/public-ipv4 | sed -e 's/^ *//g' -e 's/ *$//g')"
    eval $__RESULTVAR="'${TMP_STRING}'"
}

###------------------------------------------------------------------------------------------------
## FUNCTION: aws_metadata_mac()
## - Returns MAC
## - Arguments
##   - $1: Variable name to pass info to
function aws_metadata_mac() {
    local FUNCTION_DESCRIPTION="AWS Metadata (MAC)"
    local __RESULTVAR=$1
    local TMP_STRING="$($(which curl) --retry 3 --silent --fail ${URL_AWS_BASE}/meta-data/mac | sed -e 's/^ *//g' -e 's/ *$//g')"
    eval $__RESULTVAR="'${TMP_STRING}'"
}

###------------------------------------------------------------------------------------------------
## FUNCTION: aws_metadata_region()
## - Returns AWS Region
## - Arguments
##   - $1: Variable name to pass info to
function aws_metadata_region() {
    local FUNCTION_DESCRIPTION="AWS Metadata (Region)"
    local __RESULTVAR=$1
    local TMP_STRING="$($(which curl) --retry 3 --silent --fail ${URL_AWS_BASE}/dynamic/instance-identity/document | awk -F\" '/region/ {print $4}' | sed -e 's/^ *//g' -e 's/ *$//g')"
    eval $__RESULTVAR="'${TMP_STRING}'"
}

###------------------------------------------------------------------------------------------------
## FUNCTION: aws_metadata_route_id_private()
## - Returns Private Route ID
## - Arguments
##   - $1: Variable name to pass info to
##   - $2: Region
##   - $3: Instance ID
function aws_metadata_route_id_private() {
    local FUNCTION_DESCRIPTION="AWS Metadata (Private Route ID)"
    local TMP_STRING=""
    local __RESULTVAR=$1
    local TMP_AWS_REGION=$2
    local TMP_AWS_VPC_ID=$3

    if(is_empty "${TMP_AWS_REGION}"); then
        aws_metadata_region TMP_AWS_REGION
    fi
    if(is_empty "${TMP_AWS_VPC_ID}"); then
        aws_metadata_vpc_id TMP_AWS_VPC_ID
    fi
    local AWS_FILE_ERROR=""

    generate_temp_file AWS_FILE_ERROR "aws error log"

    TMP_STRING="$($(which aws) ec2 describe-route-tables --region ${TMP_AWS_REGION} --query 'RouteTables[*].RouteTableId' --filters Name=vpc-id,Values=${TMP_AWS_VPC_ID} Name=tag:Network,Values=private --output text)"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_warning "${FUNCTION_DESCRIPTION}: Failed to retrieve Private Route ID"
        log_add_from_file "${LOG_FILE_AWS_CLI_ERROR}" "aws_cli error file"
        return 1
    fi
    eval $__RESULTVAR="'${TMP_STRING}'"
}

###------------------------------------------------------------------------------------------------
## FUNCTION: aws_metadata_route_id_public()
## - Returns Public Route ID
## - Arguments
##   - $1: Variable name to pass info to
##   - $2: Region
##   - $3: Instance ID
function aws_metadata_route_id_public() {
    local FUNCTION_DESCRIPTION="AWS Metadata (Public Route ID)"
    local TMP_STRING=""
    local __RESULTVAR=$1
    local TMP_AWS_REGION=$2
    local TMP_AWS_VPC_ID=$3

    if(is_empty "${TMP_AWS_REGION}"); then
        aws_metadata_region TMP_AWS_REGION
    fi
    if(is_empty "${TMP_AWS_VPC_ID}"); then
        aws_metadata_vpc_id TMP_AWS_VPC_ID
    fi
    local AWS_FILE_ERROR=""

    generate_temp_file AWS_FILE_ERROR "aws error log"

    TMP_STRING="$($(which aws) ec2 describe-route-tables --region ${TMP_AWS_REGION} --query 'RouteTables[*].RouteTableId' --filters Name=vpc-id,Values=${TMP_AWS_VPC_ID} Name=tag:Network,Values=public --output text)"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_warning "${FUNCTION_DESCRIPTION}: Failed to retrieve Public Route ID"
        log_add_from_file "${LOG_FILE_AWS_CLI_ERROR}" "aws_cli error file"
        return 1
    fi
    eval $__RESULTVAR="'${TMP_STRING}'"
}

###------------------------------------------------------------------------------------------------
## FUNCTION: aws_metadata_vpc_cidr()
## - Returns AWS VPC CIDR Range
## - Arguments
##   - $1: Variable name to pass info to
function aws_metadata_vpc_cidr() {
    local FUNCTION_DESCRIPTION="AWS Metadata (VPC CIDR Range)"
    local __RESULTVAR=$1
    local TMP_ETH0_MAC="$(cat /sys/class/net/eth0/address)"
    local TMP_STRING="$($(which curl) --retry 3 --silent --fail ${URL_AWS_BASE}/meta-data/network/interfaces/macs/${TMP_ETH0_MAC}/vpc-ipv4-cidr-block | sed -e 's/^ *//g' -e 's/ *$//g')"
    eval $__RESULTVAR="'${TMP_STRING}'"
}

###------------------------------------------------------------------------------------------------
## FUNCTION: aws_metadata_vpc_dns()
## - Returns AWS VPC DNS IP
## - Arguments
##   - $1: Variable name to pass info to
function aws_metadata_vpc_dns() {
    local FUNCTION_DESCRIPTION="AWS Metadata (VPC ID)"
    local TMP_STRING=""
    local __RESULTVAR=$1

    local TMP_VPCCIDR=""
    local TMP_VPCNET=""
    local TMP_VPCBASE=""
    local TMP_VPCDNS=""

    aws_metadata_vpc_cidr TMP_VPCCIDR
    TMP_VPCDNS="$(echo "${TMP_VPCCIDR}" | cut -d"." -f1-3).2"

    eval $__RESULTVAR="'${TMP_VPCDNS}'"
}

###------------------------------------------------------------------------------------------------
## FUNCTION: aws_metadata_vpc_id()
## - Returns AWS VPC ID
## - Arguments
##   - $1: Variable name to pass info to
##   - $2: Region
##   - $3: Instance ID
function aws_metadata_vpc_id() {
    local FUNCTION_DESCRIPTION="AWS Metadata (VPC ID)"
    local TMP_STRING=""
    local __RESULTVAR=$1
    local TMP_AWS_REGION=$2
    local TMP_AWS_INSTANCE_ID=$3

    if(is_empty "${TMP_AWS_REGION}"); then
        aws_metadata_region TMP_AWS_REGION
    fi
    if(is_empty "${TMP_AWS_INSTANCE_ID}"); then
        aws_metadata_instance_id TMP_AWS_INSTANCE_ID
    fi
    local AWS_FILE_ERROR=""

    generate_temp_file AWS_FILE_ERROR "aws error log"

    TMP_STRING="$($(which aws) ec2 describe-instances --region ${TMP_AWS_REGION} --instance-ids ${TMP_AWS_INSTANCE_ID} --query 'Reservations[*].Instances[*].VpcId' --output text 2>${AWS_FILE_ERROR})"
    RETURNVAL="$?"
    if [ ${RETURNVAL} -ne 0 ]; then
        log_warning "${FUNCTION_DESCRIPTION}: Failed to retrieve VPC ID"
        log_add_from_file "${LOG_FILE_AWS_CLI_ERROR}" "aws_cli error file"
        return 1
    fi
    eval $__RESULTVAR="'${TMP_STRING}'"
}
