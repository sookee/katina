#!/bin/bash

source upgrade-utils.sh

update_db()
{
	if [[ echo $2 | $1 ]]; then
		echo Database updated
	else
		echo Error updating database
		exit 1
	fi
}

read -d '' SQL << 'EOF'
ALTER TABLE `playerstats`
ADD `holyShitFrags` int(2) unsigned NOT NULL,
ADD `holyShitFragged` int(2) unsigned NOT NULL,
ADD `carrierFrags` int(2) unsigned NOT NULL,
ADD `carrierFragsRecv` int(2) unsigned NOT NULL
EOF

update_db $MYSQL $SQL "playerstats"

read -d '' SQL << 'EOF'
DROP TABLE IF EXISTS `version`;
CREATE TABLE `version` (
  `maj` int unsigned NOT NULL,
  `min` int(2) ZEROFILL NOT NULL,
  PRIMARY KEY (`maj`,`min`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;
INSERT INTO version (`maj`,`min`) values (1,0);
EOF

update_db $MYSQL $SQL "version"

exit 1




echo SQL: $SQL

KATINA_DATA=data


