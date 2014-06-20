/*-----------------------------------------------------------------.
| Copyright (C) 2013 SooKee oasookee@gmail.com			   |
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

#include <katina/KatinaPlugin.h>
#include "KatinaPluginStats.h"

#include <thread>
#include <future>
#include <cstring>

#include <katina/Database.h>
#include <katina/GUID.h>

#include <katina/types.h>
#include <katina/log.h>
#include <katina/codes.h>

namespace katina { namespace plugin {

using namespace katina::log;
using namespace katina::data;
using namespace katina::types;

KATINA_PLUGIN_TYPE(KatinaPluginStats);
KATINA_PLUGIN_INFO("katina::stats", "katina Stats", "0.1-dev");


siz map_get(const siz_map& m, siz key)
{
	return m.find(key) == m.end() ? 0 : m.at(key);
}



KatinaPluginStats::KatinaPluginStats(Katina& katina)
: KatinaPlugin(katina)
, mapname(katina.get_mapname())
, clients(katina.getClients())
, players(katina.getPlayers())
, teams(katina.getTeams())
, server(katina.server)
, db()
, active(true)
, write(true)
, recordBotGames(false)
, do_prev_stats(false)
, in_game(false)
, stop_stats(false)
, carrierBlue(slot::bad)
, carrierRed(slot::bad)
{
}


bool KatinaPluginStats::open()
{
	bug_func();
	host = katina.get("rcon.host", "127.0.0.1");
	port = katina.get("rcon.port", "27960");

	str default_db = katina.get("db");

	if(!default_db.empty())
	{
		plog("DEFAULT DB: " << default_db);
		default_db += ".";
	}

	str host = katina.get("stats.db.host", katina.get(default_db + "db.host", "localhost"));
	siz port = katina.get("stats.db.port", katina.get(default_db + "db.port", 3306));
	str user = katina.get("stats.db.user", katina.get(default_db + "db.user", ""));
	str pass = katina.get("stats.db.pass", katina.get(default_db + "db.pass", ""));
	str base = katina.get("stats.db.base", katina.get(default_db + "db.base"));

	if(base.empty())
	{
		plog("FATAL: Database config not found");
		return false;
	}

	db.config(host, port, user, pass, base);

	if(!db.check())
	{
		plog("FATAL: Database can not connect");
		return false;
	}

	katina.add_var_event(this, "stats.active", active, true);
	katina.add_var_event(this, "stats.allow.bots", allow_bots, false);
	katina.add_var_event(this, "stats.write", write, true);
	katina.add_var_event(this, "stats.weaps", db_weaps);

	for(siz_set_iter i = db_weaps.begin(); i != db_weaps.end(); ++i)
		plog("DB LOG WEAPON: " << *i);


	katina.add_log_event(this, EXIT);
	katina.add_log_event(this, SHUTDOWN_GAME);
	katina.add_log_event(this, CLIENT_USERINFO_CHANGED);
	//katina.add_log_event(this, CLIENT_CONNECT);
	katina.add_log_event(this, CLIENT_DISCONNECT);
	katina.add_log_event(this, KILL);
	katina.add_log_event(this, CTF);
	katina.add_log_event(this, AWARD);
	katina.add_log_event(this, INIT_GAME);
	katina.add_log_event(this, WARMUP);
	katina.add_log_event(this, WEAPON_USAGE);
	katina.add_log_event(this, MOD_DAMAGE);
	katina.add_log_event(this, PLAYER_STATS);
	katina.add_log_event(this, SAY);
	katina.add_log_event(this, SAYTEAM);
	katina.add_log_event(this, SPEED);

	return true;
}


str KatinaPluginStats::get_id() const	   { return ID; }
str KatinaPluginStats::get_name() const	 { return NAME; }
str KatinaPluginStats::get_version() const  { return VERSION; }

std::multimap<siz, str> prev_game_stats;
str prev_mapname;

bool KatinaPluginStats::exit(siz min, siz sec)
{
	bug_func();
	plog("exit: " << min << ", " << sec);

	if(!in_game)
		return true;

	in_game = false;

	if(!active)
		return true;

	// in game timing
	std::time_t logged_time = 0;

	for(guid_stat_map_iter p = stats.begin(); p != stats.end(); ++p)
	{
		if(p->second.joined_time)
		{
			p->second.logged_time += (katina.now - p->second.joined_time);
			p->second.joined_time = 0;
		}

		logged_time += p->second.logged_time;
	}

	pbug_var(logged_time);
	pbug_var(this->onevone.size());
	pbug_var(this->stats.size());

	std::time_t now = katina.now;

	//lock_guard lock(katina.futures_mtx);
	katina.add_future(std::async(std::launch::async, [this,logged_time,now]
	{
		pbug("RUNNING THREAD:");
		// copy these to avoid synchronizing
		std::time_t start = std::time(0);
		onevone_map onevone = this->onevone;
		guid_stat_map stats = this->stats;

		this->onevone.clear();
		this->stats.clear();

		pbug_var(onevone.size());
		pbug_var(stats.size());

		// lock_guard lock(mtx);
		db.set_trace();
		db_scoper on(db);

		if(logged_time && write)
		{
			game_id id = db.add_game(now, host, port, mapname);
			pbug_var(id);

			if(id != null_id && id != bad_id)
			{
				for(guid_stat_map_citer p = stats.begin(); p != stats.end(); ++p)
				{
					if(!allow_bots && p->first.is_bot())
					{
						pbug("IGNORING BOT: " << katina.getPlayerName(p->first));
						continue;
					}

					db.add_player(p->first, p->second.name);

					if(p->second.hc < 100)
					{
						pbug("IGNORING HANDICAP PLAYER: [" << p->second.hc << "] " << katina.getPlayerName(p->first));
						continue;
					}

					siz count;
					for(std::set<siz>::iterator weap = db_weaps.begin(); weap != db_weaps.end(); ++weap)
					{
						if((count = map_get(p->second.kills, *weap)))
							db.add_weaps(id, "kills", p->first, *weap, count);
						if((count = map_get(p->second.deaths, *weap)))
							db.add_weaps(id, "deaths", p->first, *weap, count);
					}

					if((count = map_get(p->second.flags, FL_CAPTURED)))
						db.add_caps(id, p->first, count);

				   if((count = p->second.logged_time))
						db.add_time(id, p->first, count);

					for(siz_map_citer wu = p->second.weapon_usage.begin(); wu != p->second.weapon_usage.end(); ++wu)
						db.add_weapon_usage(id, p->first, wu->first, wu->second);

					for(moddmg_map_citer md = p->second.mod_damage.begin(); md != p->second.mod_damage.end(); ++md)
						db.add_mod_damage(id, p->first, md->first, md->second.hits, md->second.damage, md->second.hitsRecv, md->second.damageRecv, md->second.weightedHits);

					db.add_playerstats_ps(id, p->first,
						p->second.fragsFace, p->second.fragsBack, p->second.fraggedInFace, p->second.fraggedInBack,
						p->second.spawnKills, p->second.spawnKillsRecv, p->second.pushes, p->second.pushesRecv,
						p->second.healthPickedUp, p->second.armorPickedUp, p->second.holyShitFrags, p->second.holyShitFragged,
						p->second.carrierFrags, p->second.carrierFragsRecv);

					if(p->second.time && p->second.dist)
						db.add_speed(id, p->first, p->second.dist, p->second.time, false);
					if(p->second.time_f && p->second.dist_f)
						db.add_speed(id, p->first, p->second.dist_f, p->second.time_f, true);
				}

				for(onevone_citer o = onevone.begin(); o != onevone.end(); ++o)
				{
					if(!allow_bots && o->first.is_bot())
					{
						pbug("IGNORING 1v1 BOT: " << katina.getPlayerName(o->first));
						continue;
					}

					if(stats[o->first].hc < 100)
					{
						pbug("IGNORING 1v1 HANDICAP PLAYER: [" << stats[o->first].hc << "] " << katina.getPlayerName(o->first));
						continue;
					}

					for(guid_siz_map_citer p = o->second.begin(); p != o->second.end(); ++p)
					{
						if(!allow_bots && p->first.is_bot())
						{
							pbug("IGNORING 1v1 BOT: " << katina.getPlayerName(p->first));
							continue;
						}

						if(stats[p->first].hc < 100)
						{
							pbug("IGNORING 1v1 HANDICAP PLAYER: [" << stats[p->first].hc << "] " << katina.getPlayerName(p->first));
							continue;
						}

						db.add_ovo(id, o->first, p->first, p->second);
					}
				}
			}
		}

//		stats.clear();
//		onevone.clear();

//		str boss;
//		GUID guid;
//		if(db.get_ingame_boss(mapname, clients, guid, boss) && guid != null_guid)
//			server.msg_to_all("^7BOSS: " + katina.getPlayerName(guid) + "^7: " + boss, true);
//		else
//			server.msg_to_all("^7BOSS: ^3There is no boss on this map", true);
//
		plog("STATS WRITTEN IN: " << (std::time(0) - start) << " seconds:");
	}));

	return true;
}


bool KatinaPluginStats::shutdown_game(siz min, siz sec)
{
	in_game = false;

	if(!active)
		return true;

	stall_clients();

	return true;
}

void KatinaPluginStats::updatePlayerTime(slot num)
{
    struct stats& s = stats[katina.getClientGuid(num)];
    if(s.joined_time > 0)
    {
        s.logged_time += katina.now - s.joined_time;
        s.joined_time  = katina.now;
    }
}

void KatinaPluginStats::stall_client(const GUID& guid)
{
	if(!stats[guid].joined_time)
		return;

	// lock_guard lock(mtx);
	stats[guid].logged_time += katina.now - stats[guid].joined_time;
	stats[guid].joined_time = 0;
}

void KatinaPluginStats::unstall_client(const GUID& guid)
{
	if(stats[guid].joined_time)
		return;

	if(katina.getTeam(guid) != TEAM_R && katina.getTeam(guid) != TEAM_B)
		return;

	// lock_guard lock(mtx);
	stats[guid].joined_time = katina.now;
}

void KatinaPluginStats::stall_clients()
{
	for(guid_stat_map_citer ci = stats.begin(); ci != stats.end(); ++ci)
		stall_client(ci->first);
}

void KatinaPluginStats::unstall_clients()
{
	for(guid_stat_map_citer ci = stats.begin(); ci != stats.end(); ++ci)
//		if(!katina.is_disconnected(ci->first))
			unstall_client(ci->first);
}

void KatinaPluginStats::check_bots_and_players()
{
	bool stats_stopped = stop_stats;

	stop_stats = false;
	siz human_players_r = 0;
	siz human_players_b = 0;
	siz bot_players_r = 0;
	siz bot_players_b = 0;

	for(guid_siz_map_citer ci = teams.begin(); ci != teams.end(); ++ci)
	{
		if(ci->first.is_bot())
		{
			if(ci->second == TEAM_R)
				++bot_players_r;
			else if(ci->second == TEAM_B)
				++bot_players_b;
			if(!allow_bots)
				stop_stats = true;
		}
		else if(ci->second == TEAM_R)
			++human_players_r;
		else if(ci->second == TEAM_B)
			++human_players_b;
	}

	siz players_r = human_players_r;
	siz players_b = human_players_b;

	if(allow_bots)
	{
		players_r += bot_players_r;
		players_b += bot_players_b;
	}

	if(stop_stats || !players_r || !players_b)
	{
		stop_stats = true;
		stall_clients();

		if(stats_stopped != stop_stats)
			server.chat("^2Stats recording deactivated^7");
	}
	else
	{
		unstall_clients();
		if(stats_stopped != stop_stats)
			server.chat("^2Stats recording activated^7");
	}
}

bool KatinaPluginStats::client_userinfo_changed(siz min, siz sec, slot num, siz team, const GUID& guid, const str& name, siz hc)
{
	if(allow_bots || !guid.is_bot())
	{
		// lock_guard lock(mtx);
		stats[guid].hc = hc;
		stats[guid].name = name;
	}

	if(!in_game)
		return true;

	if(!active)
		return true;

	check_bots_and_players();

	return true;
}

bool KatinaPluginStats::client_disconnect(siz min, siz sec, slot num)
{
	if(!in_game)
		return true;

	if(!active)
		return true;

	check_bots_and_players();

	const GUID& guid = katina.getClientGuid(num);

	if(guid == null_guid)
	{
		plog("ERROR: Unexpected null GUID");
		return true;
	}

	stall_client(guid);

	return true;
}

bool KatinaPluginStats::kill(siz min, siz sec, slot num1, slot num2, siz weap)
{
	if(!in_game)
		return true;

	if(!active)
		return true;

	if(stop_stats)
		return true;

	const GUID& guid1 = katina.getClientGuid(num1);

	if(guid1 == null_guid)
	{
		plog("ERROR: Unexpected null GUID");
		return true;
	}

	const GUID& guid2 = katina.getClientGuid(num2);

	if(guid2 == null_guid)
	{
		plog("ERROR: Unexpected null GUID");
		return true;
	}

	// lock_guard lock(mtx);
	if(num1 == slot::world) // no killer
		++stats[guid2].deaths[weap];

	else if(allow_bots || (!guid1.is_bot() && !guid2.is_bot()))
	{
		if(num1 != num2)
		{
			++stats[guid1].kills[weap];
			++onevone[guid1][guid2];

			// Target was a flag carrier
			if(num2 == carrierRed || num2 == carrierBlue)
			{
				++stats[guid1].carrierFrags;
				++stats[guid2].carrierFragsRecv;
			}
		}

		//if(!katina.getClientGuid(num2).is_bot())
		++stats[guid2].deaths[weap];
	}

	return true;
}

bool KatinaPluginStats::ctf(siz min, siz sec, slot num, siz team, siz act)
{
	if(!in_game)
		return true;

	// Remember who is carrying the flag
	if(team == TEAM_R)
		carrierRed = act == 0 ? num : slot::bad;
	else if(team == TEAM_B)
		carrierBlue = act == 0 ? num : slot::bad;

	if(!active)
		return true;
	if(stop_stats)
		return true;

	const GUID& guid = katina.getClientGuid(num);

	if(guid == null_guid)
	{
		plog("ERROR: Unexpected null GUID");
		return true;
	}

	// lock_guard lock(mtx);
	++stats[guid].flags[act];

	return true;
}

bool KatinaPluginStats::award(siz min, siz sec, slot num, siz awd)
{
	if(!in_game)
		return true;

	if(!active)
		return true;

	if(stop_stats)
		return true;

	const GUID& guid = katina.getClientGuid(num);

	if(guid == null_guid)
	{
		plog("ERROR: Unexpected null GUID");
		return true;
	}

	// lock_guard lock(mtx);
	++stats[guid].awards[awd];

	return true;
}

bool KatinaPluginStats::init_game(siz min, siz sec, const str_map& cvars)
{
	in_game = true;

	if(!active)
		return true;

	pbug("INITGAME");

	if(!announce_time)
		announce_time = sec + katina.get("boss.announce.delay", 10);

	katina.add_log_event(this, HEARTBEAT);

	// lock_guard lock(mtx);
	{
		stats.clear();
		onevone.clear();
	}

	return true;
}

bool KatinaPluginStats::warmup(siz min, siz sec)
{
	pbug("WARMUP");
	in_game = false;
	//stall_clients();

	// kybosch the announcement
	announce_time = 0;
	//katina.del_log_event(this, HEARTBEAT);

	return true;
}

void KatinaPluginStats::heartbeat(siz min, siz sec)
{
	if(!announce_time || min || sec < announce_time)
		return;

	announce_time = 0; // turn off

	pbug("HEARTBEAT");

	pbug_var(clients.size());

	db_scoper on(db);

	str boss;
	GUID guid;
	if(db.get_ingame_boss(mapname, clients, guid, boss) && guid != null_guid)
		server.msg_to_all("^7BOSS: " + katina.getPlayerName(guid) + "^7: " + boss, true);
	else
		server.msg_to_all("^7BOSS: ^3There is no boss on this map", true);
}

// mod_katina >= 0.1-beta
bool KatinaPluginStats::speed(siz min, siz sec, slot num, siz dist, siz time, bool has_flag)
{
	if(!in_game)
		return true;

	if(!active)
		return true;

	if(stop_stats)
		return true;

	// lock_guard lock(mtx);

	const GUID& guid = katina.getClientGuid(num);

	if(guid == null_guid)
	{
		plog("ERROR: Unexpected null GUID");
		return true;
	}

	struct stats& s = stats[guid];

	if(has_flag)
	{
		s.time_f += time;
		s.dist_f += dist;
	}
	else
	{
		s.time += time;
		s.dist += dist;
	}

	return true;
}

bool KatinaPluginStats::weapon_usage(siz min, siz sec, slot num, siz weapon, siz shots)
{
	if(!in_game)
		return true;

	if(!active)
		return true;

	if(stop_stats)
		return true;

	const GUID& guid = katina.getClientGuid(num);

	if(guid == null_guid)
	{
		plog("ERROR: Unexpected null GUID");
		return true;
	}

	// lock_guard lock(mtx);
	stats[guid].weapon_usage[weapon] += shots;

	return true;
}

bool KatinaPluginStats::mod_damage(siz min, siz sec, slot num, siz mod, siz hits, siz damage, siz hitsRecv, siz damageRecv, float weightedHits)
{
	if(!in_game)
		return true;

	if(!active)
		return true;

	if(stop_stats)
		return true;

	const GUID& guid = katina.getClientGuid(num);

	if(guid == null_guid)
	{
		plog("ERROR: Unexpected null GUID");
		return true;
	}

	// lock_guard lock(mtx);
	mod_damage_stats& moddmg = stats[guid].mod_damage[mod];
	moddmg.hits		 += hits;
	moddmg.damage	   += damage;
	moddmg.hitsRecv	 += hitsRecv;
	moddmg.damageRecv   += damageRecv;
	moddmg.weightedHits += weightedHits;

	return true;
}

bool KatinaPluginStats::player_stats(siz min, siz sec, slot num,
	siz fragsFace, siz fragsBack, siz fraggedInFace, siz fraggedInBack,
	siz spawnKills, siz spawnKillsRecv, siz pushes, siz pushesRecv,
	siz healthPickedUp, siz armorPickedUp, siz holyShitFrags, siz holyShitFragged)
{
	if(!in_game)
		return true;

	if(!active)
		return true;

	if(stop_stats)
		return true;

	const GUID& guid = katina.getClientGuid(num);

	if(guid == null_guid)
	{
		plog("ERROR: Unexpected null GUID");
		return true;
	}

	// lock_guard lock(mtx);
	struct stats& s	 = stats[guid];
	s.fragsFace		+= fragsFace;
	s.fragsBack		+= fragsBack;
	s.fraggedInFace	+= fraggedInFace;
	s.fraggedInBack	+= fraggedInBack;
	s.spawnKills	   += spawnKills;
	s.spawnKillsRecv   += spawnKillsRecv;
	s.pushes		   += pushes;
	s.pushesRecv	   += pushesRecv;
	s.healthPickedUp   += healthPickedUp;
	s.armorPickedUp	+= armorPickedUp;
	s.holyShitFrags	+= holyShitFrags;
	s.holyShitFragged  += holyShitFragged;

	return true;
}

bool KatinaPluginStats::sayteam(siz min, siz sec, const GUID& guid, const str& text)
{
	return say(min, sec, guid, text);
}

bool KatinaPluginStats::check_slot(slot num)
{
	if(clients.find(num) == clients.end())
	{
		plog("WARN: Unknown client number: " << num);
		server.chat_nobeep("^7!STATS: ^3Unknown client number: ^7" + to_string(num));
		return false;
	}
	return true;
}

bool KatinaPluginStats::say(siz min, siz sec, const GUID& guid, const str& text)
{
	if(!active)
		return true;

	slot say_num;

	if((say_num = katina.getClientSlot(guid)) == slot::bad)
	{
		plog("ERROR: Unable to get slot number from guid: " << guid);
		return true;
	}

	if(!check_slot(say_num))
		return true;

	str cmd;
	siss iss(text);

	if(!(iss >> cmd))
		return true;

	const str PREFIX = "^7STATS: ";

	if(cmd == "!register" || cmd == "?register")
	{
		if(cmd[0] == '?')
		{
			server.msg_to(say_num, PREFIX + "^3!register = select your current name to dosplay in the web stats", true);
			server.msg_to(say_num, PREFIX + "^3 The web stats can be found at ^7http:^7/^7/^377.237.250.186^7:^381^7/^3webkatti^7/^3oa-ictf");
			server.msg_to(say_num, PREFIX + "^3 ^7(^3we hope to get a better URL soon^7)");
			return true;
		}

		if(write && katina.getPlayerName(guid) != "UnnamedPlayer" && katina.getPlayerName(guid) != "RenamedPlayer")
		{
			db_scoper on(db);
			if(db.set_preferred_name(guid, katina.getPlayerName(guid)))
				server.chat(PREFIX + katina.getPlayerName(guid) + "^7: ^3Your preferred name has been registered.");
		}
	}
	else if(cmd == "!help" || cmd == "?help")
	{
		server.msg_to(say_num, PREFIX + "^2?stats^7, ^2?boss^7, ^2?champ");
	}
	else if(cmd == "!stats" || cmd == "?stats")
	{
		if(cmd[0] == '?')
		{
			server.msg_to(say_num, PREFIX + "^3!stats <1-3>? = give stats for this month or", true);
			server.msg_to(say_num, PREFIX + "^3optionally 1-3 months previously.");
			server.msg_to(say_num, PREFIX + "^3FH ^7(^2frags^7/^2hour^7)");
			server.msg_to(say_num, PREFIX + "^3CH ^7(^2caps^7/^2hour^7)");
			server.msg_to(say_num, PREFIX + "^3SP ^7(^2average speed in u^7/^2second^7)");
			server.msg_to(say_num, PREFIX + "^3SK ^7(^2skill rating^7)");
			return true;
		}

		siz prev = 0; // count $prev month's back
		if(!(iss >> prev))
			prev = 0;

		bug_var(prev);

		bug("getting stats");

		str stats;
		siz idx = 0;
		db_scoper on(db);
		if(db.get_ingame_stats(guid, mapname, prev, stats, idx))
		{
			str skill = to_string(idx);
			for(siz i = 0; i < 3; ++i)
				skill = skill.size() < 4 ? (" " + skill) : skill;
			server.msg_to_all(stats + " ^7" + katina.getPlayerName(guid));
		}
	}
/*	else if(cmd == "!champ") // last month's champion
	{
		bug("champ:");

		if(ka_cfg.do_db)
		{
			bug("getting champ");
			db.on();
			str stats;
			GUID guid;
			//if(db.get_ingame_champ(mapname, guid, stats))
			//	server.chat("^7LAST MONTH'S CHAMPION: " + players[guid] + "^7: " + stats);
			db.off();
		}
	}
*/
	else if(cmd == "!boss" || cmd == "?boss") // best player in this game (from current months stats)
	{
		if(cmd[0] == '?')
		{
			server.msg_to(say_num, PREFIX + "^2!boss^7: ^3display this map's best player and their ^2!stats ^3for this month.", true);
			server.msg_to(say_num, PREFIX + "^2!boss^7: ^3out of all the players currently connected.");
			return true;
		}

		bug("getting boss");

		str stats;
		GUID guid;
		db_scoper on(db);
		if(db.get_ingame_boss(mapname, clients, guid, stats) && guid != null_guid)
			server.msg_to_all("^7BOSS: " + katina.getPlayerName(guid) + "^7: " + stats, true);
		else
			server.msg_to_all("^7BOSS: ^3There is no boss on this map", true);
	}
	else if(cmd == "!crap" || cmd == "?crap") // best player in this game (from current months stats)
	{
		// TODO: add ?crap to help
		if(cmd[0] == '?')
		{
			server.msg_to(say_num, PREFIX + "^2!crap^7: ^3display this map's crappiest player (who cause the most holy-craps).", true);
			server.msg_to(say_num, PREFIX + "^2!crap^7: ^3out of all the players currently connected.");
			return true;
		}

		bug("getting crappiest");

		str stats;
		GUID guid;
		db_scoper on(db);
		if(db.get_ingame_crap(mapname, clients, guid, stats) && guid != null_guid)
			server.msg_to_all("^7CRAPPIEST: " + katina.getPlayerName(guid) + "^7: " + stats, true);
		else
			server.msg_to_all("^7CRAPPIEST: ^3There is no crappiest on this map", true);
	}

	return true;
}

