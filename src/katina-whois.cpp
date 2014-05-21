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

#include <katina/types.h>
#include <katina/log.h>
#include <katina/str.h>

#include <mysql.h>

using namespace oastats;
using namespace oastats::log;
using namespace oastats::types;
using namespace oastats::string;

const str NAME = "katina-whois";
const str VERSION = "0.1";
const str TAG = "alpha";

std::istream& getstring(std::istream& is, str& s)
{
	char term = ' ';
	if((is >> std::ws).peek() == '"' || is.peek() == '\'')
		is.get(term);

	char c;
	str n;
	while(is.get(c) && c != term)
		n.append(1, c);

	if(n.empty())
		is.setstate(std::ios::failbit);
	else
	{
		s = n;
		is.clear();
	}

	return is;
}

void usage()
{
	std::cout << "usage: katina-whois (GUID|name|ip) [options]*\n";
	exit(1);
}

struct ent
{
	siz count;
	str data;
	ent(): count(0) {}
	ent(const ent& e): count(e.count), data(e.data) {}
	ent(siz count, str data): count(count), data(data) {}
	bool operator<(const ent& e) const
	{
		if(count == e.count)
			return data < e.data;
		return count > e.count;
	}
};

//typedef std::map<const str, db_rec> str_rec_map;
//typedef std::pair<const str, db_rec> str_rec_pair;
typedef std::set<ent> ent_set;
typedef std::map<const str, ent_set> str_ent_set_map;
typedef std::pair<const str, ent_set> str_ent_set_pair;

bool is_guid(const str& s, siz min = 8)
{
	//assert(min <= 8);
	return s.size() <= 8 && s.size() >= min
		&& std::count_if(s.begin(), s.end(), std::ptr_fun<int, int>(isxdigit)) == s.size();
}

bool is_ip(const str& s)
{
	siz dot = std::count(s.begin(), s.end(), '.');
	if(dot > 3)
		return false;

	siz dig = std::count_if(s.begin(), s.end(), std::ptr_fun<int, int>(isdigit));
	if(dig > dot * 3 + 3)
		return false;

	return s.size() == dot + dig;
}

str adjust(const str& name)
{
	str s = name;
	for(siz i = 1; i < s.size(); ++i)
		if(s[i - 1] == '^' && std::isdigit(s[i]))
			replace(s, str(s.c_str() + i - 1, 2), "");
	return s;
}

