#!/bin/bash

if [[ $1 == "-d" ]]; then
	EXEC=echo
fi

REFERENCE_SQL="unit-test-katina-stats-20140623-155705-49F98C8.sql" # error parsing g_timestamp
REFERENCE_SQL="unit-test-katina-stats-20140701-222405-4955C04.sql"

REVISION=$(git log -n 1 --pretty=format:%h|tr [:lower:] [:upper:])
DIRTY=$(git status|grep 'modified:')

USER=$(grep 'user:' ~/.db-unit|cut -d ' ' -f 2)
PASS=$(grep 'pass:' ~/.db-unit|cut -d ' ' -f 2)
BASE="oadb_test" # must be the same one that katina-rerun.sh script uses

AUTH="-u$USER -p$PASS"

$EXEC mysql $AUTH -e "drop database if exists $BASE; create database $BASE;"
$EXEC mysql $AUTH $BASE < $HOME/dev/oastats/katina-schema-1.0.sql
$EXEC mysql $AUTH $BASE -e "ALTER TABLE game AUTO_INCREMENT = 0;"

## RERUN

#LOGFILE_OVERRIDE=$HOME/dev/oastats/data/unit-test-katina-stats.log

export UNIT_HOME=$HOME/dev/oastats/unit-tests
export UNIT_DATA=$UNIT_HOME/data

export KATINA_HOME=$HOME/dev/oastats/build-debug/src
export LD_LIBRARY_PATH=$KATINA_HOME
KATINA=$KATINA_HOME/katina

KATINA_DATA=katina-unit-stats
KATINA_LOG=$KATINA_DATA/katina.log

mkdir -p $KATINA_DATA/logs
if [[ -f $KATINA_LOG ]]; then
	STAMP=$(ls -l --time-style +%Y%m%d-%H%M%S $KATINA_LOG|cut -d ' ' -f 6)
	mv $KATINA_LOG $KATINA_DATA/logs/katina-${STAMP}.log
fi
$EXEC $KATINA $KATINA_DATA | tee $KATINA_LOG 2>&1

#$EXEC katina-rerun.sh unit-test-katina-stats.log

#echo DIRTY: $DIRTY

if [[ -n $DIRTY ]]; then
	STAMP=$(stamp.sh)
else
	STAMP=$(stamp.sh)-$REVISION
fi

#STAMP=X
#echo STAMP: $STAMP

OPTIONS="--skip-opt --skip-dump-date --compact"

$EXEC mysqldump $OPTIONS $AUTH $BASE > unit-test-katina-stats-${STAMP}.sql

$EXEC diff -q $REFERENCE_SQL unit-test-katina-stats-${STAMP}.sql

$EXEC rm -i unit-test-katina-stats-${STAMP}.sql

