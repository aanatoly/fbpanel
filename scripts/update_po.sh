#/bin/bash

function info()
{
    cat <<EOF
Updates PO file with new strings that were added to project
EOF
}

function help()
{
    cat <<EOF
Parameters
   old - old PO file 
   new - new PO file
For example:
   create_po.sh --old=/tmp/ru.po --new=po/ru.po
EOF
}

source scripts/funcs.sh

parse_argv "$@"
check_var old
check_var new

function print_translation()
{
    while read -r nline; do
        if [ "$nline" == "$1" ]; then
            while read -r nline; do
                echo "$nline"
                [ -z "$nline" ] && break
            done
            return
        fi
    done < $old
    echo "msgstr \"\""
    echo
}

tmp=/tmp/tmp-$$
msgid=msgid
while read -r line; do
    echo "$line"
    if [ "${line:0:${#msgid}}" == "$msgid" ]; then
        read -r tmp1
        read -r tmp2
        if [ "$tmp1" == "msgstr \"\"" -a "$tmp2" == "" ]; then
            print_translation "$line"
        else
            echo "$tmp1"
            echo "$tmp2"
        fi
    fi
done < $new > $tmp
mv $tmp $new

# code
