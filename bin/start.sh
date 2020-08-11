#!/bin/bash

cd /home/pi/heartrate-server/bin
./heartrate-server &
server_pid=$!
sleep 10
kill -KILL $server_pid
./heartrate-server
