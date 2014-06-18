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
#undef DEBUG

#include <katina/types.h>
#include <katina/log.h>
#include <katina/str.h>

#include <mysql.h>

using namespace katina;
using namespace katina::log;
using namespace katina::types;
using namespace katina::string;

const str NAME = "katina-votes";
const str VERSION = "0.2";
const str TAG = "alpha";

void usage(const str& msg)
{

}

int main(int argc, char* argv[])
{
	const str_vec args(argv + 1, argv + argc);

	str host = "localhost";
	siz port = 3306;
	str user = "katina";
	str pass;

	for(str_vec_citer arg = args.begin(); arg != args.end(); ++arg)
	{
		if(*arg == "-h" || *arg == "--host")
		{
			if(++arg == args.end())
				usage("Expected host");
			host = *arg;
		}
		else if(*arg == "-P" || *arg == "--port")
		{
			if(++arg == args.end())
				usage("Expected port");
			port = to<siz>(*arg);
		}
		else if(*arg == "-u" || *arg == "--user")
		{
			if(++arg == args.end())
				usage("Expected username");
			user = *arg;
		}
		else if(*arg == "-p" || *arg == "--pass")
		{
			if(++arg == args.end())
				usage("Expected password");
			pass = *arg;
		}
	}

//	con("host: " << host);
//	con("port: " << port);
//	con("user: " << user);
//	con("pass: " << pass);
//
	//bug_func();
	MYSQL mysql;

	//str base = "oadb";

	str_vec bases;
	bases.push_back("oadb");
//	bases.push_back("oadb3");
//	bases.push_back("oadb_aw");

	mysql_init(&mysql);

	for(str_vec_iter i = bases.begin(); i != bases.end(); ++i)
	{
		str base = *i;

		if(mysql_real_connect(&mysql, host.c_str(), user.c_str()
			, pass.c_str(), base.c_str(), port, NULL, 0) != &mysql)
		{
			log("DATABASE ERROR: Unable to connect; " << mysql_error(&mysql));
			return 1;
		}

//		bug("Database open");

		soss oss;
		oss << "select `item`,`count` from `votes` where `type` = 'map'";

		str sql = oss.str();

		if(mysql_real_query(&mysql, sql.c_str(), sql.length()))
		{
			log("DATABASE ERROR: Unable to read votes; " << mysql_error(&mysql));
			log("              : sql = " << sql);
			return false;
		}

		struct opine
		{
			siz love, hate, soso;
			opine(): love(0), hate(0), soso(0) {}
		};
		std::map<str,opine> votes; // mapname -> { love, hate, soso }

		MYSQL_RES* result = mysql_store_result(&mysql);

		if(result)
		{
//			bug("Processing votes");

			MYSQL_ROW row;
			while((row = mysql_fetch_row(result)))
			{
				int vote = to<int>(row[1]);
				if(vote > 0)
					++votes[row[0]].love;
				else if(vote < 0)
					++votes[row[0]].hate;
				else
					++votes[row[0]].soso;
			}

			mysql_free_result(result);

			for(std::map<str,opine>::iterator i = votes.begin(); i != votes.end(); ++i)
			{
				con(i->first << ": " << i->second.love << ", " << i->second.hate << ", " << i->second.soso);
			}
		}
		mysql_close(&mysql);
	}
}
