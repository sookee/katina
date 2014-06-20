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
//, human_players_r(0)
//, human_players_b(0)
, carrierBlue(-1)
, carrierRed(-1)
{
}


bool KatinaPluginStats::open()
{
	bug_func();
	host = katina.get("rcon.host", "127.0.0.1");
	port = katina.get("rcon.port", "27960");

	str host = katina.get("stats.db.host", katina.get("db.host", "localhost"));
	siz port = katina.get("stats.db.port", katina.get("db.port", 3306));
	str user = katina.get("stats.db.user", katina.get("db.user", ""));
	str pass = katina.get("stats.db.pass", katina.get("db.pass", ""));
	str base = katina.get("stats.db.base", katina.get("db.base"));

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

}} // Namespace katina::plugin