str KatinaPluginStats::api(const str& cmd, void* blob)
{
	bug("API CALL: " << cmd);
	siss iss(cmd);
	str c;
	if(!(iss >> c))
		return "ERROR: bad request";

	if(c == "get_skill")
	{
		str guid, mapname;
		if(!(iss >> guid >> mapname))
			return "ERROR: bad parameters: " + cmd;

		return std::to_string(get_skill(GUID(guid), mapname));
	}
	else if(c == "get_stats") //guid_stat_map stats;
	{
//		*static_cast<guid_stat_map**>(blob) = &stats;
		set_blob(blob, &stats);

		return "OK:";
	}

	return KatinaPlugin::api(cmd);//"ERROR: unknown request";
}

siz KatinaPluginStats::get_skill(const GUID& guid, const str& mapname)
{
	static str stats;
	static siz skill;

	db_scoper on(db);
	if(!db.get_ingame_stats(guid, mapname, 0, stats, skill))
		skill = 0;
	return skill;
}

void KatinaPluginStats::close()
{

}

// StatsDatabase

static const str playerstats_sql = "insert into `playerstats` values (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)";

void StatsDatabase::init()
{
	// EXPERIMENTAL CODE
	if(!stmt_add_playerstats)
	{
		if(init_stmt(stmt_add_playerstats, playerstats_sql))
		{
			try
			{
				for(siz i = 0, j = 0; i < bind_add_playerstats.size(); ++i)
				{
					if(i == 1)
						continue;
					bind_param(bind_add_playerstats.at(i), siz_add_playerstats.at(j++));
				}
				bind_param(bind_add_playerstats.at(1), guid_add_playerstats, guid_length);
			}
			catch(const std::out_of_range& e)
			{
				log("DATABASE ERROR: " << e.what());
				kill_stmt(stmt_add_playerstats);
			}
		}

		bind_stmt(stmt_add_playerstats, bind_add_playerstats);
	}

// WORKING CODE
//	if(!stmt_add_playerstats)
//		stmt_add_playerstats = mysql_stmt_init(&mysql);
//
//	if(stmt_add_playerstats)
//	{
//		if(mysql_stmt_prepare(stmt_add_playerstats, sql.c_str(), sql.size()))
//		{
//			log("DATABASE ERROR: Unable to prepare add_playerstats: " << mysql_stmt_error(stmt_add_playerstats));
//			mysql_stmt_close(stmt_add_playerstats);
//			stmt_add_playerstats = 0;
//		}
//
//		try
//		{
//			memset(bind_add_playerstats.data(), 0, bind_add_playerstats.size() * sizeof(MYSQL_BIND));
//
//			for(siz i = 0, j = 0; i < bind_add_playerstats.size(); ++i)
//			{
//				if(i == 1)
//					continue;
//				bind_add_playerstats.at(i).buffer_type = MYSQL_TYPE_LONGLONG;
//				bind_add_playerstats.at(i).buffer = &(siz_add_playerstats.at(j++));
//				bind_add_playerstats.at(i).is_null = 0;
//				bind_add_playerstats.at(i).length = 0;
//				bind_add_playerstats.at(i).is_unsigned = 1;
//			}
//
//			bind_add_playerstats.at(1).buffer_type = MYSQL_TYPE_VARCHAR;
//			bind_add_playerstats.at(1).buffer = guid_add_playerstats;
//			bind_add_playerstats.at(1).buffer_length = 9;
//			bind_add_playerstats.at(1).is_null = 0;
//			bind_add_playerstats.at(1).length = &guid_length;
//		}
//		catch(const std::out_of_range& e)
//		{
//			log("DATABASE ERROR: " << e.what());
//			mysql_stmt_close(stmt_add_playerstats);
//			stmt_add_playerstats = 0;
//		}
//
//		if(mysql_stmt_bind_param(stmt_add_playerstats, bind_add_playerstats.data()))
//		{
//			log("DATABASE ERROR: Unable to bind add_playerstats: " << mysql_stmt_error(stmt_add_playerstats));
//			mysql_stmt_close(stmt_add_playerstats);
//			stmt_add_playerstats = 0;
//		}
//	}
}

