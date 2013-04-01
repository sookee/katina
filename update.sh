#!/bin/bash

git pull
make

mkdir -p $HOME/bin
cp src/katina $HOME/bin
cp stamp.sh $HOME/bin
cp katina-votes.sh $HOME/bin


