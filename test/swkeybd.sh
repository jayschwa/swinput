#!/bin/bash

export SW_DEV=/dev/swkeybd

function to_dev()
{
    echo "$*" > $SW_DEV
}


for i in `cat testfile.tmp| sed 's,\([.]*\),__\1 ,g'`
do
  TMP=`echo $i | sed 's,__,,g' `

  if [ "$TMP" == "" ];
      then
      TMP=" " 
  fi
  
  to_dev "$TMP"
  
done