void StatsDatabase::deinit()
{
	if(stmt_add_playerstats)
		mysql_stmt_close(stmt_add_playerstats);

	stmt_add_playerstats = 0;
}
//   game: game_id host port date map

game_id StatsDatabase::add_game(std::time_t timet, const str& host, const str& port, const str& mapname)
{
	if(trace)
		log("DATABASE: add_game(" << timet << ", " << host << ", " << port << ", " << mapname << ")");

	str safe_mapname;
	if(!escape(mapname, safe_mapname))
	{
		log("DATABASE: ERROR: failed to escape: " << mapname);
		return bad_id;
	}

	char timef[32];// = "0000-00-00 00:00:00";

	siz times = 0;
	//time_t timet = std::time(0);
	if(!(times = strftime(timef, sizeof(timef), "%F %T", gmtime(&timet))))
	{
		log("ERROR: converting time: " << timet);
	}

	str sql = "insert into `game`"
		" (`host`, `port`, `date`, `map`) values (INET_ATON('"
		+ host + "'),'" + port + "','" + str(timef, times) + "','" + safe_mapname + "')";

	game_id id;
	if(!insert(sql, id))
		return bad_id;

	return id;
}

/**
 *
 * @param id
 * @param table "kills" | "deaths"
 * @param guid
 * @param weap
 * @param count
 * @return
 */
