#!/bin/bash


RIGHT="r"
LEFT="l"
UP="u"
DOWN="d"

MOUSE_DEV=/dev/swmouse

function pause()
{
    sleep 0
}

function to_dev()
{
    echo "$*" > $MOUSE_DEV
}

function right()
{
    to_dev $RIGHT
}

function left()
{
    to_dev $LEFT
}

function up()
{
    to_dev $UP
}

function down()
{
    to_dev $DOWN
}


function mup()
{
    UTMP=0
    while [ "$UTMP" -lt $1 ]
      do
      echo "u" > $MOUSE_DEV
      UTMP=`expr $UTMP + 1`
    done
}

function mdown()
{
    UTMP=0
    while [ "$UTMP" -lt $1 ]
      do
      echo "d" > $MOUSE_DEV
      UTMP=`expr $UTMP + 1`
    done
}

function mright()
{
    UTMP=0
    while [ "$UTMP" -lt $1 ]
      do
      echo "r" > $MOUSE_DEV
      UTMP=`expr $UTMP + 1`
    done
}

function mleft()
{
    UTMP=0
    while [ "$UTMP" -lt $1 ]
      do
      echo "l" > $MOUSE_DEV
      UTMP=`expr $UTMP + 1`
    done
}

function qmright()
{
    echo "r$1" > $MOUSE_DEV
}

function qmleft()
{
    echo "l$1" > $MOUSE_DEV
}

function qmdown()
{
    echo "m$1" > $MOUSE_DEV
}

function qmup()
{
    echo "u$1" > $MOUSE_DEV
}


function square()
{
    right 
    up
    left
    down
}

function msquare()
{
    mright $1
    mup $1
    mleft $1
    mdown $1
}

function mcross()
{
    mright $1 
    mleft  $1 
    mleft  $1 
    mright $1
    mup    $1
    mdown  $1
    mdown  $1
    mup    $1
}

function qmsquare()
{
    qmright $1
    qmup $1
    qmleft $1
    qmdown $1
}

function qmcross()
{
    qmright $1 
    qmleft  $1 
    qmleft  $1 
    qmright $1
    qmup    $1
    qmdown  $1
    qmdown  $1
    qmup    $1
}



function circle() 
{
    \rm -f circle.log
    bc -l bc/circle.bc | sed 's,\(.\)\.[0-9]*,0\1,g' > circle.log

   .  ./circle.log
    

}

circle 

mcross 50
mcross 100
mcross 200
msquare 50
msquare 100
msquare 200
exit
