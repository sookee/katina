#!/bin/bash

THIS_SCRIPT=${0##*/}

check_pass()
{
	echo ';' | mysql -B --user=root --password=$1 2> /dev/null
}

check_base()
{
	echo "use $2" | mysql -B --user=root --password=$1 2> /dev/null
}

PASSWORD=

while ! check_pass $PASSWORD
do
	echo
	echo -n "Enter mysql password: ";
	read -s PASSWORD
done

echo OK

BASE="none"

while test -z $BASE || ! check_base $PASSWORD $BASE
do
	echo
	echo -n "Enter mysql database: ";
	read BASE
done

echo OK

MYSQL="mysql -B --user=root --pass=$PASSWORD $BASE"


