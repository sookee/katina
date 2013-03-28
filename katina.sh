#!/bin/bash

CMD=$1 # start|restart|stop|clear

KATINA_HOME=$HOME/dev/oastats
KATINA_DATA=$HOME/.katina

KATINA=$KATINA_HOME/src/katina
KATINA_LOG=$KATINA_DATA/katina.log

SERVER_LOG=$HOME/.openarena/CTF/insta.log

NAME=katina
COMMAND="$KATINA $SERVER_LOG"
LOG=$KATINA_LOG
PID=$KATINA_DATA/.pid

help()
{
	echo -e "\nUsage: katina.sh [help|clear|start|stop|restart|show]"
	echo
	echo -e "\thelp    - Show this help"
	echo -e "\tclear   - Clear the running flag"
	echo -e "\tstart   - Start $NAME"
	echo -e "\tstop    - Stop $NAME"
	echo -e "\trestart - Restart $NAME"
	echo -e "\tstatus  - Is $NAME running?"
	echo -e "\tshow    - Show the $NAME start command."
}

clear()
{
	if [[ -f $PID ]]; then
		ps -p $(cat $PID) > /dev/null 2>&1
		if [[ $? == 0 ]];then
			echo "Unable to clear while $NAME is still running."
		else
			rm -f $PID
		fi
	else
		echo "Nothing to clear."
	fi
}

start()
{
	if [[ -f $PID ]]; then
		echo "$NAME already running."
	else
		nohup $COMMAND > $LOG 2>&1 &
		echo $! > $PID		
		echo "$NAME has been started."
	fi
}

stop()
{
	if [[ -f $PID ]]; then
		kill $(cat $PID)
		rm -f $PID
		echo "$NAME has been stopped."
	else
		echo "$NAME was not running."
	fi
}

restart()
{
	stop; sleep 3; start;
}

status()
{
	if [[ -f $PID ]]; then
		echo "$NAME is running."
	else
		echo "$NAME is not running."
	fi
}

show()
{
	echo nohup $COMMAND '>' $LOG '2>&1 &'
}

case $CMD in
	"help")
		help ;;
	"clear")
		clear ;;
	"start")
		start ;;
	"stop")
		stop ;;
	"restart")
		restart ;;
	"status")
		status ;;
		"show")
		show ;;
	*)
		help ;;
esac



