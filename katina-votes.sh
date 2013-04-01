#!/bin/bash

# vote.oasago2.16A8F64B: 1
# vote.ps37ctf.16A8F64B: 1
# vote.ps9ctf.16A8F64B: -1

declare -A maps

echo "" > ~/.katina/records.tmp
echo "" > ~/.katina/votes-$(stamp.sh).txt

while read line
do
	vote=$(echo $line | grep 'vote\.')

	if((${#vote} > 0)); then
		mapname=$(echo $line|cut -d '.' -f 2)
		guid=$(echo $line|cut -d '.' -f 3|cut -d ':' -f 1)
		vote=$(echo $line|cut -d '.' -f 3|cut -d ':' -f 2)
		((maps[$mapname] = ${maps[$mapname]} + $vote))
	else
		echo $line >> ~/.katina/records.tmp
	fi
done < ~/.katina/records.txt

for mapname in ${!maps[@]}
do
	echo $mapname ${maps[$mapname]} >> ~/.katina/votes-$(stamp).txt
done

mv ~/.katina/records.txt ~/.katina/records.bak
mv ~/.katina/records.tmp ~/.katina/records.txt
