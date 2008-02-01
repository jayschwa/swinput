#!/bin/bash


NR_OF_INSMODS=10
CNT=0
BASE_DIR=../src/
SUDO=sudo
RMMOD=rmmod
INSMOD=insmod
CHMOD="chmod a+rwx"
MODULES="swkeybd swmouse"

if [ "$1" != "" ]
then
    NR_OF_INSMODS=$1
fi

check_ret_val()
{
    if  [ $1 -ne 0 ]
	then
	echo "Failed: $*"
	exit $1
    fi
}

swin_in()
{
    for mod in $MODULES
    do
	$SUDO $INSMOD  $BASE_DIR/${mod}.ko
	check_ret_val $? $SUDO $RMMOD  $mod
	sleep 1
	$SUDO $CHMOD /dev/$mod
	check_ret_val $? $SUDO $RMMOD  $mod
    done
}

swin_out()
{
    for mod in $MODULES
    do
	$SUDO $RMMOD  $mod
	check_ret_val $? $SUDO $RMMOD  $mod
    done
}

relaxed_out()
{
    for mod in $MODULES
    do
	$SUDO $RMMOD  $mod 2>/dev/null
    done
}

relaxed_out
while [ $CNT -lt $NR_OF_INSMODS ]
do
    echo "  loop $CNT"
    echo "    insert modules"
    swin_in
    sleep 1 
    echo "    remove modules"
    swin_out
    sleep 1 
    CNT=$(( CNT + 1 ))
done
echo "  insert modules"
swin_in
