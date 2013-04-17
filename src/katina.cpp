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

#include "codes.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <exception>
#include <stdexcept>
#include <algorithm>
#include <memory>

#include <map>
#include <ctime>
#include <cctype>

#include <sys/time.h>
#include <unistd.h>
#include <wordexp.h>

#include <pthread.h>

// STACK TRACE
#include <cstdio>
#include <cstdlib>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <execinfo.h>
#include <cxxabi.h>

#include <sys/resource.h>
#include <cassert>

#include <vector>
#include <map>
#include <set>

#include <pthread.h>

#include "types.h"
#include "log.h"
#include "socketstream.h"
#include "str.h"
#include "rcon.h"
#include "time.h"
#include "RemoteClient.h"
#include "Database.h"
#include "GUID.h"

#include <arpa/inet.h> // IP to int

using namespace oastats;
using namespace oastats::data;
using namespace oastats::log;
using namespace oastats::types;
using namespace oastats::string;
using namespace oastats::net;
using namespace oastats::time;

const std::string version = "0.5.4";
const std::string tag = "alpha";

/*
 * Create a GUID for bots based on their slot number
 */
GUID bot_guid(siz num)
{
	soss oss;
	oss << num;
	str id = oss.str();
	if(id.size() < GUID::SIZE)
		id = str(GUID::SIZE - id.size(), '0') + id;

	return GUID(id.c_str());
}

struct stats
{
	siz_map kills;
	siz_map deaths;
	siz_map flags;
	siz_map awards;

	time_t joined_time;
	siz logged_time;

	stats(): kills(), deaths(), flags(), awards(), joined_time(0), logged_time(0) {}
};

typedef std::map<GUID, stats> guid_stat_map;
typedef std::pair<const GUID, stats> guid_stat_pair;
typedef std::map<GUID, stats>::iterator guid_stat_iter;
typedef std::map<GUID, stats>::const_iterator guid_stat_citer;

RCon server;
SkivvyClient skivvy;
Database db;

/**
 * Set a variable from a cvar using rcon.
 * @param cvar The name of the cvar whose value is wanted
 * @param val The variable to set to the cvar's value.
 */
bool rconset(const str& cvar, str& val)
{
	str response;
	if(!server.command(cvar, response))
	{
		log("WARN: rconset failure: " << cvar);
		return false;
	}

	// Possible responses:
	// -> unknown command: <var>
	// -> "<var>" is:"<val>^7", the default
	// -> "katina_skivvy_chans" is:"#katina-test(c) #katina(c)^7" default:"#katina-test(c)^7"

	str sval;

	if(response.find("unknown command:"))
	{
		str skip;
		siss iss(response);
		if(!std::getline(std::getline(iss, skip, ':').ignore(), sval, '^'))
		{
			log("ERROR: parsing rconset response: " << response);
			return false;
		}
	}

	val = sval;
	return true;
}

/**
 * Set a variable from a cvar using rcon.
 * @param cvar The name of the cvar whose value is wanted
 * @param val The variable to set to the cvar's value.
 */
template<typename T>
bool rconset(const str& cvar, T& val)
{
	str sval;
	if(!rconset(cvar, sval))
		return false;
	siss iss(sval);
	return iss >> val;
}

str_vec weapons;

// teams thread

pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
struct thread_data
{
	milliseconds delay;
	siz_guid_map* clients_p;
	guid_siz_map* teams_p;
};

bool done = false;

struct katina_conf
{
	bool active;
	bool do_flags;
	bool do_dashes;
	bool do_db; // do database writes
	std::set<siz> db_weaps; // which weapons to record

	katina_conf()
	: active(false)
	, do_flags(false)
	, do_dashes(false)
	, do_db(false)
	{
	}
};

struct skivvy_conf
{
	bool active;
	bool do_flags;
	bool do_flags_hud;
	bool do_chats;
	bool do_kills;
	bool do_infos;
	bool do_stats;
	bool spamkill;
	str chans;

	skivvy_conf()
	: active(false)
	, do_flags(false)
	, do_flags_hud(false)
	, do_chats(false)
	, do_kills(false)
	, do_infos(false)
	, do_stats(false)
	, spamkill(false)
	{
	}
};


//bool katina_active = false;
katina_conf ka_cfg;
skivvy_conf sk_cfg;

str_map recs; // high scores/ config etc

siz_guid_map clients; // slot -> GUID
guid_str_map players; // GUID -> name
onevone_map onevone; // GUID -> GUID -> <count> //
guid_siz_map caps; // GUID -> <count> // TODO: caps container duplicated in stats::caps
guid_stat_map stats; // GUID -> <stat>
guid_siz_map teams; // GUID -> 'R' | 'B'
str mapname, old_mapname; // current/previous map name

guid_int_map map_votes; // GUID -> 3

const str flag[2] = {"^1RED", "^4BLUE"};

void report_clients(const siz_guid_map& clients)
{
	for(siz_guid_citer i = clients.begin(); i != clients.end(); ++i)
		con("slot: " << i->first << ", " << i->second);
}

void report_players(const guid_str_map& players)
{
	for(guid_str_citer i = players.begin(); i != players.end(); ++i)
		con("player: " << i->first << ", " << i->second);
}

//void report_onevone(const onevone_map& onevone, const guid_str_map& players)
//{
//	for(onevone_citer o = onevone.begin(); o != onevone.end(); ++o)
//		for(guid_siz_citer p = o->second.begin(); p != o->second.end(); ++p)
//		{
//			str p1 = players.at(o->first);
//			str p2 = players.at(p->first);
//			con("player: " << p1 << " killed " << p2 << " " << p->second << " times.");
//		}
//}

