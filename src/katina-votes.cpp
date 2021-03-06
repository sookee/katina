/*
 * katina-votes.cpp
 *
 *  Created on: 07 Apr 2013
 *      Author: oasookee@googlemail.com
 */


/*-----------------------------------------------------------------.
| Copyright (C) 2013 SooKee oasookee@googlemail.com               |
'------------------------------------------------------------------'

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
02110-1301, USA.

http://www.gnu.org/licenses/gpl-2.0.html

'-----------------------------------------------------------------*/

#include "types.h"
#include "log.h"
#include "str.h"

#include <mysql.h>

using namespace oastats;
using namespace oastats::log;
using namespace oastats::types;
using namespace oastats::string;

const str NAME = "katina-votes";
const str VERSION = "0.1";
const str TAG = "alpha";

int main()
{
	bug_func();
	MYSQL mysql;

	str host = "localhost";
	siz port = 3306;
	str user = "katina";
	str pass = "6B77EA2A";
	str base = "oadb";

	mysql_init(&mysql);

	if(mysql_real_connect(&mysql, host.c_str(), user.c_str()
		, pass.c_str(), base.c_str(), port, NULL, 0) != &mysql)
	{
		log("DATABASE ERROR: Unable to connect; " << mysql_error(&mysql));
		return 1;
	}

	bug("Database open");

	soss oss;
	oss << "select `item`,`count` from `votes` where `type` = 'map'";

	str sql = oss.str();

	if(mysql_real_query(&mysql, sql.c_str(), sql.length()))
	{
		log("DATABASE ERROR: Unable to read votes; " << mysql_error(&mysql));
		log("              : sql = " << sql);
		return false;
	}

	str_siz_map votes;

	MYSQL_RES* result = mysql_store_result(&mysql);

	if(result)
	{
		bug("Processing votes");

		MYSQL_ROW row;
		while((row = mysql_fetch_row(result)))
			votes[row[0]] += to<int>(row[1]);

		mysql_free_result(result);

		for(str_siz_map_iter i = votes.begin(); i != votes.end(); ++i)
		{
			con(i->first << ": " << i->second);

			oss.str("");
			oss << "insert into `polls` (`type`,`item`,`count`) values (";
			oss << "'map','" << i->first << "','" << i->second << "')";

			str sql = oss.str();

			if(mysql_real_query(&mysql, sql.c_str(), sql.length()))
			{
				log("DATABASE ERROR: Unable to read votes; " << mysql_error(&mysql));
				log("              : sql = " << sql);
				return 1;
			}
		}
	}

	oss.str("");
	oss << "delete from `votes` where `type` = 'map'";

	sql = oss.str();

	if(mysql_real_query(&mysql, sql.c_str(), sql.length()))
	{
		log("DATABASE ERROR: Unable to delete votes; " << mysql_error(&mysql));
		log("              : sql = " << sql);
		return false;
	}

	mysql_close(&mysql);
}
