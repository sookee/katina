//#pragma once
#ifndef _OASTATS_KATINA_PLUGIN_STATS_H
#define	_OASTATS_KATINA_PLUGIN_STATS_H
/*
 * File:   KatinaPluginStats.h
 * Author: SooKee oasookee@gmail.com
 *
 * Created on April 27, 2013, 10:02 AM
 */

/*-----------------------------------------------------------------.
| Copyright (C) 2013 SooKee oasookee@gmail.com               |
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
#include <map>
#include <utility>
#include <mutex>

#include <katina/Katina.h>
#include <katina/KatinaPlugin.h>

#include <katina/Database.h>
#include <katina/GUID.h>
#include <katina/codes.h>

#include <katina/types.h>
#include <katina/log.h>

namespace katina { namespace plugin {

using namespace katina;
using namespace katina::log;
using namespace katina::data;
using namespace katina::types;

struct mod_damage_stats
{
	siz hits;
	siz damage;
	siz hitsRecv;
	siz damageRecv;
	float weightedHits;

	mod_damage_stats() :
		hits(0), damage(0), hitsRecv(0), damageRecv(0), weightedHits(0.0f)
	{}
};

TYPEDEF_MAP(siz, mod_damage_stats, moddmg_map);

struct stat
{
	siz hc; // handicap

	siz_map kills;
	siz_map deaths;
	siz_map flags;
	siz_map awards;

	siz_map weapon_usage; // shots fired
	moddmg_map mod_damage;   // MOD -> mod_damage_stats

	siz fragsFace;
	siz fragsBack;
	siz fraggedInFace;
	siz fraggedInBack;
	siz spawnKills;
	siz spawnKillsRecv;
	siz pushes;
	siz pushesRecv;
	siz healthPickedUp;
	siz armorPickedUp;
	siz holyShitFrags;
	siz holyShitFragged;

	siz carrierFrags;
	siz carrierFragsRecv;

	siz time, time_f;
	siz dist, dist_f;

	time_t joined_time;
	siz logged_time;

	str name;

	siz team;

	stat() :
		hc(100), kills(), deaths(), flags(), awards(), weapon_usage(), mod_damage(),
		fragsFace(0), fragsBack(0), fraggedInFace(0), fraggedInBack(0),
		spawnKills(0), spawnKillsRecv(0), pushes(0), pushesRecv(0),
		healthPickedUp(0), armorPickedUp(0), holyShitFrags(0), holyShitFragged(0),
		carrierFrags(0), carrierFragsRecv(0), time(0), dist(0), time_f(0), dist_f(0),
		joined_time(0), logged_time(0), team(TEAM_U)
	{}
};

TYPEDEF_MAP(GUID, guid_siz_map, onevone_map);
TYPEDEF_MAP(GUID, stat, guid_stat_map);

typedef my_ulonglong game_id;
extern const game_id bad_id;
extern const game_id null_id;

class StatsDatabase
: public Database
{
public:
	StatsDatabase(): Database() {};
	virtual ~StatsDatabase() {};

	virtual void set_trace(bool state = true) = 0;

	virtual game_id add_game(std::time_t timet, const str& host
		, const str& port, const str& mapname) = 0;

	/**
	 *
	 * @param id
	 * @param table "kills" | "deaths"
	 * @param guid
	 * @param weap
	 * @param count
	 * @return
	 */
	virtual bool add_weaps(game_id id, const str& table
		, const GUID& guid, siz weap, siz count) = 0;

	virtual bool add_caps(game_id id, const GUID& guid, siz count) = 0;
	virtual bool add_time(game_id id, const GUID& guid, siz count) = 0;

	virtual bool add_player(std::time_t timet, const GUID& guid, const str& name) = 0;

	virtual bool add_ovo(game_id id, const GUID& guid1, const GUID& guid2, siz count) = 0;

	virtual bool add_weapon_usage(game_id id, const GUID& guid, siz weap, siz shots) = 0;
	virtual bool add_mod_damage(game_id id, const GUID& guid, siz mod, siz hits
		, siz damage, siz hitsRecv, siz damageRecv, float weightedHits) = 0;
	virtual bool add_playerstats(game_id id, const GUID& guid,
		siz fragsFace, siz fragsBack, siz fraggedInFace, siz fraggedInBack,
		siz spawnKills, siz spawnKillsRecv, siz pushes, siz pushesRecv,
		siz healthPickedUp, siz armorPickedUp, siz holyShitFrags, siz holyShitFragged,
		siz carrierFrags, siz carrierFragsRecv) = 0;
	virtual bool add_playerstats_ps(game_id id, const GUID& guid,
		siz fragsFace, siz fragsBack, siz fraggedInFace, siz fraggedInBack,
		siz spawnKills, siz spawnKillsRecv, siz pushes, siz pushesRecv,
		siz healthPickedUp, siz armorPickedUp, siz holyShitFrags, siz holyShitFragged,
		siz carrierFrags, siz carrierFragsRecv) = 0;
	virtual bool add_playerstats_ps(game_id id, const GUID& guid
		, const struct stat& s) = 0;
	virtual bool add_speed(game_id id, const GUID& guid,
			siz dist, siz time, bool has_flag) = 0;

	virtual bool read_map_votes(const str& mapname, guid_int_map& map_votes) = 0;

	virtual bool set_preferred_name(const GUID& guid, const str& name) = 0;
	virtual bool get_preferred_name(const GUID& guid, str& name) = 0;

	virtual siz get_kills_per_cap(const str& sql_select_games = "") = 0;
	virtual bool get_ingame_boss(const str& mapname
		, const client_arr& clients, GUID& guid, str& stats) = 0;
	virtual bool get_ingame_champ(const str& mapname, GUID& guid, str& stats) = 0;
	virtual bool get_ingame_stats(const GUID& guid, const str& mapname
		, siz prev, str& stats, siz& skill) = 0;
	virtual bool get_ingame_stats_c(const str& mapname, const client_arr& clients
		, const GUID& guid, siz prev, str& stats, siz& skill) = 0;
};

