#!/bin/bash

#EXEC=echo

usage()
{
	echo "Usage: ${0##*/} <database> <username> <password> <game_id>"
}

if(($# < 4)); then
	usage
	exit 1
fi

base=$1
user=$2
pass=$3
game=$4

MYSQL="mysql"

declare -r tables=(
	'awards'
	'caps'
	'damage'
	'deaths'
	'game'
	'kills'
	'ovo'
	'playerstats'
	'facefrags'
	'spawnkills'
	'pushes'
	'pickups'
	'holycraps'
	'flagkills'
	'playerstats'
	'speed'
	'time'
	'weapon_usage'
)

for table in "${tables[@]}"
do
	echo TABLE: $table
	sql="delete from $table where game_id = '$game'"
	#	echo "SQL: $sql"
	$EXEC $MYSQL --user="$user" --password="$pass" --database="$base" --execute="${sql}"
done

