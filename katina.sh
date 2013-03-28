#!/bin/bash

KATINA=$HOME/dev/oastats/src/katina
KATINA_LOG=$HOME/.katina/katina.log

SERVER_LOG=$HOME/.openarena/CTF/insta.log

$KATINA $SERVER_LOG > $KATINA_LOG 2>&1

