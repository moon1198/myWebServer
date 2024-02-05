#! /bin/bash

hostname="127.0.0.1"
port=9006
data="1"

while true; do 
	echo "$data" | nc $hostname $port
	sleep 1
done
