#!/bin/bash

# check for mysql
((tries = 0))
read -s -p "Enter MYSQL root password: " mysql_root_pw
while ! mysql -u root -p$mysql_root_pw  -e ";" > /dev/null 2>&1
do
	echo
	((++tries))
	if((tries > 2)); then
		echo "Check MySQL is running and/or you have the correct password."
		exit 1
	fi
    read -s -p "Can't connect, please retry: " mysql_root_pw
done

