#!/bin/bash

# When cross compiling, you may put cross compiler directory 
# in PATH before native gcc (aka spoofing), or you may set CC 
# to exact name of cross compiler:
#    CC=/opt/ppc_gcc/bin/gcc endianess

# x86 and friends are considerd LITTLE endian, all others are BIG
a=`${CC:-gcc} -v 2>&1 | grep Target`
[ $? -ne 0 ] && exit 1
#echo $a

if [ "${a/86/}" != "$a" ]; then
    echo LITTLE
else
    echo BIG
fi

