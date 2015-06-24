#!/bin/bash 

if [ -z "$CROSS_COMPILE" ] ; then 
    echo "It looks like you forgot to source setenv-i.MX6.sh"
    exit 1
fi

make clean 
./inc_build_number.pl
make

./package.sh

