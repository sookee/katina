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
KATINA_PLUGIN_INFO("katina::stats", "katina Stats", "0.1");


static siz map_get(const siz_map& m, siz key)
{
	return m.find(key) == m.end() ? 0 : m.at(key);
}



KatinaPluginStats::KatinaPluginStats(Katina& katina)
: KatinaPlugin(katina)
, mapname(katina.get_mapname())
, clients(katina.getClients())
, players(katina.getPlayers())
//, teams(katina.getTeams())
, server(katina.server)
, db()
, active(false)
//, write(false)
, recordBotGames(false)
, in_game(false)
, stop_stats(false)
, carrierBlue(slot::bad)
, carrierRed(slot::bad)
{
	trace();
}


bool KatinaPluginStats::open()
{
	trace();
	katina.add_var_event(this, "stats.active", active, false);
	katina.add_var_event(this, "stats.write", db.get_write_flag(), false);
	katina.add_var_event(this, "stats.allow.bots", allow_bots, false);
	katina.add_var_event(this, "stats.weaps", db_weaps);

	for(siz_set_iter i = db_weaps.begin(); i != db_weaps.end(); ++i)
		plog("DB LOG WEAPON: " << *i);

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

	katina.add_log_event(this, KE_EXIT);
	katina.add_log_event(this, KE_SHUTDOWN_GAME);
	katina.add_log_event(this, KE_CLIENT_USERINFO_CHANGED);
	//katina.add_log_event(this, KE_CLIENT_CONNECT);
	katina.add_log_event(this, KE_CLIENT_DISCONNECT);
	katina.add_log_event(this, KE_KILL);
	katina.add_log_event(this, KE_CTF);
	katina.add_log_event(this, KE_AWARD);
	katina.add_log_event(this, KE_INIT_GAME);
	katina.add_log_event(this, KE_WARMUP);
	katina.add_log_event(this, KE_WEAPON_USAGE);
	katina.add_log_event(this, KE_MOD_DAMAGE);
	katina.add_log_event(this, KE_PLAYER_STATS);
	katina.add_log_event(this, KE_SAY);
	katina.add_log_event(this, KE_SAYTEAM);
	katina.add_log_event(this, KE_SPEED);

	return true;
}


str KatinaPluginStats::get_id() const	   { return ID; }
str KatinaPluginStats::get_name() const	 { return NAME; }
str KatinaPluginStats::get_version() const  { return VERSION; }

std::multimap<siz, str> prev_game_stats;
str prev_mapname;

template<typename Type>
struct clear_scoper
{
	Type& obj;
	clear_scoper(Type& obj): obj(obj) {}
	~clear_scoper() { obj.clear(); }
};

bool KatinaPluginStats::exit(siz min, siz sec)
{
	trace();

	if(!in_game)
		return true;

	in_game = false;

	if(!active)
		return true;

	clear_scoper<slot_stat_map> clear_stats(stats);
	clear_scoper<onevone_map> clear_onevone(onevone);

	// in game timing
	std::time_t logged_time = 0;

//	for(auto p = stats.begin(); p != stats.end(); ++p)
	for(auto&& s: stats)
	{
		if(s.second.joined_time)
		{
			s.second.logged_time += (katina.now - s.second.joined_time);
			s.second.joined_time = 0;
		}

		logged_time += s.second.logged_time;

		if(!archives.emplace(clients[s.first].guid).second)
		{
			plog("ERROR: archive already exists for: " << clients[s.first].guid);
		}
	}

	bug_var(logged_time);

	if(!logged_time)
		return true;

	std::time_t now = katina.now;

//	katina.add_future(std::async(std::launch::async, [this,now,logged_time](str mapname, guid_stat_map stats, onevone_map onevone)
//	{
//		trace();
//
//		static std::mutex mtx;
//		lock_guard lock(mtx);

		pbug("RUNNING THREAD: " << std::this_thread::get_id());
		std::time_t start = std::time(0);

		StatsDatabaseMySql db;
		db.config(this->db);
		if(!db.check())
		{
			ptlog("ERROR: STATS NOT WRITTEN");
			return true;
		}

		db.get_write_flag() = true;
		db.set_trace();
		db_scoper on(db);

		game_id id = db.add_game(now, host, port, mapname);

		ptbug_var(id);
		ptbug_var(stats.size());

		if(id == null_id || id == bad_id)
			return true;

//		for(guid_stat_map_citer p = stats.begin(); p != stats.end(); ++p)
		for(auto&& s: stats)
		{
			const auto& client = clients[s.first];
			const auto& guid = client.guid;

			if(!allow_bots && client.bot)
			{
				ptbug("IGNORING BOT: " << s.second.name);
				continue;
			}

			db.add_player(now, guid, s.second.name);

			if(s.second.hc < 100)
			{
				ptbug("IGNORING HANDICAP PLAYER: [" << s.second.hc << "] " << s.second.name);
				continue;
			}

			siz count;
//			for(std::set<siz>::iterator weap = db_weaps.begin(); weap != db_weaps.end(); ++weap)
			for(auto&& weap: db_weaps)
			{
				if((count = map_get(s.second.kills, weap)))
					db.add_weaps(id, "kills", guid, weap, count);
				if((count = map_get(s.second.deaths, weap)))
					db.add_weaps(id, "deaths", guid, weap, count);
			}

			pbug_var(katina.get_line_number());
			if((count = map_get(s.second.flags, FL_CAPTURED)))
				db.add_caps(id, guid, count);

			if((count = s.second.logged_time))
				db.add_time(id, guid, count);

//			for(siz_map_citer wu = p->second.weapon_usage.begin(); wu != p->second.weapon_usage.end(); ++wu)
			for(auto&& wu: s.second.weapon_usage)
				db.add_weapon_usage(id, guid, wu.first, wu.second);

//			for(moddmg_map_citer md = p->second.mod_damage.begin(); md != p->second.mod_damage.end(); ++md)
			for(auto&& md: s.second.mod_damage)
				db.add_mod_damage(id, guid, md.first, md.second.hits, md.second.damage
					, md.second.hitsRecv, md.second.damageRecv, md.second.weightedHits);

			db.add_playerstats_ps(id, guid, s.second);

			if(s.second.time && s.second.dist)
				db.add_speed(id, guid, s.second.dist, s.second.time, false);
			if(s.second.time_f && s.second.dist_f)
				db.add_speed(id, guid, s.second.dist_f, s.second.time_f, true);
		}

//		for(onevone_map_citer o = onevone.begin(); o != onevone.end(); ++o)
		for(auto&& o: onevone)
		{
			if(!allow_bots && clients[o.first].bot)
			{
				ptbug("IGNORING 1v1 BOT: " << clients[o.first].name);
				continue;
			}

			if(stats.at(o.first).hc < 100)
			{
				ptbug("IGNORING 1v1 HANDICAP PLAYER: [" << stats.at(o.first).hc << "] "
					<< clients[o.first].name);
				continue;
			}

//			for(guid_siz_map_citer p = o->second.begin(); p != o->second.end(); ++p)
			for(auto&& p: o.second)
			{
				if(!allow_bots && clients[p.first].bot)
				{
					ptbug("IGNORING 1v1 BOT: " << clients[p.first].name);
					continue;
				}

				if(stats.at(p.first).hc < 100)
				{
					ptbug("IGNORING 1v1 HANDICAP PLAYER: [" << stats.at(p.first).hc << "] "
						<< clients[p.first].name);
					continue;
				}

				db.add_ovo(id, clients[o.first].guid, clients[p.first].guid, p.second);
			}
		}

		ptlog("STATS WRITTEN IN: " << (std::time(0) - start) << " seconds:");
//	}, mapname, stats, onevone));

	stats.clear();
	onevone.clear();
	return true;
}

