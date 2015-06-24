#!/bin/bash

DRIVER_FILENAME=ta_canbus.ko
DRIVER_FILEPATH=.

if [ -n "$1" ] ; then 
    DRIVER_FILEPATH=$1
fi


INFO=$(modinfo $DRIVER_FILEPATH/$DRIVER_FILENAME | grep ^version:)
INFO=$(echo $INFO | sed 's/version://')
INFO=$(echo $INFO | sed 's/^[\t ]*//')
INFO=$(echo $INFO | sed 's/[\t ]*$//')

echo $INFO


