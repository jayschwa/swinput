#!/bin/sh


export PATH=${PATH}:/sbin


MODULES='usbcore usb-uhci input keybdev'
MYMODULES='swmouse swkeybd'

function makenod()
{
    export NAME=$1
    mknod /dev/$NAME c 10 `grep $NAME /proc/misc | awk '{print $1}'`
    chmod a+rw /dev/$NAME
}


function insert_modules()
{
    for mod in $MODULES
    do
      modprobe $mod
    done

    for mod in $MYMODULES
    do
      insmod src/${mod}.o
    done
}




if [ "$1" == "start" ];
then
    insert_modules
    for mod in $MYMODULES
    do
      makenod $mod
    done
else
    for mod in $MYMODULES
    do
      rmmod $mod
      rm /dev/$mod
    done
fi