typedef std::multimap<siz, GUID> siz_guid_mmap;
//typedef std::pair<const siz, GUID> siz_guid_pair;
typedef siz_guid_mmap::reverse_iterator siz_guid_mmap_ritr;

void report_caps(const guid_siz_map& caps, const guid_str_map& players, siz flags[2])
{
	siz_guid_mmap sorted;
	for(guid_siz_citer c = caps.begin(); c != caps.end(); ++c)
		sorted.insert(siz_guid_pair(c->second, c->first));

	siz i = 0;
	siz d = 1;
	siz max = 0;
	siz f = 0; // flags
	str_vec results;
	std::ostringstream oss;
	for(siz_guid_mmap_ritr ri = sorted.rbegin(); ri != sorted.rend(); ++ri)
	{
		++i;
		if(f != ri->first)
			{ d = i; f = ri->first; }
		oss.str("");
		oss << "^3#" << d << " ^7" << players.at(ri->second) << " ^3capped ^7" << ri->first << "^3 flags.";
		results.push_back(oss.str());
		if(oss.str().size() > max)
			max = oss.str().size();
	}

	server.chat("^5== ^6RESULTS ^5" + str(max - 23, '='));
	for(siz i = 0; i < results.size(); ++i)
		server.chat(results[i]);
	server.chat("^5" + str(max - 12, '-'));

	if(sk_cfg.do_infos)
	{
//		skivvy.chat('f', "^1RED^3: ^7" + to_string(flags[FL_BLUE]) + " ^3v ^4BLUE^3: ^7" + to_string(flags[FL_RED]));
		skivvy.chat('i', "^5== ^6RESULTS ^5== ^7"
			+ to_string(flags[FL_BLUE]) + " ^1RED ^7"
			+ to_string(flags[FL_RED]) + " ^4BLUE ^3 ==");
//		skivvy.chat('i', "^5== ^6RESULTS ^5" + str(max - 23, '='));
		for(siz i = 0; i < results.size(); ++i)
			skivvy.chat('f', results[i]);
		skivvy.chat('i', "^5" + str(max - 12, '-'));
	}
}

siz map_get(const siz_map& m, siz key)
{
	return m.find(key) == m.end() ? 0 : m.at(key);
}

void report_stats(const guid_stat_map& stats, const guid_str_map& players)
{
	std::multimap<double, str> skivvy_scores;

	for(guid_stat_citer p = stats.begin(); p != stats.end(); ++p)
	{
		const str& player = players.at(p->first);
		con("player: " << player);
		con("\t  caps: " << map_get(p->second.flags, FL_CAPTURED));
		con("\t kills: " << map_get(p->second.kills, MOD_RAILGUN));
		con("\tdeaths: " << map_get(p->second.deaths, MOD_RAILGUN));
		con("\t  defs: " << map_get(p->second.awards, AW_DEFENCE));
		con("\t gaunt: " << map_get(p->second.awards, AW_GAUNTLET));
		con("\t  time: " << p->second.logged_time << 's');
		// TODO: modify this to add AW options as well as insta
		if(sk_cfg.do_stats)
		{
			siz c = map_get(p->second.flags, FL_CAPTURED);
			siz k = map_get(p->second.kills, MOD_RAILGUN);
			k += map_get(p->second.kills, MOD_GAUNTLET);
			siz d = map_get(p->second.deaths, MOD_RAILGUN);
			d += map_get(p->second.deaths, MOD_GAUNTLET);

			con("c: " << c);
			con("k: " << k);
			con("d: " << d);

			double rkd = 0.0;
			double rcd = 0.0;
			str kd, cd;
			if(!d)
			{
				if(k)
					kd = "perf  ";
				if(c)
					cd = "perf  ";
			}
			else
			{
				rkd = double(k) / d;
				rcd = double(c * 100) / d;
				kd = to_string(rkd, 6);
				cd = to_string(rcd, 6);
			}
			if(k || c || d)
			{
				str mins, secs;
				siz m = p->second.logged_time / 60;
				siz s = p->second.logged_time % 60;
				soss oss;
				oss << m;
				mins = oss.str();
				oss.str("");
				oss << s;
				secs = oss.str();
				if(mins.size() < 3)
					mins = str(3 - mins.size(), ' ') + mins;
				if(secs.size() < 2)
					secs = str(2 - secs.size(), '0') + secs;
				oss.str("");
				oss << "^3time: ^7" << mins << "^3:^7" << secs << " " << "^3kills^7/^3d ^5(^7" << kd << "^5) ^3caps^7/^3d ^5(^7" << cd << "^5)^7: " + player;
				skivvy_scores.insert(std::make_pair(rkd, oss.str()));
			}
		}
	}
	if(sk_cfg.do_stats)
		for(std::multimap<double, str>::reverse_iterator r = skivvy_scores.rbegin(); r != skivvy_scores.rend(); ++r)
			skivvy.chat('s', r->second);
}

void save_records(const str_map& recs)
{
	log("save_records:");
	std::ofstream ofs((str(getenv("HOME")) + "/.katina/records.txt").c_str());

	str sep;
	for(str_map_citer r = recs.begin(); r != recs.end(); ++r)
		{ ofs << sep << r->first << ": " << r->second; sep = "\n"; }
}

void load_records(str_map& recs)
{
	log("load_records:");
	std::ifstream ifs((str(getenv("HOME")) + "/.katina/records.txt").c_str());

	recs.clear();
	str key;
	str val;
	while(std::getline(std::getline(ifs, key, ':') >> std::ws, val))
		recs[key] = val;
}

