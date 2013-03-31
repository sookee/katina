#!/bin/bash

git pull
make

mkdir -p $HOME/bin
cp katina $HOME/bin