bool StatsDatabase::add_weaps(game_id id, const str& table, const GUID& guid, siz weap, siz count)
{
	if(trace)
		log("DATABASE: add_weaps(" << id << ", " << table << ", " << guid << ", " << weap << ", " << count << ")");

	soss oss;
	oss << "insert into `" << table << "` (`game_id`, `guid`, `weap`, `count`) values (";
	oss << "'" << id << "','" << guid << "','" << weap << "','" << count << "')";

	str sql = oss.str();

	return insert(sql);
}

bool StatsDatabase::add_caps(game_id id, const GUID& guid, siz count)
{
	if(trace)
		log("DATABASE: add_caps(" << id << ", " << guid << ", " << count << ")");

	soss oss;
	oss << "insert into `caps` (`game_id`, `guid`, `count`) values (";
	oss << "'" << id << "','" << guid << "','" << count << "')";

	str sql = oss.str();

	return insert(sql);
}

bool StatsDatabase::add_time(game_id id, const GUID& guid, siz count)
{
	if(trace)
		log("DATABASE: add_time(" << id << ", " << guid << ", " << count << ")");

	soss oss;
	oss << "insert into `time` (`game_id`, `guid`, `count`) values (";
	oss << "'" << id << "','" << guid << "','" << count << "')";

	str sql = oss.str();

	return insert(sql);
}

