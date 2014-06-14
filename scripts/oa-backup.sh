#!/bin/bash

#EXEC=echo

usage()
{
	echo "Usage: ${0##*/} <database> <username> <password>"
}

if(($# < 3)); then
	usage
	exit 1
fi

base=$1
user=$2
pass=$3

MYSQL="mysqldump"

$EXEC $MYSQL --user="$user" --password="$pass" --database="$base" > $HOME/logs/mysql-$base-$(stamp.sh).sql

