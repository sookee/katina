#!/bin/bash

#EXEC=echo

REVISION=$(git log -n 1 --pretty=format:%h|tr [:lower:] [:upper:])
DIRTY=$(git status|grep 'modified:')

USER=$(grep 'user:' ~/.db-unit|cut -d ' ' -f 2)
PASS=$(grep 'pass:' ~/.db-unit|cut -d ' ' -f 2)
BASE="oadb_test" # must be same t=one that katina-rerun.sh script uses

AUTH="-u$USER -p$PASS"

$EXEC mysql $AUTH -e "drop database if exists $BASE; create database $BASE;"
$EXEC mysql $AUTH $BASE < $HOME/dev/oastats/katina-schema-1.0.sql
$EXEC mysql $AUTH $BASE -e "ALTER TABLE game AUTO_INCREMENT = 0;"

$EXEC katina-rerun.sh unit-test-katina-stats.log

#echo DIRTY: $DIRTY

if [[ -n $DIRTY ]]; then
	STAMP=$(stamp.sh)
else
	STAMP=$REVISION
fi

#echo STAMP: $STAMP

OPTIONS="--skip-opt --skip-dump-date --compact"

$EXEC mysqldump $OPTIONS $AUTH $BASE > unit-test-katina-stats-${STAMP}.sql