//class StatsDatabaseFile
//: public StatsDatabase
//{
//public:
//	StatsDatabaseFile(): StatsDatabase() {};
//	virtual ~StatsDatabaseFile() {};
//
//	void init() override {}
//	void deinit() override {}
//
//	virtual void set_trace(bool state = true) {}
//
//	virtual game_id add_game(std::time_t timet, const str& host
//		, const str& port, const str& mapname) override;
//
//	/**
//	 *
//	 * @param id
//	 * @param table "kills" | "deaths"
//	 * @param guid
//	 * @param weap
//	 * @param count
//	 * @return
//	 */
//	virtual bool add_weaps(game_id id, const str& table
//		, const GUID& guid, siz weap, siz count) override { return true; }
//
//	virtual bool add_caps(game_id id, const GUID& guid, siz count) override;
//	virtual bool add_time(game_id id, const GUID& guid, siz count) override;
//
//	virtual bool add_player(std::time_t timet, const GUID& guid, const str& name) override;
//
//	virtual bool add_ovo(game_id id, const GUID& guid1, const GUID& guid2, siz count) override;
//
//	virtual bool add_weapon_usage(game_id id, const GUID& guid, siz weap, siz shots) override { return true; }
//	virtual bool add_mod_damage(game_id id, const GUID& guid, siz mod, siz hits
//		, siz damage, siz hitsRecv, siz damageRecv, float weightedHits) override { return true; }
//	virtual bool add_playerstats(game_id id, const GUID& guid,
//		siz fragsFace, siz fragsBack, siz fraggedInFace, siz fraggedInBack,
//		siz spawnKills, siz spawnKillsRecv, siz pushes, siz pushesRecv,
//		siz healthPickedUp, siz armorPickedUp, siz holyShitFrags, siz holyShitFragged,
//		siz carrierFrags, siz carrierFragsRecv) override { return true; }
//	virtual bool add_playerstats_ps(game_id id, const GUID& guid,
//		siz fragsFace, siz fragsBack, siz fraggedInFace, siz fraggedInBack,
//		siz spawnKills, siz spawnKillsRecv, siz pushes, siz pushesRecv,
//		siz healthPickedUp, siz armorPickedUp, siz holyShitFrags, siz holyShitFragged,
//		siz carrierFrags, siz carrierFragsRecv) override { return true; }
//	virtual bool add_playerstats_ps(game_id id, const GUID& guid
//		, const struct stat& s) override { return true; }
//	virtual bool add_speed(game_id id, const GUID& guid,
//			siz dist, siz time, bool has_flag) override { return true; }
//
//	virtual bool read_map_votes(const str& mapname, guid_int_map& map_votes) override;
//
//	virtual bool set_preferred_name(const GUID& guid, const str& name) override { return true; }
//	virtual bool get_preferred_name(const GUID& guid, str& name) override { return true; }
//
//	virtual siz get_kills_per_cap(const str& sql_select_games = "") override { return true; }
//	virtual bool get_ingame_boss(const str& mapname
//		, const slot_guid_map& clients, GUID& guid, str& stats) override { return true; }
//	virtual bool get_ingame_champ(const str& mapname, GUID& guid, str& stats) override { return true; }
//	virtual bool get_ingame_stats(const GUID& guid, const str& mapname
//		, siz prev, str& stats, siz& skill) override { return true; }
//	virtual bool get_ingame_stats_c(const str& mapname, const slot_guid_map& clients
//		, const GUID& guid, siz prev, str& stats, siz& skill) override { return true; }
//};

