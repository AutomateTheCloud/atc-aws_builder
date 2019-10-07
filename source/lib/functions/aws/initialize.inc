###------------------------------------------------------------------------------------------------
# script:  initialize.inc
# purpose: Initializes AWS CLI use for Scripts
# version: 1.0.0
###------------------------------------------------------------------------------------------------
REQUIRED_EXECUTABLES+=('aws')

###------------------------------------------------------------------------------------------------
# Load AWS Credentials into Environment
if(is_empty "${AWS_ACCESS_KEY_ID}" && is_empty "${AWS_PROFILE_TARGET}"); then
    $(which aws) --profile default sts get-caller-identity >/dev/null 2>&1
    RETURNVAL="$?"
    if [ ${RETURNVAL} -eq 0 ]; then 
        AWS_PROFILE_TARGET="default"
    else
        log_highlight "Loading AWS Credentials from IAM Role"
    fi
fi

if(! is_empty "${AWS_PROFILE_TARGET}"); then
    log_highlight "Loading AWS Credentials from profile [${AWS_PROFILE_TARGET}]"
    AWS_ACCESS_KEY_ID="$($(which aws) configure get aws_access_key_id --profile ${AWS_PROFILE_TARGET} 2>/dev/null | sed -e 's/^ *//g' -e 's/ *$//g' 2>&1)"
    AWS_SECRET_ACCESS_KEY="$($(which aws) configure get aws_secret_access_key --profile ${AWS_PROFILE_TARGET} 2>/dev/null | sed -e 's/^ *//g' -e 's/ *$//g' 2>&1)"
    AWS_SESSION_TOKEN="$($(which aws) configure get aws_session_token --profile ${AWS_PROFILE_TARGET} 2>/dev/null | sed -e 's/^ *//g' -e 's/ *$//g' 2>&1)"
    if(is_empty "${AWS_ACCESS_KEY_ID}"); then log_error "- Failed to load profile [${AWS_PROFILE_TARGET}]"; exit_logic $E_BAD_ARGS; fi
fi

export AWS_ACCESS_KEY_ID="${AWS_ACCESS_KEY_ID}"
export AWS_SECRET_ACCESS_KEY="${AWS_SECRET_ACCESS_KEY}"
if(! is_empty "${AWS_SESSION_TOKEN}"); then
    export AWS_SESSION_TOKEN="${AWS_SESSION_TOKEN}"
fi
