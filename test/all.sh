#!/bin/bash

check_ret_val()
{
    if  [ $1 -ne 0 ]
	then
	echo "Failed: $*"
	exit $1
    fi
}


TMP=0
while [ $TMP -ne 10 ]
do
    echo "Test all: loop nr ${TMP}"
    echo "-- inout.sh -- "
    ./inout.sh 2
    check_ret_val $?

    echo "-- swkeybd.sh -- "
    sleep 1
    ./swkeybd.sh
    check_ret_val $?

    echo "-- swmouse.sh -- "
    sleep 1
    ./swmouse.sh
    check_ret_val $?

    TMP=$(( TMP  + 1 ))
done