bool KatinaPluginStats::shutdown_game(siz min, siz sec)
{
	trace();

	in_game = false;

	if(!active)
		return true;

	stall_clients();

	return true;
}

void KatinaPluginStats::updatePlayerTime(slot num)
{
	trace();

	auto& s = stats[num];
    if(s.joined_time > 0)
    {
        s.logged_time += katina.now - s.joined_time;
        s.joined_time  = katina.now;
    }
}

void KatinaPluginStats::stall_client(slot num)
{
	trace();

	if(!stats[num].joined_time)
		return;

	// lock_guard lock(mtx);
	stats[num].logged_time += katina.now - stats[num].joined_time;
	stats[num].joined_time = 0;
}

void KatinaPluginStats::unstall_client(slot num)
{
	trace();

	if(stats[num].joined_time)
		return;

	if(clients[num].team != TEAM_R && clients[num].team != TEAM_B)
		return;

	// lock_guard lock(mtx);
	stats[num].joined_time = katina.now;
}

void KatinaPluginStats::stall_clients()
{
	trace();

	for(auto&& s: stats)
		stall_client(s.first);

//	for(guid_stat_map_citer ci = stats.begin(); ci != stats.end(); ++ci)
//		stall_client(ci->first);
}

void KatinaPluginStats::unstall_clients()
{
	trace();

	for(auto&& s: stats)
		unstall_client(s.first);
//	for(guid_stat_map_citer ci = stats.begin(); ci != stats.end(); ++ci)
//			unstall_client(ci->first);
}