void* set_teams(void* td_vp)
{
	thread_data& td = *reinterpret_cast<thread_data*>(td_vp);
	siz_guid_map& clients = *td.clients_p;
	guid_siz_map& teams = *td.teams_p;

	if(td.delay < 3000)
		td.delay = 3000;

	while(!done)
	{
		thread_sleep_millis(td.delay);

		// cvar controls

		static siz c = 0;

		katina_conf old_ka_cfg = ka_cfg;
		skivvy_conf old_sk_cfg = sk_cfg;
		str cvar;
		siss iss;
		siz weap;

		switch(c++)
		{
			case 0:
				if(!rconset("katina_active", ka_cfg.active))
					rconset("katina_active", ka_cfg.active); // one retry
				if(ka_cfg.active != old_ka_cfg.active)
				{
					log("katina: " + str(ka_cfg.active?"":"de-") + "activated");
					server.chat("^3going ^1" + str(ka_cfg.active?"on":"off") + "-line^3.");
					skivvy.chat('*', "^3going ^1" + str(ka_cfg.active?"on":"off") + "-line^3.");
				}
			break;
			case 1:
				if(!rconset("katina_flags", ka_cfg.do_flags))
					rconset("katina_flags", ka_cfg.do_flags); // one retry
				if(ka_cfg.do_flags != old_ka_cfg.do_flags)
				{
					log("katina: flag counting is now: " << (ka_cfg.do_flags ? "on":"off"));
					server.chat( "^3Flag countng ^1" + str(ka_cfg.do_flags ? "on":"off") + "^3.");
					skivvy.chat('f', "^3Flag countng ^1" + str(ka_cfg.do_flags ? "on":"off") + "^3.");
				}
			break;
			case 2:
				if(!rconset("katina_dashes", ka_cfg.do_dashes))
					rconset("katina_dashes", ka_cfg.do_dashes); // one retry
				if(ka_cfg.do_dashes != old_ka_cfg.do_dashes)
				{
					log("katina: flag timing is now: " << (ka_cfg.do_dashes ? "on":"off"));
					server.chat("^3Flag timing ^1" + str(ka_cfg.do_dashes ? "on":"off") + "^3.");
					skivvy.chat('f', "^3Flag timing ^1" + str(ka_cfg.do_dashes ? "on":"off") + "^3.");
				}
			break;
			case 3:
				if(!rconset("katina_db_active", ka_cfg.do_db))
					rconset("katina_db_active", ka_cfg.do_db); // one retry
				if(ka_cfg.do_db != old_ka_cfg.do_db)
				{
					log("katina: database writing is now: " << (ka_cfg.do_db ? "on":"off"));
					skivvy.chat('*', "^3Flag timing ^1" + str(ka_cfg.do_db ? "on":"off") + "^3.");
					if(!ka_cfg.do_db)
						db.off();
					else
					{
						db.on();
						db.read_map_votes(mapname, map_votes);
					}
				}
			break;
			case 4:
				if(!rconset("katina_db_weaps", cvar))
					if(!rconset("katina_db_weaps", cvar))
						break;

				iss.clear();
				iss.str(cvar);
				while(iss >> weap)
					ka_cfg.db_weaps.insert(weap);

				if(ka_cfg.db_weaps != old_ka_cfg.db_weaps)
				{
					log("katina: database weaps set to: " << cvar);
					skivvy.chat('*', "^3Database weapons set to: ^1" + cvar + "^3.");
				}
			break;
			case 5:
				if(!rconset("katina_skivvy_active", sk_cfg.active))
					rconset("katina_skivvy_active", sk_cfg.active); // one retry
				if(sk_cfg.active != old_sk_cfg.active)
				{
					if(sk_cfg.active)
					{
						log("skivvy: reporting activated");
						skivvy.chat('*', "^3reporting turned on.");
						skivvy.on();
					}
					else
					{
						log("skivvy: reporting deactivated");
						skivvy.chat('*', "^3reporting turned off.");
						skivvy.off();
					}
				}
			break;
			case 6:
				if(!rconset("katina_skivvy_chans", sk_cfg.chans))
					rconset("katina_skivvy_chans", sk_cfg.chans); // one retry
				if(old_sk_cfg.chans != sk_cfg.chans)
				{
					log("skivvy: new chans: " << sk_cfg.chans);
					skivvy.set_chans(sk_cfg.chans);
					skivvy.chat('*', "^3Now reporting to ^7" + sk_cfg.chans);
				}
			break;
			case 7:
				if(!rconset("katina_skivvy_chats", sk_cfg.do_chats))
					rconset("katina_skivvy_chats", sk_cfg.do_chats); // one retry
				if(sk_cfg.do_chats != old_sk_cfg.do_chats)
				{
					log("skivvy: chat reporting is now: " << (sk_cfg.do_chats ? "on":"off"));
					skivvy.chat('*', "^3Chat reports ^1" + str(sk_cfg.do_chats ? "on":"off") + "^3.");
				}
			break;
			case 8:
				if(!rconset("katina_skivvy_flags", sk_cfg.do_flags))
					rconset("katina_skivvy_flags", sk_cfg.do_flags); // one retry
				if(sk_cfg.do_flags != old_sk_cfg.do_flags)
				{
					log("skivvy: flag reporting is now: " << (sk_cfg.do_flags ? "on":"off"));
					skivvy.chat('*', "^3Flag reports ^1" + str(sk_cfg.do_flags ? "on":"off") + "^3.");
				}
			break;
			case 9:
				if(!rconset("katina_skivvy_flags_hud", sk_cfg.do_flags_hud))
					rconset("katina_skivvy_flags_hud", sk_cfg.do_flags_hud); // one retry
				if(sk_cfg.do_flags_hud != old_sk_cfg.do_flags_hud)
				{
					log("skivvy: flag HUD is now: " << (sk_cfg.do_flags_hud ? "on":"off"));
					skivvy.chat('*', "^3Flag HUD ^1" + str(sk_cfg.do_flags_hud ? "on":"off") + "^3.");
				}
			break;
			case 10:
				if(!rconset("katina_skivvy_kills", sk_cfg.do_kills))
					rconset("katina_skivvy_kills",sk_cfg. do_kills); // one retry
				if(sk_cfg.do_kills != old_sk_cfg.do_kills)
				{
					log("skivvy: kill reporting is now: " << (sk_cfg.do_kills ? "on":"off"));
					skivvy.chat('*', "^3Kill reports ^1" + str(sk_cfg.do_kills ? "on":"off") + "^3.");
				}
			break;
			case 11:
				if(!rconset("katina_skivvy_infos", sk_cfg.do_infos))
					rconset("katina_skivvy_infos", sk_cfg.do_infos); // one retry
				if(sk_cfg.do_kills != old_sk_cfg.do_kills)
				{
					log("skivvy: info reporting is now: " << (sk_cfg.do_infos ? "on":"off"));
					skivvy.chat('*', "^3Info reports ^1" + str(sk_cfg.do_infos ? "on":"off") + "^3.");
				}
			break;
			case 12:
				if(!rconset("katina_skivvy_stats", sk_cfg.do_stats))
					rconset("katina_skivvy_stats", sk_cfg.do_stats); // one retry
				if(sk_cfg.do_stats != old_sk_cfg.do_stats)
				{
					log("skivvy: stats reporting is now: " << (sk_cfg.do_stats ? "on":"off"));
					skivvy.chat('*', "^3Stats reports ^1" + str(sk_cfg.do_stats ? "on":"off") + "^3.");
				}
			break;
			case 13:
				if(!rconset("katina_skivvy_spamkill", sk_cfg.spamkill))
					rconset("katina_skivvy_spamkill", sk_cfg.spamkill); // one retry
				if(old_sk_cfg.spamkill != sk_cfg.spamkill)
				{
					log("skivvy: spamkill is now: " << (sk_cfg.spamkill ? "on":"off"));
					skivvy.chat('*', "^3Spamkill ^1" + str(sk_cfg.spamkill ? "on":"off") + "^3.");
				}
			break;
			default:
				c = 0;
			break;
		}

//		if(!sk_cfg.active || !sk_cfg.active)
//			continue;
//
//		str reply;
//		if(server.command("!listplayers", reply))
//		{
//			trim(reply);
//			// !listplayers: 4 players connected:
//			//  1 R 0   Unknown Player (*)   Major
//			//  2 B 0   Unknown Player (*)   Tony
//			//  4 B 0   Unknown Player (*)   Sorceress
//			//  5 R 0   Unknown Player (*)   Sergei
//			if(!reply.empty())
//			{
//				siz n;
//				char team;
//				siss iss(reply);
//				str line;
//				std::getline(iss, line); // skip command
//				std::time_t now = std::time(0);
//				while(std::getline(iss, line))
//				{
//					//bug("\t\tline: " << line);
//					siss iss(line);
//					if(iss >> n >> team)
//					{
//						pthread_mutex_lock(&mtx);
//						// TODO: WHAt ABOUT GAME QUITTERS? How to detect that?
//						if(!clients[n].is_bot() && team != teams[clients[n]])
//						{
//							if((team == 'R' || team == 'B')
//							&& (teams[clients[n]] != 'R' && teams[clients[n]] != 'B'))
//							{
//								// joined game
//								bug("TIMER:        start: " << clients[n]);
//								bug("     : current time: " << stats[clients[n]].logged_time);
//
//								stats[clients[n]].joined_time = now;
//							}
//							else if((team != 'R' && team != 'B')
//							&& (teams[clients[n]] == 'R' || teams[clients[n]] == 'B'))
//							{
//								// parted from game
//								if(stats[clients[n]].joined_time) // 0 for new record
//									stats[clients[n]].logged_time += now - stats[clients[n]].joined_time;
//								stats[clients[n]].joined_time = 0; // stop counting time
//
//								bug("TIMER:         stop: " << clients[n]);
//								bug("     : current time: " << stats[clients[n]].logged_time);
//							}
//						}
//						teams[clients[n]] = team;
//						pthread_mutex_unlock(&mtx);
//					}
//				}
//			}
//		}
	}
	pthread_exit(0);
}