class StatsDatabaseMySql
: public StatsDatabase
{
	bool dbtrace = false;

	MYSQL_STMT* stmt_add_playerstats = 0;
	std::array<MYSQL_BIND, 16> bind_add_playerstats;
	std::array<siz, 15> siz_add_playerstats;
	std::array<char, 8> guid_add_playerstats;
	siz guid_length = 8;

public:
	StatsDatabaseMySql(): StatsDatabase() {};
	virtual ~StatsDatabaseMySql() {};

	void init() override;
	void deinit() override;

	void set_trace(bool state = true) override { dbtrace = state; }

	game_id add_game(std::time_t timet, const str& host
		, const str& port, const str& mapname) override;

	/**
	 *
	 * @param id
	 * @param table "kills" | "deaths"
	 * @param guid
	 * @param weap
	 * @param count
	 * @return
	 */
	bool add_weaps(game_id id, const str& table
		, const GUID& guid, siz weap, siz count) override;

	bool add_caps(game_id id, const GUID& guid, siz count) override;
	bool add_time(game_id id, const GUID& guid, siz count) override;

	bool add_player(std::time_t timet, const GUID& guid, const str& name) override;

	bool add_ovo(game_id id, const GUID& guid1, const GUID& guid2, siz count) override;

	bool add_weapon_usage(game_id id, const GUID& guid, siz weap, siz shots) override;
	bool add_mod_damage(game_id id, const GUID& guid, siz mod
		, siz hits, siz damage, siz hitsRecv, siz damageRecv, float weightedHits) override;
	bool add_playerstats(game_id id, const GUID& guid,
		siz fragsFace, siz fragsBack, siz fraggedInFace, siz fraggedInBack,
		siz spawnKills, siz spawnKillsRecv, siz pushes, siz pushesRecv,
		siz healthPickedUp, siz armorPickedUp, siz holyShitFrags, siz holyShitFragged,
		siz carrierFrags, siz carrierFragsRecv) override;
	bool add_playerstats_ps(game_id id, const GUID& guid,
		siz fragsFace, siz fragsBack, siz fraggedInFace, siz fraggedInBack,
		siz spawnKills, siz spawnKillsRecv, siz pushes, siz pushesRecv,
		siz healthPickedUp, siz armorPickedUp, siz holyShitFrags, siz holyShitFragged,
		siz carrierFrags, siz carrierFragsRecv) override;
	bool add_playerstats_ps(game_id id, const GUID& guid, const struct stat& s) override;
	bool add_speed(game_id id, const GUID& guid,
			siz dist, siz time, bool has_flag) override;

	bool read_map_votes(const str& mapname, guid_int_map& map_votes) override;

	bool set_preferred_name(const GUID& guid, const str& name) override;
	bool get_preferred_name(const GUID& guid, str& name) override;

	siz get_kills_per_cap(const str& sql_select_games = "") override;
	bool get_ingame_boss(const str& mapname, const client_arr& clients
		, GUID& guid, str& stats) override;
	bool get_ingame_champ(const str& mapname, GUID& guid, str& stats) override;
	bool get_ingame_stats(const GUID& guid, const str& mapname
		, siz prev, str& stats, siz& skill) override;
	bool get_ingame_stats_c(const str& mapname, const client_arr& clients
		, const GUID& guid, siz prev, str& stats, siz& skill) override;
};

