#!/bin/bash

if [ $1 == "--version" ]; then
	echo "0.1-beta"
	exit 0
fi

if(($# < 2)); then
	help
	exit 1
fi

SERVER_LIST="insta2 aw"

SERVER=$1

if [[ $SERVER_LIST != *${SERVER}* ]]; then
	echo "Unknown server $SERVER"
	exit 1
fi

CMD=$2 # start|restart|stop|clear

KATINA_DATA=$HOME/.katina-$SERVER

mkdir -p $KATINA_DATA

KATINA_LOG=$KATINA_DATA/katina.log

NAME=katina-$SERVER
COMMAND="katina"
LOG=$KATINA_LOG
PID=$KATINA_DATA/.pid

help()
{
	echo -e "\nUsage: katina.sh <server> [help|clear|start|stop|restart|show]"
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



