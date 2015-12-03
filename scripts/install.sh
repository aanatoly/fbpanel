#!/bin/bash

usage () 
{
    echo "install.sh <dir perm> <dir> <files perm> <files>"
}


[ $# -eq 3 ] && exit 0
[ $# -lt 4 ] && usage && exit 1

idir=${DESTDIR}$2
p1=$1
p2=$3
shift 3

echo install -m $p1 -d "$idir"
install -m $p1 -d "$idir"
echo install -m $p2 "$@" "$idir"
install -m $p2 "$@" "$idir"