class KatinaPluginStats
: public KatinaPlugin
{
public:

	onevone_map onevone; // GUID -> GUID -> <count> //
	guid_stat_map stats; // GUID -> <stat>

private:
	const str& mapname;
	const client_arr& clients; // slot -> GUID
	const guid_str_map& players; // GUID -> name
//	const guid_siz_map& teams; // GUID -> 'R' | 'B'

	RCon& server;
	StatsDatabaseMySql db;

	str host;
	str port;

	// cvars
	bool active;
//	bool write;
    bool recordBotGames;

	bool in_game;

	bool allow_bots = false;
	bool stop_stats; // are any bots playing?

	// Current flag carriers (slot number, slot::bad if nobody carries the flag)
	slot carrierBlue;
	slot carrierRed;

	siz_set db_weaps; // which weapons to record

	siz announce_time = 0; // seconds before announce

	void stall_client(slot num);
	void unstall_client(slot num);
	void stall_clients();
	void unstall_clients();
	void check_bots_and_players();
	bool check_slot(slot num);

public:
	KatinaPluginStats(Katina& katina);

	// TODO: ass this as an api() call
	void updatePlayerTime(slot num);

    ///////////////////////////////////////////
	// API

	siz get_skill(const GUID& guid, const str& mapname);
	const guid_stat_map* get_stats() { return &stats; }

    ///////////////////////////////////////////
	// INTERFACE: KatinaPlugin

	bool open() override;

	str api(const str& cmd, void* blob = nullptr) override;

	str get_id() const override;
	str get_name() const override;
	str get_version() const override;

	bool exit(siz min, siz sec) override;
	bool shutdown_game(siz min, siz sec) override;
	bool warmup(siz min, siz sec) override;
	bool client_userinfo_changed(siz min, siz sec, slot num, siz team, const GUID& guid, const str& name, siz hc) override;
//	bool client_connect(siz min, siz sec, slot num) override;
	bool client_disconnect(siz min, siz sec, slot num) override;
	bool kill(siz min, siz sec, slot num1, slot num2, siz weap) override;
	bool ctf(siz min, siz sec, slot num, siz team, siz act) override;
	bool award(siz min, siz sec, slot num, siz awd) override;
	bool init_game(siz min, siz sec, const str_map& cvars) override;
	bool speed(siz min, siz sec, slot num, siz dist, siz time, bool has_flag) override; // zim@openmafia >= 0.1-beta
	bool weapon_usage(siz min, siz sec, slot num, siz weapon, siz shots) override;
	bool mod_damage(siz min, siz sec, slot num, siz mod, siz hits, siz damage, siz hitsRecv, siz damageRecv, float weightedHits) override;
	bool player_stats(siz min, siz sec, slot num,
		siz fragsFace, siz fragsBack, siz fraggedInFace, siz fraggedInBack,
		siz spawnKills, siz spawnKillsRecv, siz pushes, siz pushesRecv,
		siz healthPickedUp, siz armorPickedUp, siz holyShitFrags, siz holyShitFragged) override;
	bool say(siz min, siz sec, slot num, const str& text) override;
	bool sayteam(siz min, siz sec, slot num, const str& text) override;

	void heartbeat(siz min, siz sec) override;
	siz get_regularity(siz time_in_secs) const override { return 1; } // once per second

	void close();
};

}} // katina::plugin

#endif // _OASTATS_KATINA_PLUGIN_STATS_H

