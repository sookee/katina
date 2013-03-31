#!/bin/bash

git pull
make

mkdir -p $HOME/bin
cp src/katina $HOME/bin