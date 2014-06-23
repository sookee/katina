
REVISION=$(git log -n 1 --pretty=format:%h|tr [:lower:] [:upper:])

USER=$(grep 'user:' ~/.db-unit|cut -d ' ' -f 2)
PASS=$(grep 'pass:' ~/.db-unit|cut -d ' ' -f 2)

AUTH="-u$USER -p$PASS"

mysql $AUTH -e "drop database oadb_test; create database oadb_test;"
mysql $AUTH oadb_test < $HOME/dev/oastats/katina-schema-1.0.sql
mysql $AUTH -e "ALTER TABLE `game` AUTO_INCREMENT = 0;"

katina-rerun.sh unit-test-katina-stats.log

mysqldump --skip-opt --skip-dump-date --compact $AUTH oadb_test > unit-test-katina-stats-$(stamp.sh).sql