bool StatsDatabase::add_player(const GUID& guid, const str& name)
{
	if(trace)
		log("DATABASE: add_player(" << guid << ", " << name << ")");

	str safe_name;
	if(!escape(name, safe_name))
	{
		log("DATABASE: ERROR: failed to escape: " << name);
		return false;
	}

	soss oss;
	oss << "insert into `player` (`guid`,`name`) values ('" << guid << "','" << safe_name
		<< "') ON DUPLICATE KEY UPDATE count = count + 1";

	str sql = oss.str();

	return insert(sql);
}

bool StatsDatabase::add_ovo(game_id id, const GUID& guid1, const GUID& guid2, siz count)
{
	if(trace)
		log("DATABASE: add_ovo(" << id << ", " << guid1 << ", " << guid2 << ", " << count << ")");

	soss oss;
	oss << "insert into `ovo` (`game_id`,`guid1`,`guid2`,`count`) values ('"
		<< id << "','" << guid1 << "','" << guid2 << "','" << count << "')";

	str sql = oss.str();

	return insert(sql);
}


bool StatsDatabase::add_weapon_usage(game_id id, const GUID& guid, siz weap, siz shots)
{
	if(trace)
		log("DATABASE: add_weapon_usage(" << id << ", " << guid << ", " << weap << ", " << shots << ")");

	soss oss;
	oss << "insert into `weapon_usage` (`game_id`,`guid`,`weap`,`shots`) values ('"
		<< id << "','" << guid << "','" << weap << "','" << shots << "')";

	str sql = oss.str();

	return insert(sql);
}

bool StatsDatabase::add_mod_damage(game_id id, const GUID& guid, siz mod, siz hits, siz damage, siz hitsRecv, siz damageRecv, float weightedHits)
{
	if(trace)
		log("DATABASE: add_mod_damage(" << id << ", " << guid << ", " << mod << ", " << hits << ", " << damage << ", " << hitsRecv << ", " << damageRecv << ", " << weightedHits << ")");

	soss oss;
	oss << "insert into `damage` (`game_id`,`guid`,`mod`,`hits`,`dmgDone`,`hitsRecv`,`dmgRecv`,`weightedHits`) values ('"
		<< id << "','" << guid << "','" << mod << "','" << hits << "','" << damage << "','" << hitsRecv << "','" << damageRecv << "','" << weightedHits << "')";

	str sql = oss.str();

	return insert(sql);
}

bool StatsDatabase::add_playerstats(game_id id, const GUID& guid,
	siz fragsFace, siz fragsBack, siz fraggedInFace, siz fraggedInBack,
	siz spawnKills, siz spawnKillsRecv, siz pushes, siz pushesRecv,
	siz healthPickedUp, siz armorPickedUp, siz holyShitFrags, siz holyShitFragged,
	siz carrierFrags, siz carrierFragsRecv)
{
	soss oss;
	oss << "insert into `playerstats` ("
	    << "`game_id`,`guid`,`fragsFace`,`fragsBack`,`fraggedInFace`,`fraggedInBack`,`spawnKillsDone`,`spawnKillsRecv`,"
	    << "`pushesDone`,`pushesRecv`,`healthPickedUp`,`armorPickedUp`,`holyShitFrags`,`holyShitFragged`,`carrierFrags`,`carrierFragsRecv`) "
	    << "values ('" << id << "','" << guid << "','" << fragsFace << "','" << fragsBack << "','" << fraggedInFace << "','" << fraggedInBack
		<< "','" << spawnKills << "','" << spawnKillsRecv << "','" << pushes << "','" << pushesRecv << "','" << healthPickedUp << "','" << armorPickedUp
		<< "','" << holyShitFrags << "','" << holyShitFragged << "','" << carrierFrags << "','" << carrierFragsRecv << "')";

	str sql = oss.str();

	return insert(sql);
}

