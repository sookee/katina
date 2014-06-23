#!/bin/bash

usage()
{
	echo "usage   : ${0##*/} name"
}

help()
{
	echo "author  : SooKee oasookee@gmail.com"
	echo "version : ${0##*/} 0.1"
	echo "synopsis: Search for a map in a bunch of pk3 files"
	usage
}

if(($# < 1)); then
	usage
	exit 1
fi

if [[ $1 == "-h" || $1 == "--help" ]]; then
	help
	exit 0
fi

for f in $(ls *.pk3); do echo "$f: "; unzip -l $f | grep $1; done

