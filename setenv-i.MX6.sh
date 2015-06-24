#!/bin/bash 
#
#   You must "source" this script!
#
#   Reference:
#   http://boundarydevices.com/cross-compiling-kernels-2014-edition-2/
#
#
if [[ $- != *i* ]] ; then 
    echo This script must be sourced, not run.
    echo Correct Usage: source $0
    exit 1;
fi

echo Setting up terminal i.MX6 cross-compile out-of-tree driver.
echo

export ARCH=arm
export CROSS_COMPILE=arm-linux-gnueabihf-

export KDIR=~/work/linux-imx6
#export KDIR=/home/mike/work/3.10.17-27-boundary-4t3/build


export PS1="i.MX6 CANbus Driver:$ "

export BOARD=bd

