#!/bin/sh


export PATH=${PATH}:/sbin


MODULES='usbcore usb-uhci input keybdev'
MYMODULES='swmouse swkeybd'


function build_em()
{
    cd src 
    make -C /usr/src/kernel-headers-$(uname -r)/ M=$(pwd) modules modules_install
    cd ..
}

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
      rmmod    $mod
      modprobe ${mod}
    done
}




if [ "$1" == "start" ];
then
    build_em
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