// TODO: split these up into separate tables
bool StatsDatabase::add_playerstats_ps(game_id id, const GUID& guid,
	siz fragsFace, siz fragsBack, siz fraggedInFace, siz fraggedInBack,
	siz spawnKills, siz spawnKillsRecv, siz pushes, siz pushesRecv,
	siz healthPickedUp, siz armorPickedUp, siz holyShitFrags, siz holyShitFragged,
	siz carrierFrags, siz carrierFragsRecv)
{
	if(!stmt_add_playerstats)
		return add_playerstats(id, guid, fragsFace, fragsBack, fraggedInFace, fraggedInBack,
	spawnKills, spawnKillsRecv, pushes, pushesRecv,
	healthPickedUp, armorPickedUp, holyShitFrags, holyShitFragged,
	carrierFrags, carrierFragsRecv);

	siz j = 0;
	siz_add_playerstats[j++] = id;
	std::strncpy(guid_add_playerstats.data(), str(guid).c_str(), 8);
	guid_length = str(guid).size();
	siz_add_playerstats[j++] = fragsFace;
	siz_add_playerstats[j++] = fragsBack;
	siz_add_playerstats[j++] = fraggedInFace;
	siz_add_playerstats[j++] = fraggedInBack;
	siz_add_playerstats[j++] = spawnKills;
	siz_add_playerstats[j++] = spawnKillsRecv;
	siz_add_playerstats[j++] = pushes;
	siz_add_playerstats[j++] = pushesRecv;
	siz_add_playerstats[j++] = healthPickedUp;
	siz_add_playerstats[j++] = armorPickedUp;
	siz_add_playerstats[j++] = holyShitFrags;
	siz_add_playerstats[j++] = holyShitFragged;
	siz_add_playerstats[j++] = carrierFrags;
	siz_add_playerstats[j++] = carrierFragsRecv;

	if(mysql_stmt_execute(stmt_add_playerstats))
	{
		log("DATABASE: ERROR: " << mysql_stmt_error(stmt_add_playerstats));
		log("DATABASE:      : using fall-back");
		return add_playerstats(id, guid, fragsFace, fragsBack, fraggedInFace, fraggedInBack,
			spawnKills, spawnKillsRecv, pushes, pushesRecv,
			healthPickedUp, armorPickedUp, holyShitFrags, holyShitFragged,
			carrierFrags, carrierFragsRecv);
	}

	if(mysql_stmt_affected_rows(stmt_add_playerstats) != 1)
	{
		log("DATABASE: ERROR: add_playerstats_ps: no rows affected");
		log("DATABASE:      : using fall-back");
		return add_playerstats(id, guid, fragsFace, fragsBack, fraggedInFace, fraggedInBack,
			spawnKills, spawnKillsRecv, pushes, pushesRecv,
			healthPickedUp, armorPickedUp, holyShitFrags, holyShitFragged,
			carrierFrags, carrierFragsRecv);
	}

	return true;
}

bool StatsDatabase::add_speed(game_id id, const GUID& guid,
	siz dist, siz time, bool has_flag)
{
	if(trace)
		log("DATABASE: add_speed(" << id << ", " << guid << ", " << dist << ", " << time << ", " << has_flag << ")");

	soss oss;
	oss << "insert into `speed` ("
	    << "`game_id`,`guid`,`dist`,`time`,`flag`) "
	    << "values ('" << id << "','" << guid << "','" << dist << "','" << time << "','" << has_flag << "')";

	str sql = oss.str();

	return insert(sql);
}

bool StatsDatabase::set_preferred_name(const GUID& guid, const str& name)
{
	if(trace)
		log("DATABASE: set_preferred_name(" << guid << ", " << name << ")");

	str safe_name;
	if(!escape(name, safe_name))
		return bad_id;

	soss oss;
	oss << "insert into `user` (`guid`,`name`) values ('"
		<< guid << "','" << safe_name << "') on duplicate key update `name` = '" << safe_name << "'";

	str sql = oss.str();

	return insert(sql);
}

bool StatsDatabase::get_preferred_name(const GUID& guid, str& name)
{
	if(trace)
		log("DATABASE: get_preferred_name(" << guid << ", " << name << ")");

	soss oss;
	oss << "select name from user where guid = '" << guid << "'";

	str sql = oss.str();

	str_vec_vec rows;
	if(!select(sql, rows, 1))
		return false;

	if(rows.empty())
		return false;

	name = rows[0][0];

	return true;
}

bool calc_period(siz& syear, siz& smonth, siz& eyear, siz& emonth, siz prev = 0)
{
	if(prev > 3)
		return false;

	std::time_t now = std::time(0);
	std::tm t = *gmtime(&now);

	syear = t.tm_year + 1900;
	smonth = t.tm_mon; // 0 - 11
	if(smonth < prev)
	{
		smonth = smonth + 12 - prev + 1; // 1 - 12
		--syear;
	}
	else
		smonth = smonth - prev + 1; // 1 - 12

	eyear = syear;

	emonth = smonth + 1;
	if(emonth > 12)
	{
		emonth = 1;
		++eyear;
	}

//	bug_var(syear);
//	bug_var(smonth);
//	bug_var(eyear);
//	bug_var(emonth);

	return true;
}

bool StatsDatabase::get_ingame_champ(const str& mapname, GUID& guid, str& stats)
{
	return true;
}

struct stat_c
{
	siz kills;
	siz caps;
	siz secs;
	siz fph;
	siz cph;
	siz idx;
	stat_c(): kills(0), caps(0), secs(0), fph(0), cph(0), idx(0) {}
};

typedef std::map<str, stat_c> stat_map; // guid -> stat_c
typedef stat_map::iterator stat_map_iter;
typedef stat_map::const_iterator stat_map_citer;

siz StatsDatabase::get_kills_per_cap(const str& sql_select_games)
{
	// -- get ratio of frags to caps

	soss oss;

	str subsql = sql_select_games;

//	if(subsql.empty())
//	{
//		siz syear = 0;
//		siz smonth = 0;
//		siz eyear = 0;
//		siz emonth = 0;
//
//		if(!calc_period(syear, smonth, eyear, emonth))
//			return false;
//
//		oss.clear();
//		oss.str("");
//		oss << "select `game_id` from `game` where `map` = '" << mapname << "'";
//		oss << " and `date` >= TIMESTAMP('" << syear << '-' << (smonth < 10 ? "0":"") << smonth << '-' << "01" << "')";
//		oss << " and `date` <  TIMESTAMP('" << eyear << '-' << (emonth < 10 ? "0":"") << emonth << '-' << "01" << "')";
//		subsql = oss.str();
//	}

	stat_map stat_cs;
	str_set guids;

	oss.clear();
	oss.str("");
	oss << "select sum(`kills`.`count`) from `kills` where";
	oss << " `kills`.`game_id` in (" << subsql << ")";

	str sql = oss.str();

//	bug_var(sql);

	str_vec_vec rows;

	if(!select(sql, rows, 1))
		return false;

	siz k = 0;

	if(!rows.empty() && !rows[0].empty())
		k = to<siz>(rows[0][0]);

	oss.clear();
	oss.str("");
	oss << "select sum(`caps`.`count`) from `caps` where";
	oss << " `caps`.`game_id` in (" << subsql << ")";

	sql = oss.str();

//	bug_var(sql);

	if(!select(sql, rows, 1))
		return false;

	siz c = 0;

	if(!rows.empty() && !rows[0].empty())
		c = to<siz>(rows[0][0]);

	return c ? (k / c) : 1;
}

