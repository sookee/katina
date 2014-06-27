#!/bin/bash

VERSION="0.2"

give_usage()
{
	echo "usage   : ${0##*/} [-h|--help] | [-v|--version] | [-x|--exact] [-l|--list] <substr>"
	echo "        : -h | --help    print help message and exit"
	echo "        : -v | --version print version and exit"
	echo "        : -x | --exact   case sensitive match"
	echo "        : -l | --list    list all paks searched"
	echo "        : <substr>       part, or all, of the map name to find"
}

give_help()
{
	echo "script : ${0##*/}"
	echo "author  : SooKee <oasookee@gmail.com>"
	echo "version : $VERSION"
	echo "license : GPLv2"
	echo "synopsis: Search for a map in a bunch of pk3 files"
	give_usage
}

while [[ ${1:0:1} == "-" ]]
do
	if [[ $1 == "-x" || $1 == "--exact" ]]; then
		((do_exact = 1))
	elif [[ $1 == "-h" || $1 == "--help" ]]; then
		((do_help = 1))
	elif [[ $1 == "-l" || $1 == "--list" ]]; then
		((do_list = 1))
	elif [[ $1 == "-v" || $1 == "--version" ]]; then
		((do_version = 1))
	fi
	shift
done

if(($do_help)); then
	give_help
	exit 0
fi

if(($do_version)); then
	echo $VERSION
	exit 0
fi

if(($# < 1)); then
	give_usage
	exit 1
fi

FLAGS="-P"

if [[ -z $do_exact ]]; then
	FLAGS="-i $FLAGS"
fi

#echo FLAGS: $FLAGS
#exit 1

for pak in *.[Pp][Kk]3
do
	if(($do_list)); then echo "$pak:"; fi
	found=$(unzip -l $pak | grep $FLAGS "$1.*\.bsp")
	if(($? == 0)); then
		if((!do_list)); then echo "$pak:"; fi
#		echo $found|while read line
		unzip -l $pak | grep $FLAGS "$1.*\.bsp"|while read line
		do
			#  6217676  10-07-2001 20:11   maps/q3w1.bsp
			out=$(echo $line|cut -d ' ' -f 4-|cut -d '/' -f 2)
			echo "    $out";
		done
	fi
done

