#!/bin/sh

if [ $# -lt 2 ]; then
	echo "Usage: $0 <directory> <search_string>"
	exit 1
fi

filesdir=$1
searchstr=$2

if [ ! -d $filesdir ]; then
	echo "Usage: $0 <directory> <search_string>"
	exit 1
fi

#x=$(ls -R $filesdir | wc -l)
#y=$(ls -R $filesdir | grep $searchstr | wc -l)
x=$(find "$filesdir" -type f | wc -l)
y=$(grep -r "$searchstr" "$filesdir" 2>/dev/null | wc -l)

echo "The number of files are $x and the number of matching lines are $y"

exit 0