void KatinaPluginStats::check_bots_and_players()
{
	trace();

	bool stats_stopped = stop_stats;

	stop_stats = false;
	siz human_players_r = 0;
	siz human_players_b = 0;
	siz bot_players_r = 0;
	siz bot_players_b = 0;

//	for(guid_siz_map_citer ci = teams.begin(); ci != teams.end(); ++ci)
	for(auto&& ci: clients)
	{
		if(!ci.live)
			continue;
		if(ci.bot)
		{
			if(ci.team == TEAM_R)
				++bot_players_r;
			else if(ci.team == TEAM_B)
				++bot_players_b;
			if(!allow_bots)
				stop_stats = true;
		}
		else
		{
			if(ci.team == TEAM_R)
				++human_players_r;
			else if(ci.team == TEAM_B)
				++human_players_b;
		}
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
	trace();

	if(allow_bots || !clients[num].bot)
	{
		// lock_guard lock(mtx);
		if(stats.count(guid))
		{
			if(clients[num].guid != guid)// || stats[num].hc != hc)
			{
				if(!archives.emplace(clients[num].guid, stats[num]).second)
				{
					plog("ERROR: archive already has record: " << clients[num].guid);
				}

				stats.erase(num);

				auto found = archives.find(guid);

				if(found != archives.end())
				{
					stats.emplace(num, *found);
				}
			}
		}
		stats[num].hc = hc;
		stats[num].name = name;
		stats[num].team = team;
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
	trace();

	if(!in_game)
		return true;

	if(!active)
		return true;

	check_bots_and_players();

	stats[num].team = TEAM_U;

	stall_client(num);

	return true;
}

bool KatinaPluginStats::kill(siz min, siz sec, slot num1, slot num2, siz weap)
{
	trace();

	if(!in_game)
		return true;

//	bug("STATS IN GAME");

	if(!active)
		return true;

//	bug("STATS ACTIVE");

	if(stop_stats)
		return true;

//	pbug("STATS NOT STOPPED");

	// lock_guard lock(mtx);
	if(num1 == slot::world) // no killer
		++stats[num2].deaths[weap];

	else if(allow_bots || (!clients[num1].bot && !clients[num2].bot))
	{
		if(num1 != num2)
		{
			++stats[num1].kills[weap];
			++onevone[num1][num2];

			// Target was a flag carrier
			if(num2 == carrierRed || num2 == carrierBlue)
			{
				++stats[num1].carrierFrags;
				++stats[num2].carrierFragsRecv;
			}
		}

		//if(!katina.getClientGuid(num2).is_bot())
		++stats[num2].deaths[weap];
	}

	return true;
}

bool KatinaPluginStats::ctf(siz min, siz sec, slot num, siz team, siz act)
{
	trace();

	if(!in_game)
		return true;

	// Remember who is carrying the flag
	if(team == TEAM_R)
		carrierRed = act == FL_TAKEN ? num : slot::bad;
	else if(team == TEAM_B)
		carrierBlue = act == FL_TAKEN ? num : slot::bad;

	if(!active)
		return true;

	if(stop_stats)
		return true;

	if(num == slot::bad)
	{
		if(act == 3) // flag returned after timeout
			return true;
		else
		{
			log("ERROR: Unexpected bad slot");
			return true;
		}
	}

	// lock_guard lock(mtx);
	++stats[num].flags[act];

	return true;
}

bool KatinaPluginStats::award(siz min, siz sec, slot num, siz awd)
{
	trace();

	if(!in_game)
		return true;

	if(!active)
		return true;

	if(stop_stats)
		return true;

	// lock_guard lock(mtx);
	++stats[num].awards[awd];

	return true;
}

bool KatinaPluginStats::init_game(siz min, siz sec, const str_map& cvars)
{
	trace();

	in_game = true;

	if(!active)
		return true;

	pbug("INITGAME");

	if(!announce_time)
		announce_time = sec + katina.get("boss.announce.delay", 10);

	katina.add_log_event(this, KE_HEARTBEAT);

	// lock_guard lock(mtx);
	{
		stats.clear();
		onevone.clear();
	}

	return true;
}

bool KatinaPluginStats::warmup(siz min, siz sec)
{
	trace();

	pbug("WARMUP");
	in_game = false;
	//stall_clients();

	// kybosch the announcement
	announce_time = 0;
	katina.del_log_event(this, KE_HEARTBEAT);

	return true;
}

void KatinaPluginStats::heartbeat(siz min, siz sec)
{
	if(!announce_time || min || sec < announce_time)
		return;

	bug_func();

	announce_time = 0; // turn off
	katina.del_log_event(this, KE_HEARTBEAT);

	pbug("HEARTBEAT");

	pbug_var(clients.size());

	db_scoper on(db);

	str boss;
	GUID guid;

	bug_var(guid);

	if(db.get_ingame_boss(mapname, clients, guid, boss) && guid != null_guid)
		server.msg_to_all("^7BOSS: " + katina.getPlayerName(guid) + "^7: " + boss, true);
	else
		server.msg_to_all("^7BOSS: ^3There is no boss on this map", true);
}

// mod_katina >= 0.1-beta
bool KatinaPluginStats::speed(siz min, siz sec, slot num, siz dist, siz time, bool has_flag)
{
	trace();

	if(!in_game)
		return true;

	if(!active)
		return true;

	if(stop_stats)
		return true;

	// lock_guard lock(mtx);

	struct stat& s = stats[num];

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
	trace();

	if(!in_game)
		return true;

	if(!active)
		return true;

	if(stop_stats)
		return true;

	// lock_guard lock(mtx);
	stats[num].weapon_usage[weapon] += shots;

	return true;
}

bool KatinaPluginStats::mod_damage(siz min, siz sec, slot num, siz mod, siz hits, siz damage, siz hitsRecv, siz damageRecv, float weightedHits)
{
	trace();

	if(!in_game)
		return true;

	if(!active)
		return true;

	if(stop_stats)
		return true;

	// lock_guard lock(mtx);
	auto& moddmg = stats[num].mod_damage[mod];
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
	trace();

	if(!in_game)
		return true;

	if(!active)
		return true;

	if(stop_stats)
		return true;

	// lock_guard lock(mtx);
	auto& s	 = stats[num];
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

bool KatinaPluginStats::sayteam(siz min, siz sec, slot num, const str& text)
{
	trace();

	return say(min, sec, num, text);
}

bool KatinaPluginStats::check_slot(slot num)
{
	trace();

	if(!katina.check_slot(num))
	{
		plog("WARN: Unknown client number: " << num);
		server.chat_nobeep("^7!STATS: ^3Unknown client number: ^7" + to_string(num));
		return false;
	}
	return true;
}

bool KatinaPluginStats::say(siz min, siz sec, slot num, const str& text)
{
	trace();

	if(!active)
		return true;

	if(!check_slot(num))
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
			server.msg_to(num, PREFIX + "^3!register = select your current name to dosplay in the web stats", true);
			server.msg_to(num, PREFIX + "^3 The web stats can be found at ^7http:^7/^7/^377.237.250.186^7:^381^7/^3webkatti^7/^3oa-ictf");
			server.msg_to(num, PREFIX + "^3 ^7(^3we hope to get a better URL soon^7)");
			return true;
		}

		if(write && clients[num].name != "UnnamedPlayer" && clients[num].name != "RenamedPlayer")
		{
			db_scoper on(db);
			if(db.set_preferred_name(clients[num].guid, clients[num].name))
				server.chat(PREFIX + clients[num].name + "^7: ^3Your preferred name has been registered.");
		}
	}
	else if(cmd == "!help" || cmd == "?help")
	{
		server.msg_to(num, PREFIX + "^2?stats^7, ^2?boss^7, ^2?champ");
	}
	else if(cmd == "!stats" || cmd == "?stats")
	{
		if(cmd[0] == '?')
		{
			server.msg_to(num, PREFIX + "^3!stats <1-3>? = give stats for this month or", true);
			server.msg_to(num, PREFIX + "^3optionally 1-3 months previously.");
			server.msg_to(num, PREFIX + "^3FH ^7(^2frags^7/^2hour^7)");
			server.msg_to(num, PREFIX + "^3CH ^7(^2caps^7/^2hour^7)");
			server.msg_to(num, PREFIX + "^3SP ^7(^2average speed in u^7/^2second^7)");
			server.msg_to(num, PREFIX + "^3SK ^7(^2skill rating^7)");
			return true;
		}

		siz prev = 0; // count $prev month's back
		if(!(iss >> prev))
			prev = 0;

		bug_var(prev);

		bug("getting stats");

		str stats;
		siz idx = 0;
		str stats_c;
		siz idx_c = 0;
		db_scoper on(db);
		if(db.get_ingame_stats(clients[num].guid, mapname, prev, stats, idx))
		{
			str skill = to_string(idx);
			for(siz i = 0; i < 3; ++i)
				skill = skill.size() < 4 ? (" " + skill) : skill;
			server.msg_to_all(stats + " ^7" + clients[num].name);
		}
		if(db.get_ingame_stats_c(mapname, clients, clients[num].guid, prev, stats_c, idx_c))
		{
			if(stats_c != stats || idx_c != idx)
			{
				log("ERROR: get_ingame_stats()  -> " << stats << "(" << idx << ")");
				log("ERROR: get_ingame_stats_c()-> " << stats_c << "(" << idx_c << ")");
			}
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
			server.msg_to(num, PREFIX + "^2!boss^7: ^3display this map's best player and their ^2!stats ^3for this month.", true);
			server.msg_to(num, PREFIX + "^2!boss^7: ^3out of all the players currently connected.");
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
			server.msg_to(num, PREFIX + "^2!crap^7: ^3display this map's crappiest player (who cause the most holy-craps).", true);
			server.msg_to(num, PREFIX + "^2!crap^7: ^3out of all the players currently connected.");
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
	trace();

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
		set_blob(blob, stats);
		return "OK:";
	}

	return KatinaPlugin::api(cmd);//"ERROR: unknown request";
}

siz KatinaPluginStats::get_skill(const GUID& guid, const str& mapname)
{
	trace();

	static str stats;
	static siz skill;
	static siz skill_c;

	db_scoper on(db);
	if(!db.get_ingame_stats_c(mapname, clients, guid, 0, stats, skill_c))
		skill_c = 0;
	if(!db.get_ingame_stats(guid, mapname, 0, stats, skill))
		skill = 0;

	if(skill != skill_c)
	{
		log("ERROR: get_ingame_stats()  -> " << skill);
		log("ERROR: get_ingame_stats_c()-> " << skill_c);
	}

	return skill;
}

void KatinaPluginStats::close()
{
	trace();
}

// StatsDatabaseMySql

const game_id bad_id(-1);
const game_id null_id(0);

static const str playerstats_sql = "insert into `playerstats` values (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)";

void StatsDatabaseMySql::init()
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

			bind_stmt(stmt_add_playerstats, bind_add_playerstats);
		}
	}
}

