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

#include "Katina.h"

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
#include "RemoteIRCClient.h"
#include "Database.h"
#include "GUID.h"
#include "rconthread.h"

using namespace oastats;
using namespace oastats::data;
using namespace oastats::log;
using namespace oastats::types;
using namespace oastats::string;
using namespace oastats::net;
using namespace oastats::time;

const std::string version = "0.6";
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

const str weapons[] =
{
	"unknown weapon"
	, "shotgun"
	, "gauntlet"
	, "machinegun"
	, "grenade"
	, "grenade schrapnel"
	, "rocket"
	, "rocket blast"
	, "plasma"
	, "plasma splash"
	, "railgun"
	, "lightening"
	, "BFG"
	, "BFG fallout"
	, "dround"
	, "slimed"
	, "burnt up in lava"
	, "crushed"
	, "telefraged"
	, "falling to far"
	, "suicide"
	, "target lazer"
	, "inflicted pain"
	, "nailgun"
	, "chaingun"
	, "proximity mine"
	, "kamikazi"
	, "juiced"
	, "grappled"
};

const str Katina::flag[2] = {"^1RED", "^4BLUE"};

void Katina::report_clients(const siz_guid_map& clients)
{
	for(siz_guid_citer i = clients.begin(); i != clients.end(); ++i)
		con("slot: " << i->first << ", " << i->second);
}

void Katina::report_players(const guid_str_map& players)
{
	for(guid_str_citer i = players.begin(); i != players.end(); ++i)
		con("player: " << i->first << ", " << i->second);
}

void Katina::report_onevone(const onevone_map& onevone, const guid_str_map& players)
{
	for(onevone_citer o = onevone.begin(); o != onevone.end(); ++o)
		for(guid_siz_citer p = o->second.begin(); p != o->second.end(); ++p)
		{
			str p1 = players.at(o->first);
			str p2 = players.at(p->first);
			con("player: " << p1 << " killed " << p2 << " " << p->second << " times.");
		}
}

void Katina::report_caps(const guid_siz_map& caps, const guid_str_map& players, siz flags[2])
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

	if(rep_cfg.do_infos)
	{
//		skivvy->.chat('f', "^1RED^3: ^7" + to_string(flags[FL_BLUE]) + " ^3v ^4BLUE^3: ^7" + to_string(flags[FL_RED]));
		remote->chat('i', "^5== ^6RESULTS ^5== ^7"
			+ to_string(flags[FL_BLUE]) + " ^1RED ^7"
			+ to_string(flags[FL_RED]) + " ^4BLUE ^3 ==");
//		skivvy->chat('i', "^5== ^6RESULTS ^5" + str(max - 23, '='));
		for(siz i = 0; i < results.size(); ++i)
			remote->chat('f', results[i]);
		remote->chat('i', "^5" + str(max - 12, '-'));
	}
}

siz Katina::map_get(const siz_map& m, siz key)
{
	return m.find(key) == m.end() ? 0 : m.at(key);
}

void Katina::report_stats(const guid_stat_map& stats, const guid_str_map& players)
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
		// TODO: modify this to add AW options as well as insta
		if(rep_cfg.do_stats)
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
				skivvy_scores.insert(std::make_pair(rkd, "^3kills^7/^3d ^5(^7" + kd + "^5) ^3caps^7/^3d ^5(^7" + cd + "^5)^7: " + player));
		}
	}
	if(rep_cfg.do_stats)
		for(std::multimap<double, str>::reverse_iterator r = skivvy_scores.rbegin(); r != skivvy_scores.rend(); ++r)
			remote->chat('s', r->second);
}

void Katina::save_records(const str_map& recs)
{
	log("save_records:");
	std::ofstream ofs((str(getenv("HOME")) + "/.katina/records.txt").c_str());

	str sep;
	for(str_map_citer r = recs.begin(); r != recs.end(); ++r)
		{ ofs << sep << r->first << ": " << r->second; sep = "\n"; }
}

void Katina::load_records(str_map& recs)
{
	log("load_records:");
	std::ifstream ifs((str(getenv("HOME")) + "/.katina/records.txt").c_str());

	recs.clear();
	str key;
	str val;
	while(std::getline(std::getline(ifs, key, ':') >> std::ws, val))
		recs[key] = val;
}

GUID Katina::guid_from_name(const str& name)
{
	for(guid_str_iter i = players.begin(); i != players.end(); ++i)
		if(i->second == name)
			return i->first;
	return null_guid;
}

