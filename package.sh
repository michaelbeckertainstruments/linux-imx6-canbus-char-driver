#!/bin/bash

VERSION=$(./get_build_number.pl)
INSTALL_DIR=ta_canbus_driver_install
PACKAGE_NAME=install_ta_canbus_driver.tgz
INSTALL_NAME=install_ta_canbus_driver-$BOARD-$VERSION.sh


if [ -z "$BOARD" ] ; then 
    echo "\$BOARD is not set!  Who are we packaging for?"
    exit 1
elif [ $BOARD = variscite ] ; then 
    echo "Packaging Variscite YOCTO Build"
    INSTALL_TEMPLATE=install-variscite-template.sh
elif [ $BOARD = bd ] ; then 
    echo "Packaging BoundaryDevices Ubuntu Build"
    INSTALL_TEMPLATE=install-template.sh
else 
    echo "Unknown Board $BOARD."
    exit 1
fi


rm -rf $INSTALL_DIR
rm -rf $PACKAGE_NAME
rm -rf $INSTALL_NAME

mkdir $INSTALL_DIR

cp ta_canbus.ko $INSTALL_DIR
cp ta_canbus_load.sh $INSTALL_DIR
cp get_ta_canbus_version.sh $INSTALL_DIR

tar -cvzf $PACKAGE_NAME $INSTALL_DIR


cp $INSTALL_TEMPLATE $INSTALL_NAME

echo "#VERSION=$VERSION" >> $INSTALL_NAME
echo "#BOARD=$BOARD" >> $INSTALL_NAME

echo "PAYLOAD:" >> $INSTALL_NAME
cat $PACKAGE_NAME >> $INSTALL_NAME
rm $PACKAGE_NAME


