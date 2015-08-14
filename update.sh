#!/bin/bash

make -C build-debug clean
git pull
make -C build-debug all
make -C build-debug install

