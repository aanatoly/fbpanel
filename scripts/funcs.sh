
function error ()
{
    echo "$@"
    exit 1
}

function print_var ()
{
    eval echo \"$1=\$$1\"
}

function check_var ()
{
    eval [ -n \"\$$1\" ] && return
    [ -z "$2" ] && error "Var $1 was not set"
    eval $1="$2"
}

function parse_argv ()
{
    while [ $# -gt 0 ]; do
        if [ "$1" == "--help" ]; then
            info
            help
            exit 0
        fi
        if [ "${1:0:2}" != "--" ]; then
            error "$1 - not a parameter"
        fi
        tmp="${1:2}"
        var=${tmp%%=*}
        val=${tmp#*=}
        if [ "$var" == "$val" ]; then
            let $var=x
        fi
        eval "$var=\"$val\""
        shift
    done
}