GUID guid_from_name(const str& name)
{
	for(guid_str_iter i = players.begin(); i != players.end(); ++i)
		if(i->second == name)
			return i->first;
	return null_guid;
}

bool extract_name_from_text(const str& line, GUID& guid, str& text)
{
	GUID candidate;
	siz pos = 0;
	siz beg = 0;
	if((beg = line.find(": ")) == str::npos) // "say: "
		return false;

	beg += 2;

	bool found = false;
	for(pos = beg; (pos = line.find(": ", pos)) != str::npos; pos += 2)
	{
		if((candidate = guid_from_name(line.substr(beg, pos - beg))) == null_guid)
			continue;
		guid = candidate;
		text = line.substr(pos + 2);
		found = true;
	}
	return found;
}

str expand_env(const str& var)
{
	str exp;
	wordexp_t p;
	wordexp(var.c_str(), &p, 0);
	if(p.we_wordc)
		exp = p.we_wordv[0];
	wordfree(&p);
	return exp;
}

void stack_handler(int sig)
{
	con("Error: signal " << sig);

	void *array[2048];
	size_t size;

	// get void*'s for all entries on the stack
	size = backtrace(array, 2048);

	// print out all the frames to stderr
	char** trace = backtrace_symbols(array, size);

	int status;
	str obj, func;
	for(siz i = 0; i < size; ++i)
	{
		siss iss(trace[i]);
		std::getline(std::getline(iss, obj, '('), func, '+');

		char* func_name = abi::__cxa_demangle(func.c_str(), 0, 0, &status);
		std::cerr << "function: " << func_name << '\n';
		free(func_name);
	}
	free(trace);
	exit(1);
}

