#!/bin/bash

export SW_DEV=/dev/swkeybd
export TEST_FILE=data/swkeybd/simple.txt

function to_dev()
{
    echo "$*" > $SW_DEV
}


writer()
{
    echo writing file
    START=1
    for i in `cat $TEST_FILE | sed 's,\([.]*\),__\1 ,g'`
    do
	TMP=`echo $i | sed 's,__,,g' `
	if [ "$TMP" == "" ];
	then
	    TMP=" " 
	    if [ $START -eq 1 ] ; then START=0 ; continue ; fi
	fi
	
	to_dev "$TMP"
    done
    to_dev "[ENTER]" 
    echo 
    echo wrote file
}

sleep 5
writer 