void StatsDatabaseMySql::deinit()
{
	if(stmt_add_playerstats)
		mysql_stmt_close(stmt_add_playerstats);

	stmt_add_playerstats = 0;
}
//   game: game_id host port date map

bool mysql_timestamp(std::time_t timet, str& timestamp)
{
	char timef[32];// = "0000-00-00 00:00:00";

	siz times = 0;
	//time_t timet = std::time(0);
	if(!(times = strftime(timef, sizeof(timef), "%F %T", gmtime(&timet))))
	{
		log("ERROR: converting time: " << timet);
		return false;
	}

	timestamp.assign(timef, times);

	return true;
}

game_id StatsDatabaseMySql::add_game(const std::time_t timet, const str& host, const str& port, const str& mapname)
{
	if(dbtrace)
		log("DATABASE: add_game(" << timet << ", " << host << ", " << port << ", " << mapname << ")");

	str safe_mapname;
	if(!escape(mapname, safe_mapname))
	{
		log("DATABASE: ERROR: failed to escape: " << mapname);
		return bad_id;
	}

	str timestamp;
	if(!mysql_timestamp(timet, timestamp))
	{
		log("DATABASE: ERROR: failed to convert date: " << timet);
		return bad_id;
	}

	str sql = "insert into `game`"
		" (`host`, `port`, `date`, `map`) values (INET_ATON('"
		+ host + "'),'" + port + "','" + timestamp + "','" + safe_mapname + "')";

	game_id id;

	if(!insert(sql, id)||id == my_ulonglong(-1))
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
bool StatsDatabaseMySql::add_weaps(game_id id, const str& table, const GUID& guid, siz weap, siz count)
{
	if(dbtrace)
		log("DATABASE: add_weaps(" << id << ", " << table << ", " << guid << ", " << weap << ", " << count << ")");

	soss oss;
	oss << "insert into `" << table << "` (`game_id`, `guid`, `weap`, `count`) values (";
	oss << "'" << id << "','" << guid << "','" << weap << "','" << count << "')";

	str sql = oss.str();

	return insert(sql);
}

bool StatsDatabaseMySql::add_caps(game_id id, const GUID& guid, siz count)
{
	if(dbtrace)
		log("DATABASE: add_caps(" << id << ", " << guid << ", " << count << ")");

	soss oss;
	oss << "insert into `caps` (`game_id`, `guid`, `count`) values (";
	oss << "'" << id << "','" << guid << "','" << count << "')";

	str sql = oss.str();

	return insert(sql);
}

bool StatsDatabaseMySql::add_time(game_id id, const GUID& guid, siz count)
{
	if(dbtrace)
		log("DATABASE: add_time(" << id << ", " << guid << ", " << count << ")");

	soss oss;
	oss << "insert into `time` (`game_id`, `guid`, `count`) values (";
	oss << "'" << id << "','" << guid << "','" << count << "')";

	str sql = oss.str();

	return insert(sql);
}

bool StatsDatabaseMySql::add_player(const std::time_t timet, const GUID& guid, const str& name)
{
	if(dbtrace)
		log("DATABASE: add_player(" << timet << ", " << guid << ", " << name << ")");

	str safe_name;
	if(!escape(name, safe_name))
	{
		log("DATABASE: ERROR: failed to escape: " << name);
		return false;
	}

	str timestamp;
	if(!mysql_timestamp(timet, timestamp))
	{
		log("DATABASE: ERROR: failed to convert date: " << timet);
		return false;
	}

	soss oss;
	oss << "insert into `player` (`guid`,`name`,`date`) values ('" << guid << "','" << safe_name
		<< "','" << timestamp << "') ON DUPLICATE KEY UPDATE count = count + 1, `date` = '" << timestamp << "'";

	str sql = oss.str();

	return insert(sql);
}

bool StatsDatabaseMySql::add_ovo(game_id id, const GUID& guid1, const GUID& guid2, siz count)
{
	if(dbtrace)
		log("DATABASE: add_ovo(" << id << ", " << guid1 << ", " << guid2 << ", " << count << ")");

	soss oss;
	oss << "insert into `ovo` (`game_id`,`guid1`,`guid2`,`count`) values ('"
		<< id << "','" << guid1 << "','" << guid2 << "','" << count << "')";

	str sql = oss.str();

	return insert(sql);
}


bool StatsDatabaseMySql::add_weapon_usage(game_id id, const GUID& guid, siz weap, siz shots)
{
	if(dbtrace)
		log("DATABASE: add_weapon_usage(" << id << ", " << guid << ", " << weap << ", " << shots << ")");

	soss oss;
	oss << "insert into `weapon_usage` (`game_id`,`guid`,`weap`,`shots`) values ('"
		<< id << "','" << guid << "','" << weap << "','" << shots << "')";

	str sql = oss.str();

	return insert(sql);
}

bool StatsDatabaseMySql::add_mod_damage(game_id id, const GUID& guid, siz mod, siz hits, siz damage, siz hitsRecv, siz damageRecv, float weightedHits)
{
	if(dbtrace)
		log("DATABASE: add_mod_damage(" << id << ", " << guid << ", " << mod << ", " << hits << ", " << damage << ", " << hitsRecv << ", " << damageRecv << ", " << weightedHits << ")");

	soss oss;
	oss << "insert into `damage` (`game_id`,`guid`,`mod`,`hits`,`dmgDone`,`hitsRecv`,`dmgRecv`,`weightedHits`) values ('"
		<< id << "','" << guid << "','" << mod << "','" << hits << "','" << damage << "','" << hitsRecv << "','" << damageRecv << "','" << weightedHits << "')";

	str sql = oss.str();

	return insert(sql);
}

bool StatsDatabaseMySql::add_playerstats(game_id id, const GUID& guid,
	siz fragsFace, siz fragsBack, siz fraggedInFace, siz fraggedInBack,
	siz spawnKills, siz spawnKillsRecv, siz pushes, siz pushesRecv,
	siz healthPickedUp, siz armorPickedUp, siz holyShitFrags, siz holyShitFragged,
	siz carrierFrags, siz carrierFragsRecv)
{
	if(dbtrace)
		log("DATABASE: add_playerstats('" << id << ", " << guid << ", " << fragsFace << ", " << fragsBack << ", " << fraggedInFace << ", " << fraggedInBack
			<< ", " << spawnKills << ", " << spawnKillsRecv << ", " << pushes << ", " << pushesRecv << ", " << healthPickedUp << ", " << armorPickedUp
			<< ", " << holyShitFrags << ", " << holyShitFragged << ", " << carrierFrags << ", " << carrierFragsRecv << ")");

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
bool StatsDatabaseMySql::add_playerstats_ps(game_id id, const GUID& guid,
	siz fragsFace, siz fragsBack, siz fraggedInFace, siz fraggedInBack,
	siz spawnKills, siz spawnKillsRecv, siz pushes, siz pushesRecv,
	siz healthPickedUp, siz armorPickedUp, siz holyShitFrags, siz holyShitFragged,
	siz carrierFrags, siz carrierFragsRecv)
{
	if(dbtrace)
		log("DATABASE: add_playerstats_ps('" << id << ", " << guid << ", " << fragsFace << ", " << fragsBack << ", " << fraggedInFace << ", " << fraggedInBack
			<< ", " << spawnKills << ", " << spawnKillsRecv << ", " << pushes << ", " << pushesRecv << ", " << healthPickedUp << ", " << armorPickedUp
			<< ", " << holyShitFrags << ", " << holyShitFragged << ", " << carrierFrags << ", " << carrierFragsRecv << ")");

	if(!stmt_add_playerstats)
	{
		log("DATABASE:  WARN: stored procedure not set");
		log("DATABASE:      : using fall-back");
		return add_playerstats(id, guid, fragsFace, fragsBack, fraggedInFace, fraggedInBack,
			spawnKills, spawnKillsRecv, pushes, pushesRecv,
			healthPickedUp, armorPickedUp, holyShitFrags, holyShitFragged,
			carrierFrags, carrierFragsRecv);
	}

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

bool StatsDatabaseMySql::add_playerstats_ps(game_id id, const GUID& guid, const struct stat& s)
{
	if(dbtrace)
		log("DATABASE: add_playerstats_ps('"
			<< id << ", " << guid
			<< ", " << s.fragsFace
			<< ", " << s.fragsBack
			<< ", " << s.fraggedInFace
			<< ", " << s.fraggedInBack
			<< ", " << s.spawnKills
			<< ", " << s.spawnKillsRecv
			<< ", " << s.pushes
			<< ", " << s.pushesRecv
			<< ", " << s.healthPickedUp
			<< ", " << s.armorPickedUp
			<< ", " << s.holyShitFrags
			<< ", " << s.holyShitFragged
			<< ", " << s.carrierFrags
			<< ", " << s.carrierFragsRecv
			<< ")");

	if(!stmt_add_playerstats)
	{
		log("DATABASE:  WARN: stored procedure not set");
		log("DATABASE:      : using fall-back");
		return add_playerstats(id, guid, s.fragsFace, s.fragsBack, s.fraggedInFace, s.fraggedInBack,
			s.spawnKills, s.spawnKillsRecv, s.pushes, s.pushesRecv,
			s.healthPickedUp, s.armorPickedUp, s.holyShitFrags, s.holyShitFragged,
			s.carrierFrags, s.carrierFragsRecv);
	}

	siz j = 0;
	siz_add_playerstats[j++] = id;
	std::strncpy(guid_add_playerstats.data(), str(guid).c_str(), 8);
	guid_length = str(guid).size();
	siz_add_playerstats[j++] = s.fragsFace;
	siz_add_playerstats[j++] = s.fragsBack;
	siz_add_playerstats[j++] = s.fraggedInFace;
	siz_add_playerstats[j++] = s.fraggedInBack;
	siz_add_playerstats[j++] = s.spawnKills;
	siz_add_playerstats[j++] = s.spawnKillsRecv;
	siz_add_playerstats[j++] = s.pushes;
	siz_add_playerstats[j++] = s.pushesRecv;
	siz_add_playerstats[j++] = s.healthPickedUp;
	siz_add_playerstats[j++] = s.armorPickedUp;
	siz_add_playerstats[j++] = s.holyShitFrags;
	siz_add_playerstats[j++] = s.holyShitFragged;
	siz_add_playerstats[j++] = s.carrierFrags;
	siz_add_playerstats[j++] = s.carrierFragsRecv;

	if(mysql_stmt_execute(stmt_add_playerstats))
	{
		log("DATABASE: ERROR: " << mysql_stmt_error(stmt_add_playerstats));
		log("DATABASE:      : using fall-back");
		return add_playerstats(id, guid, s.fragsFace, s.fragsBack, s.fraggedInFace, s.fraggedInBack,
			s.spawnKills, s.spawnKillsRecv, s.pushes, s.pushesRecv,
			s.healthPickedUp, s.armorPickedUp, s.holyShitFrags, s.holyShitFragged,
			s.carrierFrags, s.carrierFragsRecv);
	}

	if(mysql_stmt_affected_rows(stmt_add_playerstats) != 1)
	{
		log("DATABASE: ERROR: add_playerstats_ps: no rows affected");
		log("DATABASE:      : using fall-back");
		return add_playerstats(id, guid, s.fragsFace, s.fragsBack, s.fraggedInFace, s.fraggedInBack,
			s.spawnKills, s.spawnKillsRecv, s.pushes, s.pushesRecv,
			s.healthPickedUp, s.armorPickedUp, s.holyShitFrags, s.holyShitFragged,
			s.carrierFrags, s.carrierFragsRecv);
	}

	return true;
}

bool StatsDatabaseMySql::add_speed(game_id id, const GUID& guid,
	siz dist, siz time, bool has_flag)
{
	if(dbtrace)
		log("DATABASE: add_speed(" << id << ", " << guid << ", " << dist << ", " << time << ", " << has_flag << ")");

	soss oss;
	oss << "insert into `speed` ("
	    << "`game_id`,`guid`,`dist`,`time`,`flag`) "
	    << "values ('" << id << "','" << guid << "','" << dist << "','" << time << "','" << has_flag << "')";

	str sql = oss.str();

	return insert(sql);
}

bool StatsDatabaseMySql::read_map_votes(const str& mapname, guid_int_map& map_votes)
{
	if(dbtrace)
		log("DATABASE: read_map_votes(" << mapname << ")");

	map_votes.clear();

	str safe_mapname;
	if(!escape(mapname, safe_mapname))
	{
		log("DATABASE: ERROR: failed to escape: " << mapname);
		return bad_id;
	}

	soss oss;
	oss << "select `guid`,`count` from `votes` where `type` = 'map' and `item` = '" << safe_mapname << "'";

	str sql = oss.str();

	str_vec_vec rows;
	if(!select(sql, rows, 2))
		return false;

	for(siz i = 0; i < rows.size(); ++i)
	{
		if(dbtrace)
			log("DATABASE: restoring vote: " << rows[i][0] << ": " << rows[i][1]);
		map_votes[GUID(rows[i][0])] = to<int>(rows[i][1]);
	}

	return true;
}

bool StatsDatabaseMySql::set_preferred_name(const GUID& guid, const str& name)
{
	if(dbtrace)
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

bool StatsDatabaseMySql::get_preferred_name(const GUID& guid, str& name)
{
	if(dbtrace)
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

	return true;
}

str get_game_select_period(const str& mapname, siz prev = 0)
{
	siz syear = 0;
	siz smonth = 0;
	siz eyear = 0;
	siz emonth = 0;

	if(!calc_period(syear, smonth, eyear, emonth, prev))
		return "";

	soss sql;
	sql << "select `game_id` from `game` where `map` = '" << mapname << "'";
	sql << " and `date` >= TIMESTAMP('" << syear << '-' << (smonth < 10 ? "0":"") << smonth << '-' << "01" << "')";
	sql << " and `date` <  TIMESTAMP('" << eyear << '-' << (emonth < 10 ? "0":"") << emonth << '-' << "01" << "')";

	return sql.str();
}

bool StatsDatabaseMySql::get_ingame_champ(const str& mapname, GUID& guid, str& stats)
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

siz StatsDatabaseMySql::get_kills_per_cap(const str& sql_select_games)
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

bool StatsDatabaseMySql::get_ingame_boss(const str& mapname, const client_arr& clients, GUID& guid, str& stats)
{
	if(dbtrace)
		log("DATABASE: get_ingame_boss(" << mapname << ", " << clients.size() << ")");
//	siz syear = 0;
//	siz smonth = 0;
//	siz eyear = 0;
//	siz emonth = 0;
//
//	if(!calc_period(syear, smonth, eyear, emonth))
//		return false;
//
//	soss oss;
//	oss << "select `game_id` from `game` where `map` = '" << mapname << "'";
//	oss << " and `date` >= TIMESTAMP('" << syear << '-' << (smonth < 10 ? "0":"") << smonth << '-' << "01" << "')";
//	oss << " and `date` <  TIMESTAMP('" << eyear << '-' << (emonth < 10 ? "0":"") << emonth << '-' << "01" << "')";
//	str sql_select_games = oss.str();

	str sql_select_games = get_game_select_period(mapname);

	stat_map stat_cs;
	str_set guids;
	soss oss;

	str sep;
	oss.clear();
	oss.str("");
	for(auto&& client: clients)
	{
		if(!client.live || client.bot)
			continue;
		oss << sep << "'" << client.guid << "'";
		sep = ",";
	}
//	for(slot_guid_map_citer i = clients.begin(); i != clients.end(); ++i)
//		if(!i->second.is_bot())
//			{ oss << sep << "'" << i->second << "'"; sep = ",";}

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

	bug_var(sql);

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

bool StatsDatabaseMySql::get_ingame_stats_c(const str& mapname, const client_arr& clients, const GUID& guid, siz prev, str& stats, siz& skill)
{
	if(dbtrace)
		log("DATABASE: get_ingame_stats_c(" << guid << ", " << mapname << ", " << prev << ")");

	if(mapname.empty())
		return false;

	// cache
	struct core_stat
	{
		siz kills;
		siz shots;
		siz hits;
		siz caps;
		siz secs;

		siz time; // speed
		siz dist; // speed
	};

	TYPEDEF_MAP(GUID, core_stat, guid_cstat_map);

	static siz cache_kpc = 0;
	static str cache_mapname = mapname;
	static guid_cstat_map cache;

	if(cache_mapname != mapname)
	{
		cache.clear();
		cache_kpc = 0;
		cache_mapname = mapname;
	}

//	const str key = str(guid) + "-" + std::to_string(prev);

	if(cache.find(guid) != cache.end())
		bug("CACHE  HIT: " << str(guid));
	else
	{
		bug("CACHE MISS: " << str(guid));

		str sql_select_games = get_game_select_period(mapname, prev);
		cache_kpc = get_kills_per_cap(sql_select_games);

		soss sql;

		str sep;
		sql.clear();
		sql.str("");
		for(auto&& client: clients)
		{
			if(!client.live || client.bot || cache.find(client.guid) != cache.end())
				continue;
			sql << sep << "'" << client.guid << "'";
			sep = ",";
		}
//		for(slot_guid_map_citer i = clients.begin(); i != clients.end(); ++i)
//			if(!i->second.is_bot() && cache.find(i->second) == cache.end())
//				{ sql << sep << "'" << i->second << "'"; sep = ",";}
		str insql = sql.str();

		// kills

		sql.clear();
		sql.str("");
		sql << "select distinct guid,sum(count) from kills";
		sql << " where game_id in (" << sql_select_games << ")";
		sql << " and guid in (" << insql << ")";
		sql << " group by guid";
		bug_var(sql.str());

		str_vec_vec rows;

		if(!select(sql.str(), rows, 2))
			return false;

		for(const str_vec& row: rows)
			cache[GUID(row[0])].kills = to<siz>(row[1]);

		// shots

		sql.clear();
		sql.str("");
		sql << "select distinct guid,sum(shots) from weapon_usage";
		sql << " where weap = '7'"; // FIXME: railgun only (not good for AW)
		sql << " and game_id in (" << sql_select_games << ")";
		sql << " and guid in (" << insql << ")";
		sql << " group by guid";
		bug_var(sql.str());

		if(!select(sql.str(), rows, 2))
			return false;

		for(const str_vec& row: rows)
			cache[GUID(row[0])].shots = to<siz>(row[1]);

		// hits

		sql.clear();
		sql.str("");
		sql << "select distinct guid,sum(hits) from damage";
		sql << " where `mod` = '10'"; // FIXME: railgun only (not good for AW)
		sql << " and game_id in (" << sql_select_games << ")";
		sql << " and guid in (" << insql << ")";
		sql << " group by guid";
		bug_var(sql.str());

		if(!select(sql.str(), rows, 2))
			return false;

		for(const str_vec& row: rows)
			cache[GUID(row[0])].hits = to<siz>(row[1]);

		// caps

		sql.clear();
		sql.str("");
		sql << "select distinct guid,sum(count) from caps";
		sql << " where game_id in (" << sql_select_games << ")";
		sql << " and guid in (" << insql << ")";
		sql << " group by guid";
		bug_var(sql.str());

		if(!select(sql.str(), rows, 2))
			return false;

		for(const str_vec& row: rows)
			cache[GUID(row[0])].caps = to<siz>(row[1]);

		// speed

		sql.clear();
		sql.str("");
		sql << "select distinct guid,sum(time),sum(dist) from speed";
		sql << " where game_id in (" << sql_select_games << ")";
		sql << " and guid in (" << insql << ")";
		sql << " group by guid";
		bug_var(sql.str());

		if(!select(sql.str(), rows, 3))
			return false;

		for(const str_vec& row: rows)
		{
			cache[GUID(row[0])].time = to<siz>(row[1]);
			cache[GUID(row[0])].dist = to<siz>(row[2]);
		}

		// secs

		sql.clear();
		sql.str("");
		sql << "select distinct guid,sum(count) from time";
		sql << " where game_id in (" << sql_select_games << ")";
		sql << " and guid in (" << insql << ")";
		sql << " group by guid";
		bug_var(sql.str());

		if(!select(sql.str(), rows, 2))
			return false;

		for(const str_vec& row: rows)
			cache[GUID(row[0])].secs = to<siz>(row[1]);
	}

	// speed

	const core_stat& s = cache[guid];

	siz t = s.time;
	siz d = s.dist;

	siz ups = 0; // u/sec

	if(t)
		ups = d / t;

	//

	siz fph = s.kills;
	siz acc = s.shots;
	siz hit = s.hits;
	siz cph = s.caps;
	siz sec = s.secs;

	stats = "^7<^3not recorded for this map^7>";

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
		siz kpc = cache_kpc;
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

bool StatsDatabaseMySql::get_ingame_stats(const GUID& guid, const str& mapname, siz prev, str& stats, siz& skill)
{
	if(dbtrace)
		log("DATABASE: get_ingame_stats(" << guid << ", " << mapname << ", " << prev << ")");

	if(mapname.empty())
		return false;

	str sql_select_games = get_game_select_period(mapname, prev);

	soss sql;

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

// TODO: Make boss, champ & stats return a proper GUID like this does scanning the clients
// TODO: add some minumum requirements like minimum time/frags/caps etc...
bool StatsDatabaseMySql::get_ingame_crap(const str& mapname, const client_arr& clients, GUID& guid, str& stats)
{
	if(dbtrace)
		log("DATABASE: get_ingame_crap(" << mapname << ", " << clients.size() << ")");

	str sql_select_games = get_game_select_period(mapname);

	stat_map stat_cs;
	str_set guids;
	soss oss;

	str sep;
	oss.clear();
	oss.str("");
	for(auto&& client: clients)
	{
		if(!client.live || client.bot)
			continue;
		oss << sep << "'" << client.guid << "'";
		sep = ",";
	}
//	for(slot_guid_map_citer i = clients.begin(); i != clients.end(); ++i)
//		if(!i->second.is_bot())
//			{ oss << sep << "'" << i->second << "'"; sep = ",";}
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

	// TODO:
//	need to track when GUID changes for a given slot and squirrel away the stats for the
//	previous GUID until the and: a stats archive
//	remember to store the archives in the db along with the other stats
//	NOTE: this will break the regression tests for sure

	slot maxi = slot::bad;
	double maxv = 0.0;

	for(slot i(0); i < slot::max; ++i)
	{
		const auto& client = clients[i];

		if(!client.live)
			continue;

		const str sguid = str(client.guid);
		if(stat_cs[sguid].secs)
		{
			stat_cs[sguid].fph = stat_cs[sguid].kills * 60 * 60 / stat_cs[sguid].secs;
			if(stat_cs[sguid].fph > maxv)
			{
				maxv = stat_cs[sguid].fph;
				maxi = i;
			}
		}
	}

	if(maxi != slot::bad)
	{
		if(!clients[maxi].bot)
		{
			soss oss;
			oss << "^3FCraps/Hour^7:^2" << stat_cs[str(clients[maxi].guid)].fph;
			stats = oss.str();
//			bug_var(stats);
			guid = clients[maxi].guid;
		}
	}

	return true;
}

}} // Namespace katina::plugin