int main(const int argc, const char* const argv[])
{
	bug_func();
	MYSQL mysql;

	if(argc < 1)
		usage();

	str_vec args(argv + 1, argv + argc);

	str host = "77.237.250.186";
	siz port = 3306;
	str user = "katina";
	str pass = "6B77EA2A";
	str base = "playerdb";

	// whois <GUID> [options]
	// whois <IP> [options]

	mysql_init(&mysql);

	if(mysql_real_connect(&mysql, host.c_str(), user.c_str()
		, pass.c_str(), base.c_str(), port, NULL, 0) != &mysql)
	{
		log("DATABASE ERROR: Unable to connect; " << mysql_error(&mysql));
		return 1;
	}

	bug("Database open");

	bool exact = false;
	bool add_loc = false;
	bool add_ip = false;
	bool add_isp = false;
	str add_loc_type = "code";

	siz chunk = 1;
	siss iss(args[0]);

	str query;
	getstring(iss, query);

	str param;
	for(str_vec_iter i = args.begin() + 1; i != args.end(); ++i)
	{
		const str& param = *i;
		if(param == "+x")
			exact = true;
		else if(param == "+ip")
			add_ip = true;
		else if(param == "+isp")
			add_isp = true;
		else if(!param.find("+loc"))
		{
			str skip;
			std::istringstream iss(param);
			std::getline(iss, skip, '=') >> add_loc_type;
			add_loc = true;
		}
		else if(!param.empty() && param[0] == '#')
			std::istringstream(param.substr(1)) >> chunk;
		else if(!param.empty())
			usage();
	}

	if(!chunk)
	{
		std::cerr << "Bad batch number (at least #1)\n";
		exit(1);
	}

	std::ifstream ifs;
	str_ent_set_map names;
	str_ent_set_map ips;

//	db_rec r;

	lower(query);
	bug("H");

//	if(is_guid(query, 2))
	str guid = trim_copy(query, "(*)");

	MYSQL_RES* result;
	MYSQL_ROW row;

	soss oss;
	oss << "select guid,ip,name from `info`";
	str sql = oss.str();

	if(mysql_real_query(&mysql, sql.c_str(), sql.length()))
	{
		log("DATABASE ERROR: Unable to read votes; " << mysql_error(&mysql));
		log("              : sql = " << sql);
		return false;
	}

	if(is_guid(guid, 3))
	{
		if((result = mysql_store_result(&mysql)))
		{
			while((row = mysql_fetch_row(result)))
			{
				if(exact && lower_copy(row[0]) == guid)
				{
					ips[guid].insert(ent(0, row[1]));
					names[guid].insert(ent(0, row[2]));
				}
				else if(!exact && lower_copy(row[0]).find(guid) != str::npos)
				{
					ips[guid].insert(ent(0, row[1]));
					names[row[0]].insert(ent(0, row[2]));
				}
			}
		}
		mysql_free_result(result);
	}

	str ip = trim_copy(query, "[]");
	if(is_ip(ip))
	{
		if((result = mysql_store_result(&mysql)))
		{
			while((row = mysql_fetch_row(result)))
			{
				if(exact && row[1] == ip)
					ips[row[0]].insert(ent(0, row[1]));
				else if(!exact && !str(row[1]).find(ip)) // starts with
					ips[row[0]].insert(ent(0, row[1]));
			}
		}
		mysql_free_result(result);
		add_ip = true;
	}

//	{
//		// match on name
//		if((result = mysql_store_result(&mysql)))
//		{
//			while((row = mysql_fetch_row(result)))
//			{
//				++count;
//				str name = lower_copy(adjust(row[2]));
//				if(exact && name == query)
//					names[row[0]].insert(ent(0, row[2]));
//				else if(!exact && name.find(query) != str::npos)
//					names[row[0]].insert(ent(0, row[2]));
//			}
//		}
//		mysql_free_result(result);
//	}
//		while(sgl(ifs, line))
//			if(siss(line) >> r)
//			{
//				++count;
//				str data = lower_copy(adjust(r.data));
//				if(exact && data == query)
//					names[r.guid].insert(ent(r.count, r.data));
//				else if(!exact && data.find(query) != str::npos)
//					names[r.guid].insert(ent(r.count, r.data));
//			}
//		ifs.close();
//		ifs.open(bot.getf(RCONICS_DB_IP, RCONICS_DB_IP_DEFAULT));
//		while(sgl(ifs, line))
//			if(siss(line) >> r)
//				if(names.find(r.guid) != names.end())
//					ips[r.guid].insert(ent(r.count, r.data));
//		ifs.close();
//	}
//
//	siz size = 0;
//	for(const str_ent_set_pair&p : names)
//		size += p.second.size();
//
//	const siz start = (chunk - 1) * 10;
//	const siz end = (start + 10) > size ? size : (start + 10);
//
//	if(((chunk - 1) * 10) > size)
//		return bot.cmd_error(msg, "Batch number too high.");
//
//	bot.fc_reply(msg, prompt + "Please remember to keep this information private:");
//	bot.fc_reply(msg, prompt + "Listing #" + std::to_string(chunk)
//		+ " of " + std::to_string((size + 9)/10)
//		+ " (from " + std::to_string(start + 1) + " to "
//		+ std::to_string(end) + " of " + std::to_string(size) + ") " + (exact?"<exact>":"<sub>") + ".");
//
//	siz count = 0;
//	for(const str_ent_set_pair&p : names)
//	{
//		if(count >= start && count >= end)
//			break;
//		for(const ent& e: p.second)
//		{
//			if(count >= start && count >= end)
//				break;
//			if(count >= start)
//			{
//				oss.str("");
//				oss << prompt << " " << (count + 1) << ".";
//				oss << " (" << p.first << ") ";
//				oss << IRC_BOLD << oa_to_IRC(e.data.c_str());
//
//				if(add_ip)
//				{
//					oss << " [";
//					ent_set::iterator ent = ips[p.first].begin();
//					str sep;
//					for(siz i = 0; i < 3 && ent != ips[p.first].end(); ++i, ++ent)
//					{
//						oss << sep << ent->data;
//						if(add_loc)
//							oss << " {loc: " << get_loc(ent->data, add_loc_type) << "}";
//						if(add_isp)
//							oss << " {isp: " << get_isp(ent->data) << "}";
//						sep = " ";
//					}
//					oss << "]";
//				}
//				else if(add_loc)
//				{
//					oss << " [";
//					ent_set::iterator ent = ips[p.first].begin();
//					str sep;
//					for(siz i = 0; i < 3 && ent != ips[p.first].end(); ++i, ++ent)
//					{
//						oss << sep << get_loc(ent->data, add_loc_type);
//						if(add_isp)
//							oss << " {isp: " << get_isp(ent->data) << "}";
//						sep = " ";
//					}
//					oss << "]";
//				}
//				else if(add_isp)
//				{
//					oss << " [";
//					ent_set::iterator ent = ips[p.first].begin();
//					str sep;
//					for(siz i = 0; i < 3 && ent != ips[p.first].end(); ++i, ++ent)
//					{
//						oss << sep << get_isp(ent->data);
//						if(add_loc)
//							oss << " {loc: " << get_loc(ent->data, add_loc_type) << "}";
//						sep = " ";
//					}
//					oss << "]";
//				}
//
//				lock_guard lock(db_mtx);
//				if(siz n = count_notes(p.first))
//					oss << IRC_BOLD << IRC_COLOR << IRC_Red << " +" << n << IRC_NORMAL << " note" << (n>1?"s":"");
//
//				bot.fc_reply(msg, oss.str());
//			}
//			++count;
//		}
//	}
//
//	bot.fc_reply(msg, prompt + "End of listing.");



	// =================================================


//	soss oss;
//	oss << "select `item`,`count` from `votes` where `type` = 'map'";
//
//	str sql = oss.str();
//
//	if(mysql_real_query(&mysql, sql.c_str(), sql.length()))
//	{
//		log("DATABASE ERROR: Unable to read votes; " << mysql_error(&mysql));
//		log("              : sql = " << sql);
//		return false;
//	}
//
//	std::map<str, std::pair<siz, siz> > votes; // mapname -> { loves, hates }
//
//	MYSQL_RES* result = mysql_store_result(&mysql);
//
//	if(result)
//	{
//		bug("Processing votes");
//
//		MYSQL_ROW row;
//		while((row = mysql_fetch_row(result)))
//		{
//			int vote = to<int>(row[1]);
//			if(vote > 0)
//				votes[row[0]].first += vote;
//			else if(vote < 0)
//				votes[row[0]].second -= vote;
//		}
//
//		mysql_free_result(result);
//
//		for(std::map<str, std::pair<siz, siz> >::iterator i = votes.begin(); i != votes.end(); ++i)
//		{
//			con(i->first << ": " << i->second.first << ", " << i->second.second);
//
//			oss.str("");
//			oss << "insert into `polls` (`type`,`item`,`love`,`hate`) values (";
//			oss << "'map','" << i->first << "','" << i->second.first << "','" << i->second.second << "')";
//
//			str sql = oss.str();
//
//			if(mysql_real_query(&mysql, sql.c_str(), sql.length()))
//			{
//				log("DATABASE ERROR: Unable to read votes; " << mysql_error(&mysql));
//				log("              : sql = " << sql);
//				return 1;
//			}
//		}
//	}
//
//	oss.str("");
//	oss << "delete from `votes` where `type` = 'map'";
//
//	sql = oss.str();
//
//	if(mysql_real_query(&mysql, sql.c_str(), sql.length()))
//	{
//		log("DATABASE ERROR: Unable to delete votes; " << mysql_error(&mysql));
//		log("              : sql = " << sql);
//		return false;
//	}
//	mysql_close(&mysql);
}
