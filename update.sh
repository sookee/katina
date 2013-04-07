#!/bin/bash

make clean
git pull
make

mkdir -p $HOME/bin
cp src/katina $HOME/bin
cp stamp.sh $HOME/bin
cp katina-irc $HOME/bin
cp katina-votes $HOME/bin


