#!/bin/bash

grep CVAR data/katina.log|cut -d ' ' -f 4-|cut -d ' ' -f -2

