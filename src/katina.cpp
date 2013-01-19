/*
 * katina.cpp
 *
 *  Created on: 18 Jun 2012
 *      Author: oasookee@googlemail.com
 */


/*-----------------------------------------------------------------.
| Copyright (C) 2012 SooKee oasookee@googlemail.com               |
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

#include "logrep.h"
#include "str.h"
#include "types.h"
#include "rcon.h"
#include "codes.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <thread>
#include <exception>
#include <stdexcept>

#include <map>
#include <array>

#include <ctime>

#include <chrono>

using namespace oastats;
using namespace oastats::types;
using namespace oastats::string;

typedef st_clk clock_p;
typedef clock_p::period period_p;
typedef clock_p::time_point time_p;


// Console output
#define con(m) do{std::cout << m << std::endl;}while(false)


// STACK TRACE
#include <cstdio>
#include <cstdlib>
#include <sys/wait.h>
#include <unistd.h>

#include <sys/resource.h>
#include <cassert>

struct stats
{
	siz_map kills;
	siz_map deaths;
	siz_map flags;
	siz_map awards;

	time_t first_seen;
	time_t logged_time;

	stats(): kills(), deaths(), flags(), awards(), first_seen(0), logged_time(0) {}
};

bool usage()
{
	std::cout << "Usage: " << '\n';
	return -1;
}

typedef std::array<char, 16> GUID;

sos& operator<<(sos& os, const GUID& guid)
{
	for(char c: guid)
		os << c;
	return os;
}

sos& operator<<(sos&& os, const GUID& guid)
{
	return os << guid;
}

sis& operator>>(sis& is, GUID& guid)
{
	for(char& c: guid)
		is.get(c);
	return is;
}

sis& operator>>(sis&& is, GUID& guid)
{
	return is >> guid;
}

const GUID null_guid = {};

void test()
{
	GUID guid;
//	siss("FD3FED56A7F7FB2A") >> guid;
	guid = to<GUID>("FD3FED56A7F7FB2A");
}

typedef std::map<GUID, str> guid_str_map;
typedef std::pair<const GUID, str> guid_str_pair;

typedef std::map<siz, GUID> siz_guid_map;
typedef std::pair<const siz, GUID> siz_guid_pair;

typedef std::map<GUID, siz> guid_siz_map;
typedef std::pair<const GUID, siz> guid_siz_pair;

typedef std::map<GUID, stats> guid_stat_map;
typedef std::pair<const GUID, stats> guid_stat_pair;

void testxxx()
{
	siz_guid_map m;
	GUID guid;
	m[0] = guid;
}

//typedef std::map<str, str> str_str_map;
//typedef std::pair<const str, str> str_str_pair;
//typedef std::map<siz, str> siz_str_map;
//typedef std::pair<const siz, str> siz_str_pair;

typedef std::map<GUID, guid_siz_map> onevone_map;
typedef std::pair<const GUID, guid_siz_map> onevone_pair;

typedef std::multimap<siz, str> siz_str_mmap;
typedef siz_str_mmap::reverse_iterator siz_str_mmap_ritr;

//typedef std::pair<const str, siz> str_siz_pair;

//siz_str_map clients; // slot -> GUID
//str_str_map players; // GUID -> name
//onevone_map onevone; // GUID -> GUID -> <count> //

class RCon
{
private:
	str host;
	siz port;
	str pass;

public:
	RCon(const str& host, siz port, const str& pass): host(host), port(port), pass(pass) {}

	str chat(const str& msg) const
	{
		str ret;
		net::rcon("rcon " + pass + " chat ^1K^7at^3i^7na^8: ^7" + msg, ret, host, port);
		return ret;
	}

	void cp(const str& msg) const
	{
		str ret;
		net::rcon("rcon " + pass + " cp " + msg, ret, host, port);
	}

};

void report_clients(const siz_guid_map& clients)
{
	for(const siz_guid_pair& c: clients)
		con("slot: " << c.first << ", " << c.second);
}

void report_players(const guid_str_map& players)
{
	for(const guid_str_pair& p: players)
		con("player: " << p.first << ", " << p.second);
}

void report_onevone(const onevone_map& onevone, const guid_str_map& players)
{
	for(const onevone_pair& o: onevone)
		for(const guid_siz_pair& p: o.second)
		{
			str p1 = players.at(o.first);
			str p2 = players.at(p.first);
			con("player: " << p1 << " killed " << p2 << " " << p.second << " times.");
		}
}

typedef std::multimap<siz, GUID> siz_guid_mmap;
typedef std::pair<const siz, GUID> siz_guid_pair;
typedef siz_guid_mmap::reverse_iterator siz_guid_mmap_ritr;

void report_caps(const RCon& rcon, const guid_siz_map& caps, const guid_str_map& players)
{
	siz_guid_mmap sorted;
	for(const guid_siz_pair& c: caps)
		sorted.insert(siz_guid_pair(c.second, c.first));

	siz i = 0;
	siz d = 1;
	siz max = 0;
	siz flags = 0;
	str_vec results;
	std::ostringstream oss;
	for(siz_guid_mmap_ritr ri = sorted.rbegin(); ri != sorted.rend(); ++ri)
	{
		++i;
		if(flags != ri->first)
			{ d = i; flags = ri->first; }
		oss.str("");
		oss << "^3#" << d << " ^7" << players.at(ri->second) << " ^3capped ^7" << ri->first << "^3 flags.";
		results.push_back(oss.str());
		if(oss.str().size() > max)
			max = oss.str().size();
	}

	rcon.chat("^5== ^6RESULTS ^5" + str(max - 23, '='));
	for(const str& s: results)
		rcon.chat(s);
	rcon.chat("^5" + str(max - 12, '-'));
}

siz map_get(const siz_map& m, siz key)
{
	return m.find(key) == m.cend() ? 0 : m.at(key);
}

void report_stats(const guid_stat_map& stats, const guid_str_map& players)
{
	for(const guid_stat_pair& p: stats)
	{
		const str& player = players.at(p.first);
		con("player: " << player);
		con("\t caps: " << map_get(p.second.flags, FL_CAPTURED));
		con("\tkills: " << map_get(p.second.kills, MOD_RAILGUN));
		con("\t defs: " << map_get(p.second.awards, AW_DEFENCE));
		con("\tgaunt: " << map_get(p.second.awards, AW_GAUNTLET));
	}
}

inline void delay(siz msecs)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(msecs));
}

void save_records(const str_map& recs)
{
	std::ofstream ofs(str(getenv("HOME")) + "/.katina/high-scores.txt");

	str sep;
	for(const str_pair& r: recs)
		{ ofs << sep << r.first << ": " << r.second; sep = "\n"; }
}

void load_records(str_map& recs)
{
	std::ifstream ifs(str(getenv("HOME")) + "/.katina/high-scores.txt");

	recs.clear();
	str key;
	str val;
	while(std::getline(std::getline(ifs, key, ':') >> std::ws, val))
		recs[key] = val;
}

int main(const int argc, const char* argv[])
{
	str_map recs; // high scores
	load_records(recs);

	log("Records loaded: " << recs.size());

	sifs ifs;
	if(argc > 1)
		ifs.open(argv[1], std::ios::ate);

	sis& is = (argc > 1) ? ifs : std::cin;

	if(!is)
	{
		std::cout << "Input error: " << '\n';
		return -2;
	}

	RCon rcon(recs["rcon.host"], to<siz>(recs["rcon.port"]), recs["rcon.pass"]);

	rcon.chat("^3Stats system v^70.1^3-alpha - ^1ONLINE");

	std::time_t time = 0;
	char c;
	siz m, s;
	str skip, name, cmd, stamp;
	siz secs = 0;
	std::istringstream iss;
	bool in_game = false;

	siz flags[2];

	siz_guid_map clients; // slot -> GUID
	guid_str_map players; // GUID -> name
	onevone_map onevone; // GUID -> GUID -> <count> //
	guid_siz_map caps; // GUID -> <count> // TODO: caps container duplicated in stats::caps
	guid_stat_map stats; // GUID -> <stat>
	str mapname; // current map name

	typedef std::map<str, time_t> str_utime_map; // GUID -> time
	typedef std::pair<const str, time_t> str_utime_pair;
//	str_utime_map dash[2];
	time_p dash[2];// = {0, 0}; // time of dash start
	GUID dasher[2]; // who is dashing
	bool dashing[2] = {true, true}; // flag dash in progress?

	const str flag[2] = {"^1RED", "^4BLUE"};

	bool done = false;
	std::ios::streampos pos = is.tellg();
	str line;
	while(!done)
	{
		if(!std::getline(is, line) || is.eof())
			{ delay(10); is.clear(); is.seekg(pos); continue; }
		pos = is.tellg();

		bug("line: " << line);

		iss.clear();
		iss.str(line);
		iss >> m >> c >> s >> cmd;
		secs = (m * 60) + s;
		bug("cmd: " << cmd);
		if(in_game)
		{
			if(cmd == "Exit:")
			{
				rcon.chat("^3Exit:");
				in_game = false;

				try
				{
					if(!caps.empty())
						report_caps(rcon, caps, players);

					// report
					con("Report:");
					report_clients(clients);
					con("");
					report_players(players);
					con("");
					report_onevone(onevone, players);
					con("");
					report_stats(stats, players);
					con("");
				}
				catch(std::exception& e)
				{
					con(e.what());
				}
			}
			else if(cmd == "ShutdownGame:")
			{
				rcon.chat("^ShutdownGame:");
				in_game = false;
			}
			else if(cmd == "Warmup:")
			{
				in_game = false;
			}
			else if(cmd == "ClientUserinfoChanged:")
			{
				//do_rcon("^3ClientUserinfoChanged:");
				// 0:23 ClientUserinfoChanged: 2 n\^1S^2oo^3K^5ee\t\2\model\ayumi/red\hmodel\ayumi/red\g_redteam\\g_blueteam\\c1\1\c2\1\hc\100\w\0\l\0\tt\0\tl\1\id\1A7C66BACBA16F0C9068D8B82C1D55DE
				siz num;
				if(!std::getline(std::getline(iss >> num, skip, '\\'), name, '\\'))
				{
					std::cout << "Error parsing ClientUserinfoChanged: "  << line << '\n';
					continue;
				}
				siz pos = line.find("id\\");
				if(pos != str::npos)
				{
					str guid = line.substr(pos + 3, 32);
					bug("guid: " << guid);
					if(guid.size() != 32)
						clients[num] = null_guid;
					else
						clients[num] = to<GUID>(guid);
					players[clients[num]] = name;
					if(stats[clients[num]].first_seen)
						stats[clients[num]].logged_time += std::time(0) - stats[clients[num]].first_seen;
					stats[clients[num]].first_seen = time + secs;
				}
			}
			else if(cmd == "Kill:")
			{
				//bug("Kill:");
				// 3:20 Kill: 2 1 10: ^1S^2oo^3K^5ee killed Neko by MOD_RAILGUN
				siz num1, num2, weap;
				if(!(iss >> num1 >> num2 >> weap))
				{
					std::cout << "Error parsing Kill:" << '\n';
					continue;
				}

				if(weap == MOD_SUICIDE)
				{
					// 14:22 Kill: 2 2 20: ^1S^2oo^3K^5ee killed ^1S^2oo^3K^5ee by MOD_SUICIDE
					for(siz i: {0, 1})
					{
						if(dasher[i] == clients[num1])
						{
							dasher[i] = null_guid; // end current dash (if exists)
							dashing[i] = true; // (re) enable dashing
						}
					}
				}
				else if(clients.find(num1) != clients.end() && clients.find(num2) != clients.end())
				{
					if(num1 == 1022) // no killer
						++stats[clients[num2]].deaths[weap];
					else
					{
						++stats[clients[num1]].kills[weap];
						++stats[clients[num2]].deaths[weap];
						++onevone[clients[num1]][clients[num2]];
					}
				}
			}
			else if(cmd == "CTF:")
			{
//				do_rcon("^3CTF:");
				// 10:26 CTF: 0 2 1: ^5A^6lien ^5S^6urf ^5G^6irl captured the BLUE flag!
				// 0 = got, 1 = cap, 2 = ret
				siz num, col, act;
				if(!(iss >> num >> col >> act) || col < 1 || col > 2)
				{
					std::cout << "Error parsing CTF:" << '\n';
					continue;
				}

				--col; // make 0-1 for array index
				siz ncol = col ? 0 : 1;

				bug("inc stats");
				++stats[clients[num]].flags[act];

				if(act == FL_CAPTURED) // In Game Announcer
				{
					bug("FL_CAPTURED");
					if(dashing[col] && dasher[col] != null_guid)
					{
//						time_p now = clock_p::now();
						auto diff = clock_p::now() - dash[col];
						double sec = double(diff.count() * period_p::num) / period_p::den;
						std::ostringstream oss;
						oss.precision(2);
						oss << std::fixed << sec;
						rcon.chat(players[clients[num]] + "^3 took ^7" + oss.str()
							+ "^3 seconds to capture the " + flag[col] + "^3 flag.");

						double rec = to<double>(recs["dash." + mapname + ".secs"]);

						bug("rec: " << rec);

						if(rec < 0.5)
						{
							rcon.chat(players[clients[num]] + "^3 has set the record for this map.");
							recs["dash." + mapname + ".guid"] = to<str>(clients[num]);
							recs["dash." + mapname + ".name"] = players[clients[num]];
							recs["dash." + mapname + ".secs"] = oss.str();
							save_records(recs);
						}
						else if(sec < rec)
						{
							rcon.chat(players[clients[num]] + "^3 beat "
								+ recs["dash." + mapname + ".name"] + "'s "
								+ recs["dash." + mapname + ".secs"] + " seconds.");
							recs["dash." + mapname + ".guid"] = to<str>(clients[num]);
							recs["dash." + mapname + ".name"] = players[clients[num]];
							recs["dash." + mapname + ".secs"] = oss.str();
							save_records(recs);
						}
					}
//					do_rcon("^1DEBUG:^3 End dash & (re)enable dashing of the " + flag[col] + "^3 flag.");
					dasher[col] = null_guid;;
					dashing[col] = true; // new dash now possible
					++flags[col];
					++caps[clients[num]];
					rcon.cp(players[clients[num]] + "^3 has ^7" + std::to_string(caps[clients[num]]) + "^3 flag" + (caps[clients[num]]==1?"":"s") + "!");
				}
				else if(act == FL_TAKEN)
				{
					bug("FL_TAKEN");
					if(dashing[col])
					{
//						rcon.chat("^1DEBUG:^3 Begin dash with the " + flag[col] + "^3 flag.");
						dash[col] = clock_p::now();//std::clock();//std::time(0); // begin a dash
					}
					dasher[col] = clients[num];
				}
				else if(act == FL_DROPPED)
				{
					bug("FL_DROPPED");
//					rcon.chat("^1DEBUG:^3 End dash & disable dashing for the " + flag[ncol] + "^3 flag.");
					dasher[ncol] = null_guid;; // end a dash
					dashing[ncol] = false; // no more dashes until return, capture or suicide
				}
				else if(act == FL_RETURNED)
				{
					bug("FL_RETURNED");
//					rcon.chat("^1DEBUG:^3 (Re)enable dashing for the " + flag[col] + "^3 flag.");
					dasher[col] = null_guid;; // end a dash
					dashing[col] = true; // new dash now possible
				}
			}
			else if(cmd == "Award:")
			{
				// 0:37 Award: 1 3: (drunk)Mosey gained the DEFENCE award!
				siz num, awd;
				if(!(iss >> num >> awd))
				{
					std::cout << "Error parsing Award:" << '\n';
					continue;
				}
				++stats[clients[num]].awards[awd];
			}
		}
		else
		{
			if(cmd == "InitGame:")
			{
				rcon.chat("^3InitGame:");

				time = std::time(0);
				in_game = true;
				flags[FL_RED] = 0;
				flags[FL_BLUE] = 0;

				clients.clear();
				players.clear();
				onevone.clear();
				caps.clear();
				stats.clear();
				dasher[FL_RED] = null_guid;;
				dasher[FL_BLUE] = null_guid;;
				dashing[FL_RED] = true;
				dashing[FL_BLUE] = true;

				rcon.cp("^1K^7at^3i^7na ^3Stats system v^70.1^3-alpha.");

				siz pos;
				if((pos = line.find("mapname\\")) != str::npos)
				{
					mapname.clear();
					std::istringstream iss(line.substr(pos + 8));
					if(!std::getline(iss, mapname, '\\'))
					{
						std::cout << "Error parsing mapname\\" << '\n';
						continue;
					}
					bug("mapname: " << mapname);
				}
			}
		}
		if(cmd == "say:")
		{
			// 0:23 say: ^5A^6lien ^5S^6urf ^5G^6irl: yes, 3-4 players max
			bug("line: " << line);
			siz pos;
			if((pos = line.find_last_of(':')) != str::npos)
			{
				bug("pos: " << pos);
				if((pos = line.find('!', pos)) != str::npos)
				{
					bug("pos: " << pos);
					str cmd = line.substr(pos);
					bug("cmd: " << cmd);
					if(cmd == "!record")
					{
						bug("mapname: " << mapname);

						rcon.chat("^3MAP RECORD: ^7"
							+ recs["dash." + mapname + ".secs"]
							+ "^3 set by ^7" + recs["dash." + mapname + ".name"]);
					}

				}
			}
		}
	}
}