bool StatsDatabase::get_ingame_boss(const str& mapname, const slot_guid_map& clients, GUID& guid, str& stats)
{
	if(trace)
		log("DATABASE: get_ingame_boss(" << mapname << ", " << clients.size() << ")");
	siz syear = 0;
	siz smonth = 0;
	siz eyear = 0;
	siz emonth = 0;

	if(!calc_period(syear, smonth, eyear, emonth))
		return false;

	stat_map stat_cs;
	str_set guids;

	soss oss;
	oss << "select `game_id` from `game` where `map` = '" << mapname << "'";
	oss << " and `date` >= TIMESTAMP('" << syear << '-' << (smonth < 10 ? "0":"") << smonth << '-' << "01" << "')";
	oss << " and `date` <  TIMESTAMP('" << eyear << '-' << (emonth < 10 ? "0":"") << emonth << '-' << "01" << "')";
	str sql_select_games = oss.str();

	str sep;
	oss.clear();
	oss.str("");
	for(slot_guid_map_citer i = clients.begin(); i != clients.end(); ++i)
		if(!i->second.is_bot())
			{ oss << sep << "'" << i->second << "'"; sep = ",";}

	str insql = oss.str();

	guid = null_guid;
	stats = "^3FPH^7: ^20 ^3CPH^7: ^20 ^3index^7: ^20.00";

	if(insql.empty())
		return true;

	oss.clear();
	oss.str("");
	oss << "select distinct `guid`,sum(`kills`.`count`) from `kills` where `kills`.`guid` in (" << insql << ")";
	oss << " and `game_id` in (" << sql_select_games << ") group by `guid` order by sum(`kills`.`count`) desc";

	str sql = oss.str();

//	bug_var(sql);

	str_vec_vec rows;

	if(!select(sql, rows, 2))
		return false;

	for(siz i = 0; i < rows.size(); ++i)
	{
		if(rows[i][0].empty() || rows[i][1].empty())
			continue;
		stat_cs[rows[i][0]].kills = to<siz>(rows[i][1]);
		guids.insert(rows[i][0]);
	}

	oss.clear();
	oss.str("");
	oss << "select distinct `guid`,sum(`caps`.`count`) from `caps` where `caps`.`guid` in (" << insql << ")";
	oss << " and `game_id` in (" << sql_select_games << ") group by `guid` order by sum(`caps`.`count`) desc";

	sql = oss.str();

//	bug_var(sql);

	if(!select(sql, rows, 2))
		return false;

	for(siz i = 0; i < rows.size(); ++i)
	{
		if(rows[i][0].empty() || rows[i][1].empty())
			continue;
		stat_cs[rows[i][0]].caps = to<siz>(rows[i][1]);
		guids.insert(rows[i][0]);
	}

	oss.clear();
	oss.str("");
	oss << "select distinct `guid`,sum(`time`.`count`) from `time` where `time`.`guid` in (" << insql << ")";
	oss << " and `game_id` in (" << sql_select_games << ") group by `guid` order by sum(`time`.`count`) desc";

	sql = oss.str();

//	bug_var(sql);

	if(!select(sql, rows, 2))
		return false;

	for(siz i = 0; i < rows.size(); ++i)
	{
		if(rows[i][0].empty() || rows[i][1].empty())
			continue;
		stat_cs[rows[i][0]].secs = to<siz>(rows[i][1]);
		guids.insert(rows[i][0]);
	}

	if(guids.empty())
		return true;

	// -- get ratio of kills to caps


	double kpc = get_kills_per_cap(sql_select_games);

//	bug_var(k);
//	bug_var(c);
//	bug_var(kpc);

	// -- index: sqrt(pow(fph, 2) + pow(cph * kpc, 2)) * acc

	str_set_iter maxi = guids.end();
	double maxv = 0.0;

	for(str_set_iter g = guids.begin(); g != guids.end(); ++g)
	{
		if(stat_cs[*g].secs)
		{
			stat_cs[*g].fph = stat_cs[*g].kills * 60 * 60 / stat_cs[*g].secs;
			stat_cs[*g].cph = stat_cs[*g].caps * 60 * 60 / stat_cs[*g].secs;
			stat_cs[*g].idx = std::sqrt(std::pow(stat_cs[*g].fph, 2)
				+ std::pow(stat_cs[*g].cph * kpc, 2));
			if(stat_cs[*g].idx > maxv)
			{
				maxv = stat_cs[*g].idx;
				maxi = g;
			}
		}
	}

	if(maxi != guids.end())
	{
		guid = GUID(*maxi);
		if(!guid.is_bot())
		{
			str fpad = stat_cs[*maxi].fph < 10 ? "00" : (stat_cs[*maxi].fph < 100 ? "0" : "");
			str cpad = stat_cs[*maxi].cph < 10 ? "00" : (stat_cs[*maxi].cph < 100 ? "0" : "");
			str spad = stat_cs[*maxi].idx < 10 ? "00" : (stat_cs[*maxi].idx < 100 ? "0" : "");
			soss oss;
			oss << "^3FH^7:^2" << fpad << stat_cs[*maxi].fph;
			oss << " ^3CH^7:^2" << cpad << stat_cs[*maxi].cph;
			oss << std::fixed;
			oss.precision(2);
			oss << " ^3skill^7:^2" << spad << stat_cs[*maxi].idx;
			stats = oss.str();
//			bug_var(stats);
		}
	}

	return true;
}

//2014-05-08 11:42:17: DATABASE: on [../../src/Database.cpp] (36)
//2014-05-08 11:42:17: DATABASE: get_ingame_stats(7B5DA741, , 0) [../../src/Database.cpp] (638)
//2014-05-08 11:42:17: DATABASE: off [../../src/Database.cpp] (46)

