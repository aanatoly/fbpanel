#!/bin/bash

function usage () 
{
    echo "Updates projects files with latest changes"
    echo "usage: update-proj.sh <tar>"
    echo "  tar  - tar file with latest proj version"
    echo "Must be run from top directory"
    exit $1
}

[ $# -ne 1 ] && usage 1

tar="$1"
tar --strip-components=1 --exclude doc --exclude src \
    --keep-newer-files -xvf "$tar"

