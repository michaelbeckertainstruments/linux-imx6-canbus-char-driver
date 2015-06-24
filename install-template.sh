#!/bin/bash
#######################################################################
#   @see:
#   http://www.linuxjournal.com/content/add-binary-payload-your-shell-scripts
#######################################################################

HOME_DIR=/opt
INST_DIR=ta_bin
DRV_DIR=ta_canbus_driver
TARED_INSTALL_DIR=ta_canbus_driver_install


function log_output()
{
    TAG="TA-INSTALL-CANBUS[$$]" 
    logger -s -t "$TAG" -- "$@"
}


function untar_payload()
{
	MATCH=$(grep --text --line-number '^PAYLOAD:$' $0 | cut -d ':' -f 1)
	PAYLOAD_START=$((MATCH + 1))
	tail -n +$PAYLOAD_START $0 | tar -xzvmf -
    rc=$?
    log_output "untar_payload return code = $rc"
    return $rc
}


if [ $EUID != 0 ] ; then 
    log_output "This script must be run as root..."
    exit 1
fi


#
#   Extract this first.  In case this is a bad file or there 
#   is corruption this will allow us to return an error to Inst.
#
log_output "Extracting upgrade"
untar_payload
rc=$?
if [ $rc != 0 ] ; then 
    log_output "$0 is corrupted!  (rc = $rc)  Aborting install!"
    exit 1
fi


log_output "Stopping Inst"
pkill -TERM Inst


if lsmod | grep ta_canbus 2>&1 >/dev/null ; then 

    log_output "TA CANbus Driver is loaded, unloading"
    rmmod -s ta_canbus
    rc=$?
    if [ $rc != 0 ] ; then 
        log_output "Failed unloading driver! (rc = $rc)  Aborting install!"
        exit 1
    fi
else
    log_output "TA CANbus Driver was not loaded"
fi


log_output "Removing existing software"
rm -rf $HOME_DIR/$DRV_DIR


#
#   Move the relative pathed new directory to the 
#   absolute pathed bin directory.  This moves it and 
#   renames it.
#
log_output "Installing upgrade"
mv $TARED_INSTALL_DIR $HOME_DIR/$DRV_DIR


log_output "Restarting driver and Inst"
cd /etc
exec ./rc.local


log_output "Opps, shouldn't ever see this!"
exit 1