bool StatsDatabase::get_ingame_stats(const GUID& guid, const str& mapname, siz prev, str& stats, siz& skill)
{
	if(trace)
		log("DATABASE: get_ingame_stats(" << guid << ", " << mapname << ", " << prev << ")");

	if(mapname.empty())
		return false;

	siz syear = 0;
	siz smonth = 0;
	siz eyear = 0;
	siz emonth = 0;

	if(!calc_period(syear, smonth, eyear, emonth, prev))
		return false;

	soss sql;
	sql << "select `game_id` from `game` where `map` = '" << mapname << "'";
	sql << " and `date` >= TIMESTAMP('" << syear << '-' << (smonth < 10 ? "0":"") << smonth << '-' << "01" << "')";
	sql << " and `date` <  TIMESTAMP('" << eyear << '-' << (emonth < 10 ? "0":"") << emonth << '-' << "01" << "')";
	str sql_select_games = sql.str();

	// kills

	sql.clear();
	sql.str("");
	sql << "select sum(`kills`.`count`) from `kills` where `kills`.`guid` = '";
	sql << guid << "'";
	sql << " and `game_id` in (" << sql_select_games << ")";

//	bug_var(sql.str());

	str_vec_vec rows;

	if(!select(sql.str(), rows, 1))
		return false;

	str kills = (rows.empty() || rows[0][0].empty()) ? "0" : rows[0][0];
//	bug_var(kills);

	// shots

	sql.clear();
	sql.str("");
	sql << "select sum(`weapon_usage`.`shots`) from `weapon_usage`";
	sql << " where `weapon_usage`.`guid` = '" << guid << "'";
	sql << " and `weapon_usage`.`weap` = '7'"; // FIXME: railgun only (not good for AW)
	sql << " and `game_id` in (" << sql_select_games << ")";

//	bug_var(sql.str());

	if(!select(sql.str(), rows, 1))
		return false;

	str shots = rows.empty() || rows[0][0].empty() ? "0" : rows[0][0];
//	bug_var(shots);

	// hits

	sql.clear();
	sql.str("");
	sql << "select sum(`damage`.`hits`) from `damage`";
	sql << " where `damage`.`guid` = '" << guid << "'";
	sql << " and `damage`.`mod` = '10'"; // FIXME: railgun only (not good for AW)
	sql << " and `game_id` in (" << sql_select_games << ")";

//	bug_var(sql.str());

	if(!select(sql.str(), rows, 1))
		return false;

	str hits = rows.empty() || rows[0][0].empty() ? "0" : rows[0][0];
	bug_var(hits);

	// caps

	sql.clear();
	sql.str("");
	sql << "select sum(`caps`.`count`) from `caps` where `caps`.`guid` = '";
	sql << guid << "'";
	sql << " and `game_id` in (" << sql_select_games << ")";

//	bug_var(sql.str());

	if(!select(sql.str(), rows, 1))
		return false;

	str caps = (rows.empty() || rows[0][0].empty()) ? "0" : rows[0][0];
//	bug_var(caps);

	// speed

	sql.clear();
	sql.str("");
	sql << "select sum(`speed`.`time`), sum(`speed`.`dist`) from `speed` where `speed`.`guid` = '";
	sql << guid << "'";
	sql << " and `game_id` in (" << sql_select_games << ")";

//	bug_var(sql.str());

	if(!select(sql.str(), rows, 2))
		return false;

	str time = rows.empty() || rows[0][0].empty() ? "0" : rows[0][0];
	str distance = rows.empty() || rows[0][1].empty() ? "0" : rows[0][1];

//	bug_var(time);
//	bug_var(distance);

	siz t = 0;
	siz d = 0;

	siss iss;

	iss.str(time + ' ' + distance);
	iss.clear();

	if(!(iss >> t >> d))
	{
		log("DATABASE ERROR: parsing results: time: " << time << " distance: " << distance);
		return false;
	}

	siz ups = 0; // u/sec

	if(t)
		ups = d / t;

	// secs

	sql.clear();
	sql.str("");
	sql << "select sum(`time`.`count`) from `time` where `time`.`guid` = '";
	sql << guid << "'";
	sql << " and `game_id` in (" << sql_select_games << ")";

//	bug_var(sql.str());

	if(!select(sql.str(), rows, 1))
		return false;

	str secs = rows.empty() || rows[0][0].empty() ? "0" : rows[0][0];
//	bug_var(secs);

	siz sec = 0;
	siz fph = 0;
	siz cph = 0;
	siz hit = 0;
	siz acc = 0;

	iss.str(kills + ' ' + shots + ' ' + hits + ' ' + caps + ' ' + secs);
	iss.clear();

	if(!(iss >> fph >> acc >> hit >> cph >> sec))
	{
		log("DATABASE ERROR: parsing results: " << (kills + ' ' + shots + ' ' + hits + ' ' + caps + ' ' + secs));
		return false;
	}

	stats = "^7<^3not recorded for this map^7>";

	//hours /= (60 * 60);
	if(acc)
		acc = (hit * 100) / acc;
	else
		acc = 0;

	skill = 0;
	if(sec)
	{
		fph = (fph * 60 * 60) / sec;
		cph = (cph * 60 * 60) / sec;
		// Ranking
		siz kpc = get_kills_per_cap(sql_select_games);
		skill = std::sqrt(std::pow(fph, 2) + std::pow(cph * kpc, 2));
		// - Ranking

		str fpad = fph < 10 ? "  " : (fph < 100 ? " " : "");
		str cpad = cph < 10 ? " " : "";
		str apad = acc < 10 ? " " : "";
		str spad = ups < 10 ? "  " : (ups < 100 ? " " : "");
		soss oss;
		oss << std::fixed;
		oss.precision(1);
		oss << "^3FH^7:^2" << fpad << fph << " ^3CH^7:^2" << cpad << cph << " ^3AC^7:^2" << apad << acc;
		oss << " ^3SP^7:^2" << spad << ups << "u/s" << " ^3SK^7:^2" << skill;
		stats = oss.str();
//		bug_var(stats);

	}

	return true;
}

// TODO: Make bood, champ & stats return a proper GUID like this does scanning the clients
bool StatsDatabase::get_ingame_crap(const str& mapname, const slot_guid_map& clients, GUID& guid, str& stats)
{
	log("DATABASE: get_ingame_crap(" << mapname << ", " << clients.size() << ")");
	siz syear = 0;
	siz smonth = 0;
	siz eyear = 0;
	siz emonth = 0;

	if(!calc_period(syear, smonth, eyear, emonth))
		return false;

	stat_map stat_cs;
	//str_set guids;

	soss oss;
	oss << "select `game_id` from `game` where `map` = '" << mapname << "'";
	oss << " and `date` >= TIMESTAMP('" << syear << '-' << (smonth < 10 ? "0":"") << smonth << '-' << "01" << "')";
	oss << " and `date` <  TIMESTAMP('" << eyear << '-' << (emonth < 10 ? "0":"") << emonth << '-' << "01" << "')";
	str sql_select_games = oss.str();

	str sep;
	oss.clear();
	oss.str("");
	for(slot_guid_map_citer i = clients.begin(); i != clients.end(); ++i)
		if(!i->second.is_bot())
			{ oss << sep << "'" << i->second << "'"; sep = ",";}
	str insql = oss.str();

	guid = null_guid;
	stats = "^3Craps/Hour^7: ^20.0";

	if(insql.empty())
		return true;

	oss.clear();
	oss.str("");
	oss << "select distinct `guid`,sum(`playerstats`.`holyShitFrags`) from `playerstats` where `playerstats`.`guid` in (" << insql << ")";
	oss << " and `game_id` in (" << sql_select_games << ") group by `guid` order by sum(`playerstats`.`holyShitFrags`) desc";

	str sql = oss.str();

	bug_var(sql);

	str_vec_vec rows;

	if(!select(sql, rows, 2))
		return false;

	for(siz i = 0; i < rows.size(); ++i)
	{
		if(rows[i][0].empty() || rows[i][1].empty())
			continue;
		stat_cs[rows[i][0]].kills = to<siz>(rows[i][1]);
		//guids.insert(rows[i][0]);
	}

	oss.clear();
	oss.str("");
	oss << "select distinct `guid`,sum(`time`.`count`) from `time` where `time`.`guid` in (" << insql << ")";
	oss << " and `game_id` in (" << sql_select_games << ") group by `guid` order by sum(`time`.`count`) desc";

	sql = oss.str();

//	bug_var(sql);

	if(!select(sql, rows, 2))
		return false;

	for(siz i = 0; i < rows.size(); ++i)
	{
		if(rows[i][0].empty() || rows[i][1].empty())
			continue;
		stat_cs[rows[i][0]].secs = to<siz>(rows[i][1]);
		//guids.insert(rows[i][0]);
	}

//	if(guids.empty())
//		return true;

	slot_guid_map_citer maxi = clients.end();
	double maxv = 0.0;

	for(slot_guid_map_citer g = clients.begin(); g != clients.end(); ++g)
	{
		if(stat_cs[g->second].secs)
		{
			stat_cs[g->second].fph = stat_cs[g->second].kills * 60 * 60 / stat_cs[g->second].secs;
			if(stat_cs[g->second].fph > maxv)
			{
				maxv = stat_cs[g->second].fph;
				maxi = g;
			}
		}
	}

	if(maxi != clients.end())
	{
		if(!maxi->second.is_bot())
		{
			soss oss;
			oss << "^3FCraps/Hour^7:^2" << stat_cs[maxi->second].fph;
			stats = oss.str();
//			bug_var(stats);
			guid = maxi->second;
		}
	}

	return true;
}

}} // Namespace katina::plugin