str get_hud(siz m, siz s, GUID dasher[2])
{
	soss oss;
	oss << "00[15" << (m < 10?"0":"") << m << "00:15" << (s < 10?"0":"") << s << " ";
	oss << "04" << (dasher[FL_RED] != null_guid?"⚑":".");
	oss << "02" << (dasher[FL_BLUE] != null_guid?"⚑":".");
	oss << "00]";
	return oss.str();
}

const siz TEAM_U = 0;
const siz TEAM_R = 1;
const siz TEAM_B = 2;
const siz TEAM_S = 3;

int main(const int argc, const char* argv[])
{
	signal(11, stack_handler);

	load_records(recs);

	log("Records loaded: " << recs.size());

	sifs ifs;
	if(argc > 1)
		ifs.open(argv[1], std::ios::ate);
	else if(!recs["logfile"].empty())
		ifs.open(expand_env(recs["logfile"]).c_str(), std::ios::ate);

	sis& is = ifs.is_open() ? ifs : std::cin;

	if(!is)
	{
		log("Input error:");
		return -2;
	}

	server.config(recs["rcon.host"], to<siz>(recs["rcon.port"]), recs["rcon.pass"]);
	skivvy.config(recs["skivvy.host"], to<siz>(recs["skivvy.port"]));
	db.config(recs["db.host"], to<siz>(recs["db.port"]), recs["db.user"], recs["db.pass"], recs["db.base"]);

	server.chat("^3Stats System v^7" + version + "^3-" + tag + " - ^1ONLINE");
	skivvy.chat('*', "^3Stats System v^7" + version + "^3-" + tag + " - ^1ONLINE");

	// weapons
	weapons.push_back("unknown weapon");
	weapons.push_back("shotgun");
	weapons.push_back("gauntlet");
	weapons.push_back("machinegun");
	weapons.push_back("grenade");
	weapons.push_back("grenade schrapnel");
	weapons.push_back("rocket");
	weapons.push_back("rocket blast");
	weapons.push_back("plasma");
	weapons.push_back("plasma splash");
	weapons.push_back("railgun");
	weapons.push_back("lightening");
	weapons.push_back("BFG");
	weapons.push_back("BFG fallout");
	weapons.push_back("dround");
	weapons.push_back("slimed");
	weapons.push_back("burnt up in lava");
	weapons.push_back("crushed");
	weapons.push_back("telefraged");
	weapons.push_back("falling to far");
	weapons.push_back("suicide");
	weapons.push_back("target lazer");
	weapons.push_back("inflicted pain");
	weapons.push_back("nailgun");
	weapons.push_back("chaingun");
	weapons.push_back("proximity mine");
	weapons.push_back("kamikazi");
	weapons.push_back("juiced");
	weapons.push_back("grappled");

	std::time_t time = 0;
	char c;
	siz m, s;
	str skip, name, cmd, stamp;
	siz secs = 0;
	std::istringstream iss;
	bool in_game = false;

	str_siz_map spam;
	siz spam_limit = 2;

	siz flags[2];

	milliseconds dash[2];// = {0, 0}; // time of dash start
	GUID dasher[2]; // who is dashing
	bool dashing[2] = {true, true}; // flag dash in progress?

	pthread_t teams_thread;
	milliseconds thread_delay = 6000; // default
	if(recs.count("rcon.delay"))
		thread_delay = to<milliseconds>(recs["rcon.delay"]);
	thread_data td = {thread_delay, &clients, &teams};
	pthread_create(&teams_thread, NULL, &set_teams, (void*) &td);

	milliseconds sleep_time = 100; // milliseconds
	bool done = false;
	std::ios::streampos pos = is.tellg();
	str line;
	while(!done)
	{
		if(ka_cfg.do_dashes)
			sleep_time = 10;
		else
			sleep_time = 100;

		if(!std::getline(is, line) || is.eof())
			{ thread_sleep_millis(sleep_time); is.clear(); is.seekg(pos); continue; }

		pos = is.tellg();

		if(!sk_cfg.active)
			continue;

//		bug("line: " << line);

		iss.clear();
		iss.str(line);
		iss >> m >> c >> s >> cmd;
		secs = (m * 60) + s;
//		bug("cmd: " << cmd);
		std::time_t now = std::time(0);
		if(in_game)
		{
			if(cmd == "Exit:")
			{
				trace(cmd << "(" << (in_game?"playing":"waiting") << ")");
				// shutdown voting until next map
				log("exit: writing stats to database and collecting votes");
//				str reply;
//				if(!server.command("set g_allowVote 0", reply))
//					server.command("set g_allowVote 0", reply); // one retry

				// in game timing
				for(guid_stat_iter i = stats.begin(); i != stats.end(); ++i)
				{
					bug("TIMER:         EOG: " << i->first);
					if(i->second.joined_time);
					{
						bug("TIMER:         ADD: " << i->first);
						bug("TIMER:         now: " << now);
						bug("TIMER: logged_time: " << i->second.logged_time);
						bug("TIMER: joined_time: " << i->second.joined_time);
						i->second.logged_time += now - i->second.joined_time;
						i->second.joined_time = 0;
					}
				}

				skivvy.chat('*', "^3Game Over");
				in_game = false;

				// erase non spam marked messages
				for(str_siz_map_iter i = spam.begin(); i != spam.end();)
				{
					if(i->second < spam_limit)
					{
						spam.erase(i->first);
						i = spam.begin();
					}
					else
						++i;
				}

				try
				{
					if(ka_cfg.do_flags && !caps.empty())
						report_caps(caps, players, flags);

					game_id id = db.add_game(recs["rcon.host"], recs["rcon.port"], mapname);
					bug("id; " << id);
					if(id != null_id && id != bad_id)
					{
						// TODO: insert game stats here
						for(guid_stat_citer p = stats.begin(); p != stats.end(); ++p)
						{
							const str& player = players.at(p->first);

							siz count;
							for(std::set<siz>::iterator weap = ka_cfg.db_weaps.begin(); weap != ka_cfg.db_weaps.end(); ++weap)
							{
								if((count = map_get(p->second.kills, *weap)))
									db.add_weaps(id, "kills", p->first, *weap, count);
								if((count = map_get(p->second.deaths, *weap)))
									db.add_weaps(id, "deaths", p->first, *weap, count);
							}

							if((count = map_get(p->second.flags, FL_CAPTURED)))
								db.add_caps(id, p->first, count);

							if(!p->first.is_bot())
								if((count = p->second.logged_time))
									db.add_time(id, p->first, count);
						}

						for(onevone_citer o = onevone.begin(); o != onevone.end(); ++o)
							for(guid_siz_citer p = o->second.begin(); p != o->second.end(); ++p)
								db.add_ovo(id, o->first, p->first, p->second);
					}

					// report
					con("-- Report: -------------------------------");
					report_clients(clients);
					con("");
					report_players(players);
//					con("");
//					report_onevone(onevone, players);
					con("");
					report_stats(stats, players);
					con("------------------------------------------");
				}
				catch(std::exception& e)
				{
					con(e.what());
				}

				for(guid_str_map::iterator player = players.begin(); player != players.end(); ++player)
					if(!player->first.is_bot())
						db.add_player(player->first, player->second);

				log("exit: done");
			}
			else if(cmd == "ShutdownGame:")
			{
				trace(cmd << "(" << (in_game?"playing":"waiting") << ")");
				in_game = false;

			}
			else if(cmd == "Warmup:")
			{
				trace(cmd << "(" << (in_game?"playing":"waiting") << ")");
				in_game = false;
			}
			else if(cmd == "ClientUserinfoChanged:")
			{
				trace(cmd << "(" << (in_game?"playing":"waiting") << ")");

				// 1:58 ClientUserinfoChanged: 2 n\<name>\t\<team>\model\sar

				//bug("ClientUserinfoChanged:");
				//do_rcon("^3ClientUserinfoChanged:");
				// 0:23 ClientUserinfoChanged: 2 n\^1S^2oo^3K^5ee\t\2\model\ayumi/red\hmodel\ayumi/red\g_redteam\\g_blueteam\\c1\1\c2\1\hc\100\w\0\l\0\tt\0\tl\1\id\1A7C66BACBA16F0C9068D8B82C1D55DE
				siz num, team;
				if(!std::getline(std::getline(std::getline(iss >> num, skip, '\\'), name, '\\'), skip, '\\') >> team)
				{
					std::cout << "Error parsing ClientUserinfoChanged: "  << line << '\n';
					continue;
				}

				//bug("num: " << num);
				//bug("skip: " << skip);
				//bug("name: " << name);

				siz pos = line.find("\\id\\");
				if(pos != str::npos)
				{
					str id = line.substr(pos + 4, 32);

					if(id.size() != 32)
						clients[num] = bot_guid(num);//null_guid;
					else
					{
						GUID guid = to<GUID>(id.substr(24));
						if(clients[num] != guid)
						{
							clients[num] = guid;
							teams[clients[num]] = TEAM_U;
						}
					}

					players[clients[num]] = name;

					if(!clients[num].is_bot() && team != teams[clients[num]])
					{
						if((team == TEAM_R || team == TEAM_B)
						&& (teams[clients[num]] != TEAM_R && teams[clients[num]] != TEAM_B))
						{
							// joined game
							bug("TIMER:        start: " << clients[num]);
							bug("     : current time: " << stats[clients[num]].logged_time);

							stats[clients[num]].joined_time = now;
						}
						else if((team != TEAM_R && team != TEAM_B)
						&& (teams[clients[num]] == TEAM_R || teams[clients[num]] == TEAM_B))
						{
							// parted from game
							if(stats[clients[num]].joined_time) // 0 for new record
								stats[clients[num]].logged_time += now - stats[clients[num]].joined_time;
							stats[clients[num]].joined_time = 0; // stop counting time

							bug("TIMER:         stop: " << clients[num]);
							bug("     : current time: " << stats[clients[num]].logged_time);
						}
						teams[clients[num]] = team; // 1 = red, 2 = blue, 3 = spec
					}
				}
			}
			else if(cmd == "ClientConnect:")
			{
				siz num;
				if((iss >> num))
				{
					stats[clients[num]].joined_time = 0;
					stats[clients[num]].logged_time = 0;
				}
			}
			else if(cmd == "ClientDisconnect:")
			{
				siz num;
				if((iss >> num))
				{
					if(stats[clients[num]].joined_time)
						stats[clients[num]].logged_time += now - stats[clients[num]].joined_time;
					stats[clients[num]].joined_time = 0;
				}
			}
			else if(cmd == "Kill:")
			{
				trace(cmd << "(" << (in_game?"playing":"waiting") << ")");
				//bug("Kill:");
				// 3:20 Kill: 2 1 10: ^1S^2oo^3K^5ee killed Neko by MOD_RAILGUN
				siz num1, num2, weap;
				if(!(iss >> num1 >> num2 >> weap))
				{
					log("Error parsing Kill:" << line);
					continue;
				}

				if(weap == MOD_SUICIDE)
				{
					// 14:22 Kill: 2 2 20: ^1S^2oo^3K^5ee killed ^1S^2oo^3K^5ee by MOD_SUICIDE
					for(siz i = 0; i < 2; ++i)
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
					if(num1 == 1022 && !clients[num2].is_bot()) // no killer
						++stats[clients[num2]].deaths[weap];
					else if(!clients[num1].is_bot() && !clients[num2].is_bot())
					{
						if(num1 != num2)
						{
							++stats[clients[num1]].kills[weap];
							++onevone[clients[num1]][clients[num2]];
						}
						++stats[clients[num2]].deaths[weap];

						if(sk_cfg.do_kills)
							skivvy.chat('k', "^7" + players[clients[num1]] + " ^4killed ^7" + players[clients[num2]]
								+ " ^4with a ^7" + weapons[weap]);
					}
				}
			}
			else if(cmd == "CTF:")
			{
				trace(cmd << "(" << (in_game?"playing":"waiting") << ")");
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

				str nums_team = "^7[^2U^7]"; // unknown team
				str nums_nteam = "^7[^2U^7]"; // unknown team

				//pthread_mutex_lock(&mtx);
				if(teams[clients[num]] == TEAM_R)
					nums_team = "^7[^1R^7]";
				else if(teams[clients[num]] == TEAM_B)
					nums_team = "^7[^4B^7]";
				if(teams[clients[num]] == TEAM_B)
					nums_nteam = "^7[^1R^7]";
				else if(teams[clients[num]] == TEAM_R)
					nums_nteam = "^7[^4B^7]";
				//pthread_mutex_unlock(&mtx);

				//bug("inc stats");
				if(!clients[num].is_bot())
					++stats[clients[num]].flags[act];

				str hud;

				if(act == FL_CAPTURED) // In Game Announcer
				{
					bug("FL_CAPTURED");
					if(ka_cfg.do_dashes && dashing[col] && dasher[col] != null_guid)
					{
						double sec = (get_millitime() - dash[col]) / 1000.0;

						std::ostringstream oss;
						oss.precision(2);
						oss << std::fixed << sec;
						server.chat(players[clients[num]] + "^3 took ^7" + oss.str()
							+ "^3 seconds to capture the " + flag[col] + "^3 flag.");
						if(sk_cfg.do_flags)
							skivvy.chat('f', players[clients[num]] + "^3 took ^7" + oss.str()
								+ "^3 seconds to capture the " + flag[col] + "^3 flag.");

						double rec = to<double>(recs["dash." + mapname + ".secs"]);

						bug("rec: " << rec);

						if(rec < 0.5)
						{
							server.chat(players[clients[num]] + "^3 has set the record for this map.");
							skivvy.chat('f', players[clients[num]] + "^3 has set the record for this map.");
							recs["dash." + mapname + ".guid"] = to_string(clients[num]);
							recs["dash." + mapname + ".name"] = players[clients[num]];
							recs["dash." + mapname + ".secs"] = oss.str();
							save_records(recs);
						}
						else if(sec < rec)
						{
							server.chat(players[clients[num]] + "^3 beat ^7"
								+ recs["dash." + mapname + ".name"] + "'^3s ^7"
								+ recs["dash." + mapname + ".secs"] + " ^3seconds.");
							skivvy.chat('f', players[clients[num]] + "^3 beat ^7"
								+ recs["dash." + mapname + ".name"] + "'^3s ^7"
								+ recs["dash." + mapname + ".secs"] + " ^3seconds.");
							recs["dash." + mapname + ".guid"] = to_string(clients[num]);
							recs["dash." + mapname + ".name"] = players[clients[num]];
							recs["dash." + mapname + ".secs"] = oss.str();
							save_records(recs);
						}
					}

					++flags[col];
					++caps[clients[num]];
					dasher[col] = null_guid;
					dashing[col] = true; // new dash now possible

					if(ka_cfg.do_flags)
					{
						str msg = players[clients[num]] + "^3 has ^7" + to_string(caps[clients[num]]) + "^3 flag" + (caps[clients[num]]==1?"":"s") + "!";
						server.cp(msg);
						if(sk_cfg.do_flags)
						{
							if(sk_cfg.do_flags_hud)
								hud = get_hud(m, s, dasher);
							skivvy.raw_chat('f', hud + oa_to_IRC(nums_team + " " + msg));
							skivvy.raw_chat('f', hud + oa_to_IRC("^7[ ] ^1RED^3: ^7" + to_string(flags[FL_BLUE]) + " ^3v ^4BLUE^3: ^7" + to_string(flags[FL_RED])));
						}
					}
				}
				else if(act == FL_TAKEN)
				{
					if(dashing[col])
						dash[col] = get_millitime();

					dasher[col] = clients[num];

					if(sk_cfg.do_flags)
					{
						if(sk_cfg.do_flags_hud)
							hud = get_hud(m, s, dasher);
						skivvy.raw_chat('f', hud + oa_to_IRC(nums_team + " ^7" + players[clients[num]] + "^3 has taken the " + flag[col] + " ^3flag!"));
					}
				}
				else if(act == FL_DROPPED)
				{
					if(sk_cfg.do_flags)
					{
						if(sk_cfg.do_flags_hud)
							hud = get_hud(m, s, dasher);
						skivvy.raw_chat('f', hud + oa_to_IRC(nums_team + " ^7" + players[clients[num]] + "^3 has killed the " + flag[ncol] + " ^3flag carrier!"));
					}
					GUID dasher_guid = dasher[ncol];
					dasher[ncol] = null_guid;; // end a dash
					dashing[ncol] = false; // no more dashes until return, capture or suicide
					if(sk_cfg.do_flags)
					{
						if(sk_cfg.do_flags_hud)
							hud = get_hud(m, s, dasher);
						skivvy.raw_chat('f', hud + oa_to_IRC(nums_nteam + " ^7" + players[dasher_guid] + "^3 has dropped the " + flag[ncol] + " ^3flag!"));
					}
				}
				else if(act == FL_RETURNED)
				{
					dasher[col] = null_guid;; // end a dash
					dashing[col] = true; // new dash now possible
					if(sk_cfg.do_flags)
					{
						if(sk_cfg.do_flags_hud)
							hud = get_hud(m, s, dasher);
						skivvy.raw_chat('f', hud + oa_to_IRC(nums_team + " ^7" + players[clients[num]] + "^3 has returned the " + flag[col] + " ^3flag!"));
					}
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
				trace(cmd << "(" << (in_game?"playing":"waiting") << ")");

				// SAVE mapvotes from the previous game (if any)
				// We do this here because if the previous map was voted off
				// end of map processing will have been avoided.

				// NB. This MUST be done before mapname changes
				for(guid_int_map_iter i = map_votes.begin(); i != map_votes.end(); ++i)
					db.add_vote("map", mapname, i->first, i->second);
				map_votes.clear();
				// -----------------

				// in game timing
				for(guid_stat_iter i = stats.begin(); i != stats.end(); ++i)
				{
					i->second.joined_time = 0;
					i->second.logged_time = 0;
				}
				for(guid_siz_map::iterator i = teams.begin(); i != teams.end(); ++i)
				{
					i->second = 0;
				}
				// -----------------

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

				str msg = "^1K^7at^3i^7na ^3Stats System v^7" + version + "^3-" + tag + ".";
				server.cp(msg);

				// startup voting
//				str reply;
//				if(!server.command("set g_allowVote 1", reply))
//					server.command("set g_allowVote 1", reply); // do 1 retry

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
					lower(mapname);
					bug("mapname: " << mapname);

					// load map votes for new map
					db.read_map_votes(mapname, map_votes);
				}
				if(sk_cfg.do_infos && mapname != old_mapname)
				{
					siz love = 0;
					siz hate = 0;
					for(guid_int_map_iter i = map_votes.begin(); i != map_votes.end(); ++i)
					{
						if(i->second > 0)
							++love;
						else
							++hate;
					}
					skivvy.chat('i', ".");
					skivvy.chat('i', "^3== Playing Map: ^7" + mapname + "^3 == ^7" + to_string(love)
						+ " ^1LOVE ^7" + to_string(hate) + " ^2HATE ^3==");
					old_mapname = mapname;
				}
			}
		}
		if(cmd == "say:")
		{
			// 0:23 say: ^5A^6lien ^5S^6urf ^5G^6irl: yes, 3-4 players max
			bug("line: " << line);

			if(sk_cfg.do_chats)
			{
				str text;
				GUID guid;

				if(extract_name_from_text(line, guid, text))
					if(!sk_cfg.spamkill || ++spam[text] < spam_limit)
						skivvy.chat('c', "^7say: " + players[guid] + " ^2" + text);

//				if(std::getline(iss >> std::ws, text))
//					if(!sk_cfg.spamkill || ++spam[text] < spam_limit)
//						skivvy.chat('c', "^7say: " + text);
			}

			siz pos;
			if((pos = line.find_last_of(':')) == str::npos)
				continue;
			if((pos = line.find('!', pos)) == str::npos)
				continue;

			siss iss(line.substr(pos));
			str cmd;
			iss >> cmd;
			bug("cmd: " << cmd);
			lower(cmd);

			if(cmd == "!record")
			{
				con("!record");
				server.chat("^3MAP RECORD: ^7"
					+ recs["dash." + mapname + ".secs"]
					+ "^3 set by ^7" + recs["dash." + mapname + ".name"]);
			}
			else if(cmd == "!love") // TODO:
			{
				str text;
				GUID guid;

				if(!extract_name_from_text(line, guid, text))
					continue;

				iss >> text;

				if(lower(trim(text)) == "map")
				{
					if(map_votes.count(guid) && map_votes[guid] == 1)
						server.chat("^3You can only vote once per week.");
					else if(map_votes.count(guid))
						server.chat("^3Your vote has changed.");
					else
						server.chat("^7" + players[guid] + "^7: ^3Your vote is counted.");
					map_votes[guid] = 1;
				}
			}
			else if(cmd == "!hate") // TODO:
			{
				str text;
				GUID guid;

				if(!extract_name_from_text(line, guid, text))
					continue;

				iss >> text;

				if(lower(trim(text)) == "map")
				{
					if(map_votes.count(guid) && map_votes[guid] == -1)
						server.chat("^3You can only vote once per week.");
					else if(map_votes.count(guid))
						server.chat("^3Your vote has changed.");
					else
						server.chat("^7" + players[guid] + "^7: ^3Your vote is counted.");
					map_votes[guid] = -1;
				}
			}
			else if(cmd == "!register")
			{
				str text;
				GUID guid;

				if(!extract_name_from_text(line, guid, text))
					continue;

				if(db.set_preferred_name(guid, players[guid]))
					server.chat("^7" + players[guid] + "^7: ^3Your preferred name has been registered.");
			}
		}
	}
	done = true;
	pthread_join(teams_thread, 0);
}
