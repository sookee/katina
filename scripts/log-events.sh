#!/bin/bash

VERSION="0.2"

give_usage()
{
	echo "usage: ${0##*/} <logfile> <event>"
	echo "     : List all the events in the <logfile> that left-match the <event> substring"
	echo "     : sorted according to frequency"
}

if [[ $1 == '-h' || $1 == '--help' ]]; then
	give_usage
	exit 0
fi

if(($# < 2)); then
	give_usage
	exit 1
fi

#grep -oP ":\d\d $2[^:]+:" $1|cut -b 4-|sort|uniq -c|sort -nr|cut -b 10-
grep -oP ":\d\d $2[^:]+:" $1|cut -b 4-|awk '{ print length($1) " " $0; }' $FILE | sort -u -n|cut -d ' ' -f 2-