bool Katina::extract_name_from_text(const str& line, GUID& guid, str& text)
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

str Katina::expand_env(const str& var)
{
	str exp;
	wordexp_t p;
	wordexp(var.c_str(), &p, 0);
	if(p.we_wordc)
		exp = p.we_wordv[0];
	wordfree(&p);
	return exp;
}

/**
 *
 * @param m
 * @param s
 * @param dasher
 * @param killtype 0 = none, 1 = red killed, 2 = blue killed
 * @return
 */
str Katina::get_hud(siz m, siz s, GUID dasher[2], siz killtype)
{
	con("dasher[0]: " << dasher[0]);
	con("dasher[1]: " << dasher[1]);
	con("killtype: " << killtype);
	str redflag = "⚑";
	str bluflag = "⚑";

	redflag = dasher[FL_RED] != null_guid ? "⚑" : ".";
	bluflag = dasher[FL_BLUE] != null_guid ? "⚑" : ".";

	redflag = killtype == 1 ? "⚔" : redflag;
	bluflag = killtype == 2 ? "⚔" : bluflag;

	soss oss;
	oss << "00[15" << (m < 10?"0":"") << m << "00:15" << (s < 10?"0":"") << s << " ";
	oss << "04" << redflag;
	oss << "02" << bluflag;
	oss << "00]";
	return oss.str();
}

bool Katina::is_guid(const str& s)
{
	return s.size() == 8 & std::count_if(s.begin(), s.end(), std::ptr_fun<int, int>(isxdigit)) == s.size();
}

