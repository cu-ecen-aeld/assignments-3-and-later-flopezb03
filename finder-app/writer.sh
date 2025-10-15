#!/bin/bash

if [ $# -lt 2 ]; then
	echo "Usage: $0 <filename> <write_string>"
	exit 1
fi

writefile=$1
writestr=$2

mkdir -p $(dirname $writefile)
echo $writestr > $writefile

exit 0
