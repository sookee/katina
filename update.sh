#!/bin/bash

screen -d -r katina 
cd $HOME/dev/oastats
git pull
make

