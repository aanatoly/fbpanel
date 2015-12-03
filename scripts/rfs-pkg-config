#!/bin/bash

# Tt runs pkg-config in a way that everything is searched under RFS
# but reported including full path. Suitable for cross-compiling
# Suppose we have /tmp/rfs/usr/lib/pkgconfig/glib-2.0.pc, then
# RFS=/tmp/rfs ./pc.sh --cflags glib-2.0
# -I/tmp/rfs/usr/include/glib-2.0 -I/tmp/rfs/usr/lib/glib-2.0/include
#
# RFS=/tmp/rfs ./pc.sh --libs glib-2.0
# -L/tmp/rfs/usr/lib   


#RFS=/nfsroot
if [ -n "$RFS" ]; then
    ds=`find $RFS  -maxdepth 4 -name pkgconfig -type d`
    [ $? -ne 0 ] && [ -z "$ds" ] && exit 1
    for i in "$ds"; do
        PKG_CONFIG_LIBDIR="$i:$PKG_CONFIG_LIBDIR"
    done
    export PKG_CONFIG_LIBDIR
    export PKG_CONFIG_ALLOW_SYSTEM_CFLAGS=yes 
    export PKG_CONFIG_ALLOW_SYSTEM_LIBS=yes
fi

# export PKG_CONFIG_SYSROOT_DIR=$RFS # does not work,  -l flags disappears

# pkg-config "$@" | sed -e "s/\\(^\\|[[:space:]]\\)-\(I\|L\)/\\1-\\2${RFS//\//\\/}/g"
var=$(pkg-config --silence-errors "$@")
[ "$?" -ne 0 ] && exit 1
if [ -n "$RFS" ]; then
    sed -e "s/\\(^\\|[[:space:]]\\)-\(I\|L\)/\\1-\\2${RFS//\//\\/}/g" <<< "$var"
else
    echo "$var"
fi


