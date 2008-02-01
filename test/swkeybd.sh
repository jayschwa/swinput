#!/bin/bash

export SW_DEV=/dev/swkeybd
export TEST_FILE=data/swkeybd/simple.txt

function to_dev()
{
    echo "$*" > $SW_DEV
}

./sw_writer.sh &

echo "cat to file..."
xterm -e "cat > tmpfile.txt" &
echo "sleep 10"
sleep 10
to_dev "[CONTROL_DOWN]" 
to_dev "d" 
to_dev "[CONTROL_UP]" 
sleep 2 

diff tmpfile.txt $TEST_FILE 
exit $?