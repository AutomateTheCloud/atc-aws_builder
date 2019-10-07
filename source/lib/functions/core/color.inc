###--------------------------------------------------------------------------------
# script:  color.inc
# purpose: Collection of functions related to bringing some color to STDOUT
# version: 1.0.0
#
# function list:
#          - color_lookup
#          - color_text
#          - debug_color_text
###------------------------------------------------------------------------------------------------

###------------------------------------------------------------------------------------------------
# Define all the colors
COLOR_BLUE='\e[0;34m'
COLOR_BLACK='\e[30m'
COLOR_CYAN='\e[0;36m'
COLOR_DARK_GRAY='\e[0;90m'
COLOR_GREEN='\e[0;32m'
COLOR_LIGHT_BLUE='\e[1;34m'
COLOR_LIGHT_CYAN='\e[1;36m'
COLOR_LIGHT_GRAY='\e[0;37m'
COLOR_LIGHT_GREEN='\e[1;32m'
COLOR_LIGHT_MAGENTA='\e[1;35m'
COLOR_LIGHT_RED='\e[1;31m'
COLOR_LIGHT_YELLOW='\e[1;33m'
COLOR_MAGENTA='\e[0;35m'
COLOR_RED='\e[0;31m'
COLOR_WHITE='\e[1;37m'
COLOR_YELLOW='\e[0;33m'
COLOR_NONE='\e[0m'

AZURE_DEVOPS_COLOR_BLUE="##[command]"
AZURE_DEVOPS_COLOR_GREEN="##[section]"
AZURE_DEVOPS_COLOR_ORANGE="##[warning]"
AZURE_DEVOPS_COLOR_PURPLE="##[debug]"
AZURE_DEVOPS_COLOR_RED="##[error]"

###--------------------------------------------------------------------------------
## FUNCTION: color_lookup()
## - wrapper function for making curl calls
## - performs retries if enabled and appropriate
## - attempts to catch all curl specific return codes and attempts to handle the return code appropriately
## - Arguments
##   - $1: color
function color_lookup() {
    local USE_COLOR="$(to_lower "$1")"
    case ${USE_COLOR} in
        blue)         echo -n "${COLOR_BLUE}"; return;;
        cyan)         echo -n "${COLOR_CYAN}"; return;;
        dark_gray)    echo -n "${COLOR_DARK_GRAY}"; return;;
        green)        echo -n "${COLOR_GREEN}"; return;;
        light_blue)   echo -n "${COLOR_LIGHT_BLUE}"; return;;
        light_cyan)   echo -n "${COLOR_LIGHT_CYAN}"; return;;
        light_gray)   echo -n "${COLOR_LIGHT_GRAY}"; return;;
        light_green)  echo -n "${COLOR_LIGHT_GREEN}"; return;;
        light_purple) echo -n "${COLOR_LIGHT_PURPLE}"; return;;
        light_red)    echo -n "${COLOR_LIGHT_RED}"; return;;
        magenta)      echo -n "${COLOR_MAGENTA}"; return;;
        purple)       echo -n "${COLOR_PURPLE}"; return;;
        red)          echo -n "${COLOR_RED}"; return;;
        white)        echo -n "${COLOR_WHITE}"; return;;
        yellow)       echo -n "${COLOR_YELLOW}"; return;;
        none)         echo -n "${COLOR_NONE}"; return;;
        *)            echo "${1}"; return;;
        ?)            echo "${COLOR_NONE}"; return;;
    esac
}

###--------------------------------------------------------------------------------
## FUNCTION: color_text()
## - returns text gently swaddled in radiant color
## - Arguments
##   - $1: color
##   - $2: text
##   - $3: color to resume
function color_text() {
    COLOR_TO_RESUME="${3}"
    if (is_empty "${COLOR_TO_RESUME}"); then
        COLOR_TO_RESUME="${LOG_COLOR_MESSAGE}"
    fi
    echo -en "${1}${2}${COLOR_TO_RESUME}"
}

###--------------------------------------------------------------------------------
## FUNCTION: debug_color_text()
## - echos to screen a test line with each of the defined colors
## - for debug purposes only
function debug_color_text() {
    local COLOR_ARRAY=( blue cyan dark_gray green light_blue light_cyan light_gray light_green light_purple light_red magenta orange pink purple red white yellow none bad_test )
    for i in "${COLOR_ARRAY[@]}"; do
        echo "$(color_text "$(color_lookup "${i}")" "1234567890 abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ - ${i}")"
    done
}