int Katina::run(const int argc, const char* argv[])
{
	load_records(recs);

	log("Records loaded: " << recs.size());
	bool testing = false;

	sifs ifs;
	if(argc > 1 && (testing = (str(argv[1]) == "TEST")))
		ifs.open(argv[1]); // read whole log
	else if(argc > 1)
		ifs.open(argv[1], std::ios::ate);
	else if(!recs["logfile"].empty())
		ifs.open(expand_env(recs["logfile"]).c_str(), std::ios::ate);

	sis& is = ifs.is_open() ? ifs : std::cin;

	if(!is)
	{
		log("Input error:");
		return -2;
	}

	if(!(remote = RemoteIRCClient::create(recs["remote.irc.client"])).get())
	{
		log("FATAL ERROR: failed to allocate object at: " << __LINE__);
		return 1;
	}

	remote->config(recs);
	remote->set_testing(testing);

	server.config(recs["rcon.host"], to<siz>(recs["rcon.port"]), recs["rcon.pass"]);
	db.config(recs["db.host"], to<siz>(recs["db.port"]), recs["db.user"], recs["db.pass"], recs["db.base"]);

	server.chat("^3Stats System v^7" + version + "^3-" + tag + " - ^1ONLINE");
	remote->chat('*', "^3Stats System v^7" + version + "^3-" + tag + " - ^1ONLINE");

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

	if(recs.count("rcon.delay"))
		thread_delay = to<milliseconds>(recs["rcon.delay"]);

//	thread_data td =
//	{
//		&mtx
//		, thread_delay
//		, &clients
//		, &teams
//		, &done
//		, &svr_cfg
//		, &rep_cfg
//		, &server
//		, remote.get()
//		, &db
//		, &mapname
//		, &map_votes
//	};
	pthread_create(&teams_thread, NULL, &rconthread, (void*) this);

	milliseconds sleep_time = 100; // milliseconds
	bool done = false;
	std::ios::streampos pos = is.tellg();
	str line;
	while(!done)
	{
		if(svr_cfg.do_dashes)
			sleep_time = 10;
		else
			sleep_time = 100;

		if(!std::getline(is, line) || is.eof())
			{ thread_sleep_millis(sleep_time); is.clear(); is.seekg(pos); continue; }

		pos = is.tellg();

		if(!rep_cfg.active)
			continue;

//		bug("line: " << line);
		bug_var(line);

		iss.clear();
		iss.str(line);
		iss >> m >> c >> s >> cmd;
		secs = (m * 60) + s;
//		bug("cmd: " << cmd);
		if(in_game)
		{
			if(cmd == "Exit:")
			{
				trace(cmd << "(" << (in_game?"playing":"waiting") << ")");
				// shutdown voting until next map
				log("exit: writing stats to database and collecting votes");
				str reply;
				if(!server.command("set g_allowVote 0", reply))
					server.command("set g_allowVote 0", reply); // one retry

				remote->chat('*', "^3Game Over");
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
					if(svr_cfg.do_flags && !caps.empty())
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
							for(std::set<siz>::iterator weap = svr_cfg.db_weaps.begin(); weap != svr_cfg.db_weaps.end(); ++weap)
							{
								if((count = map_get(p->second.kills, *weap)))
									db.add_weaps(id, "kills", p->first, *weap, count);
								if((count = map_get(p->second.deaths, *weap)))
									db.add_weaps(id, "deaths", p->first, *weap, count);
							}

							if((count = map_get(p->second.flags, FL_CAPTURED)))
								db.add_caps(id, p->first, count);
						}
					}

					// report
					con("-- Report: -------------------------------");
					report_clients(clients);
					con("");
					report_players(players);
					con("");
					report_onevone(onevone, players);
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
				//bug("ClientUserinfoChanged:");
				//do_rcon("^3ClientUserinfoChanged:");
				// 0:23 ClientUserinfoChanged: 2 n\^1S^2oo^3K^5ee\t\2\model\ayumi/red\hmodel\ayumi/red\g_redteam\\g_blueteam\\c1\1\c2\1\hc\100\w\0\l\0\tt\0\tl\1\id\1A7C66BACBA16F0C9068D8B82C1D55DE
				siz num;
				if(!std::getline(std::getline(iss >> num, skip, '\\'), name, '\\'))
				{
					std::cout << "Error parsing ClientUserinfoChanged: "  << line << '\n';
					continue;
				}

				//bug("num: " << num);
				//bug("skip: " << skip);
				//bug("name: " << name);

				siz pos = line.find("id\\");
				if(pos != str::npos)
				{
					str guid = line.substr(pos + 3, 32);
					bug_var(guid);

					trim(guid);

					if(guid.empty())
						clients[num] = bot_guid(num);//null_guid;
					else
					{
						clients[num] = to<GUID>(guid.substr(24));
						if(guid.size() != 32 || !is_guid(guid.substr(24)))
						{
							log("INVALID GUID: " << guid);
							log("        line: " << line);
							continue;
						}
					}

					players[clients[num]] = name;

					if(stats[clients[num]].first_seen)
						stats[clients[num]].logged_time += std::time(0) - stats[clients[num]].first_seen;
					stats[clients[num]].first_seen = time + secs;
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

						if(rep_cfg.do_kills)
							remote->chat('k', "^7" + players[clients[num1]] + " ^4killed ^7" + players[clients[num2]]
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

				pthread_mutex_lock(&mtx);
				if(teams[clients[num]] == 'R')
					nums_team = "^7[^1R^7]";
				else if(teams[clients[num]] == 'B')
					nums_team = "^7[^4B^7]";
				if(teams[clients[num]] == 'B')
					nums_nteam = "^7[^1R^7]";
				else if(teams[clients[num]] == 'R')
					nums_nteam = "^7[^4B^7]";
				pthread_mutex_unlock(&mtx);

				//bug("inc stats");
				if(!clients[num].is_bot())
					++stats[clients[num]].flags[act];

				str hud;
				if(act == FL_CAPTURED) // In Game Announcer
				{
					bug("FL_CAPTURED");
					if(svr_cfg.do_dashes && dashing[col] && dasher[col] != null_guid)
					{
						double sec = (get_millitime() - dash[col]) / 1000.0;

						std::ostringstream oss;
						oss.precision(2);
						oss << std::fixed << sec;
						server.chat(players[clients[num]] + "^3 took ^7" + oss.str()
							+ "^3 seconds to capture the " + flag[col] + "^3 flag.");
						if(rep_cfg.do_flags)
							remote->chat('f', players[clients[num]] + "^3 took ^7" + oss.str()
								+ "^3 seconds to capture the " + flag[col] + "^3 flag.");

						double rec = to<double>(recs["dash." + mapname + ".secs"]);

						bug("rec: " << rec);

						if(rec < 0.5)
						{
							server.chat(players[clients[num]] + "^3 has set the record for this map.");
							remote->chat('f', players[clients[num]] + "^3 has set the record for this map.");
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
							remote->chat('f', players[clients[num]] + "^3 beat ^7"
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

					if(svr_cfg.do_flags)
					{
						str msg = players[clients[num]] + "^3 has ^7" + to_string(caps[clients[num]]) + "^3 flag" + (caps[clients[num]]==1?"":"s") + "!";
						server.cp(msg);
						if(rep_cfg.do_flags)
						{
							if(rep_cfg.do_flags_hud)
								hud = get_hud(m, s, dasher);
							remote->raw_chat('f', hud + oa_to_IRC(nums_team + " " + msg));
							remote->raw_chat('f', hud + oa_to_IRC("^7[ ] ^1RED^3: ^7" + to_string(flags[FL_BLUE]) + " ^3v ^4BLUE^3: ^7" + to_string(flags[FL_RED])));
						}
					}
				}
				else if(act == FL_TAKEN)
				{
					if(dashing[col])
						dash[col] = get_millitime();

					dasher[col] = clients[num];

					if(rep_cfg.do_flags)
					{
						if(rep_cfg.do_flags_hud)
							hud = get_hud(m, s, dasher);
						remote->raw_chat('f', hud + oa_to_IRC(nums_team + " ^7" + players[clients[num]] + "^3 has taken the " + flag[col] + " ^3flag!"));
					}
				}
				else if(act == FL_DROPPED)
				{
					if(rep_cfg.do_flags)
					{
						if(rep_cfg.do_flags_hud)
							hud = get_hud(m, s, dasher);//, col ? 1 : 2);
						remote->raw_chat('f', hud + oa_to_IRC(nums_team + " ^7" + players[clients[num]] + "^3 has killed the " + flag[ncol] + " ^3flag carrier!"));
					}
					GUID dasher_guid = dasher[ncol];
					dasher[ncol] = null_guid;; // end a dash
					dashing[ncol] = false; // no more dashes until return, capture or suicide
					if(rep_cfg.do_flags)
					{
						if(rep_cfg.do_flags_hud)
							hud = get_hud(m, s, dasher);
						remote->raw_chat('f', hud + oa_to_IRC(nums_nteam + " ^7" + players[dasher_guid] + "^3 has dropped the " + flag[ncol] + " ^3flag!"));
					}
				}
				else if(act == FL_RETURNED)
				{
					dasher[col] = null_guid;; // end a dash
					dashing[col] = true; // new dash now possible
					if(rep_cfg.do_flags)
					{
						if(rep_cfg.do_flags_hud)
							hud = get_hud(m, s, dasher);
						remote->raw_chat('f', hud + oa_to_IRC(nums_team + " ^7" + players[clients[num]] + "^3 has returned the " + flag[col] + " ^3flag!"));
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
				str reply;
				if(!server.command("set g_allowVote 1", reply))
					server.command("set g_allowVote 1", reply); // do 1 retry

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
				if(rep_cfg.do_infos && mapname != old_mapname)
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
					remote->chat('i', ".");
					remote->chat('i', "^3== Playing Map: ^7" + mapname + "^3 == ^7" + to_string(love)
						+ " ^1LOVE ^7" + to_string(hate) + " ^2HATE ^3==");
					old_mapname = mapname;
				}
			}
		}
		if(cmd == "say:")
		{
			// 0:23 say: ^5A^6lien ^5S^6urf ^5G^6irl: yes, 3-4 players max
			bug("line: " << line);

			if(rep_cfg.do_chats)
			{
				str text;
				GUID guid;

				if(extract_name_from_text(line, guid, text))
					if(!rep_cfg.spamkill || ++spam[text] < spam_limit)
						remote->chat('c', "^7say: " + players[guid] + " ^2" + text);

//				if(std::getline(iss >> std::ws, text))
//					if(!sk_cfg.spamkill || ++spam[text] < spam_limit)
//						skivvy->chat('c', "^7say: " + text);
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
				str love;
				GUID guid;

				if(!extract_name_from_text(line, guid, love))
					continue;

				iss >> love;

				if(lower(trim(love)) == "map")
				{
					if(map_votes.count(guid))
						server.chat("^3You can only vote once per week.");
					else
					{
						map_votes[guid] = 1;
						server.chat("^7" + players[guid] + "^7: ^3Your vote has been counted.");
					}
				}
			}
			else if(cmd == "!hate") // TODO:
			{
				str hate;
				GUID guid;

				if(!extract_name_from_text(line, guid, hate))
					continue;

				iss >> hate;

				if(lower(trim(hate)) == "map")
				{
					if(map_votes.count(guid))
						server.chat("^3You can only vote once per week.");
					else
					{
						map_votes[guid] = -1;
						server.chat("^7" + players[guid] + "^7: ^3Your vote has been counted.");
					}
				}
			}
		}
	}
	done = true;
	pthread_join(teams_thread, 0);
}
