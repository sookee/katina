#!/bin/bash

grep CONF: data/katina.log|sort -u|cut -d ' ' -f 4

