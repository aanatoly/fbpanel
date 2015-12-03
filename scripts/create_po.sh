#/bin/bash

function info()
{
    cat <<EOF
Create gettext translation files (*.po) with strings marked
with _() or c_() macros. Script scans all C files in current dir and down.

EOF
}

function help()
{
    cat <<EOF
Parameters
   package - package name
   version - package version
   destdir - dir to save resulted POT and PO files (default is po/ )
   locale - full locale name, e.g ru_RU.UTF-8
   translator - name and email of a translator
   update - update existing PO with new strings
For example:
   create_po.sh --package=fbpanel --version=6.2 --locale=ru_RU.UTF-8
EOF
}


source scripts/funcs.sh

parse_argv "$@"
check_var package
check_var version
check_var locale
check_var destdir po

# code
pot=$destdir/$package.pot
po=$destdir/$locale.po

list=/tmp/list-$$
mkdir -p $destdir
find . -name "*.c" -o -name "*.h" > $list

# Create POT file and set charset to be UTF-8
2>/dev/null xgettext --package-name=$package --package-version=$version \
    --default-domain=$package --from-code=UTF-8 \
    --force-po -k_ -kc_ -f $list -o - |\
sed '/^"Content-Type:/ s/CHARSET/UTF-8/' > $pot

# create language translation files, POs
if [ -n "$update" ]; then
    old=/tmp/old-$$
    cp $po $old
fi
tr="${translator:-Automaticaly generated}"
2>/dev/null msginit --no-translator --locale=$locale --input=$pot -o - |\
sed "/\"Last-Translator:/ s/Auto.*\\\n/$tr\\\n/" > $po
if [ -n "$update" ]; then
    scripts/update_po.sh --old=$old --new=$po
    rm $old
fi
echo Created $po
rm $pot