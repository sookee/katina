#pragma once
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

#include <katina/Katina.h>
#include <katina/KatinaPlugin.h>

#include <katina/Database.h>
#include <katina/GUID.h>

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

typedef std::map<siz, mod_damage_stats> moddmg_map;
typedef moddmg_map::const_iterator moddmg_map_citer;


struct stats
{
	siz_map kills;
	siz_map deaths;
	siz_map flags;
	siz_map awards;

	siz_map   	weapon_usage; // shots fired
	moddmg_map	mod_damage;   // MOD -> mod_damage_stats

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

	stats() :
		kills(), deaths(), flags(), awards(), weapon_usage(), mod_damage(),
		fragsFace(0), fragsBack(0), fraggedInFace(0), fraggedInBack(0),
		spawnKills(0), spawnKillsRecv(0), pushes(0), pushesRecv(0),
		healthPickedUp(0), armorPickedUp(0), holyShitFrags(0), holyShitFragged(0),
		carrierFrags(0), carrierFragsRecv(0), time(0), dist(0), time_f(0), dist_f(0),
		joined_time(0), logged_time(0)
	{}
};

typedef std::map<GUID, guid_siz_map> onevone_map;
typedef std::pair<const GUID, guid_siz_map> onevone_pair;
typedef std::map<GUID, guid_siz_map>::iterator onevone_iter;
typedef std::map<GUID, guid_siz_map>::const_iterator onevone_citer;

typedef std::map<GUID, stats> guid_stat_map;
typedef guid_stat_map::value_type guid_stat_vt;
typedef std::map<GUID, stats>::iterator guid_stat_map_iter;
typedef std::map<GUID, stats>::const_iterator guid_stat_map_citer;

//class Database
//: public katina::data::Database
//{
//};

class KatinaPluginStats
: public KatinaPlugin
{
public:

	onevone_map onevone; // GUID -> GUID -> <count> //
	guid_stat_map stats; // GUID -> <stat>
	//guid_str_map names; // keep track of all players involed in the game

private:
	const str& mapname;
	const slot_guid_map& clients; // slot -> GUID
	const guid_str_map& players; // GUID -> name
	const guid_siz_map& teams; // GUID -> 'R' | 'B'
	RCon& server;

	Database db;

	str host;
	str port;

	// cvars
	bool active;
	bool write;
    bool recordBotGames;
    bool do_prev_stats;

	bool in_game;
	bool have_bots; // are any bots playing?
	siz human_players_r; // number of human players on red team
	siz human_players_b; // number of human players on blue team

	// Current flag carriers (slot number, bad_slot if nobody carries the flag)
	slot carrierBlue;
	slot carrierRed;

	siz_set db_weaps; // which weapons to record

	void stall_client(const GUID& guid);
	void unstall_client(const GUID& guid);
	void stall_clients();
	void unstall_clients();
	void check_bots_and_players();
	bool check_slot(slot num);

public:
	KatinaPluginStats(Katina& katina);

	void updatePlayerTime(slot num);
    

    ///////////////////////////////////////////
	// API

	siz get_skill(const GUID& guid, const str& mapname);

    ///////////////////////////////////////////
	// INTERFACE: KatinaPlugin

	virtual bool open() override;

	virtual str api(const str& cmd) override;

	virtual str get_id() const override;
	virtual str get_name() const override;
	virtual str get_version() const override;

	virtual bool exit(siz min, siz sec) override;
	virtual bool shutdown_game(siz min, siz sec) override;
	virtual bool warmup(siz min, siz sec) override;
	virtual bool client_userinfo_changed(siz min, siz sec, slot num, siz team, const GUID& guid, const str& name, siz hc) override;
//	virtual bool client_connect(siz min, siz sec, slot num) override;
	virtual bool client_disconnect(siz min, siz sec, slot num) override;
	virtual bool kill(siz min, siz sec, slot num1, slot num2, siz weap) override;
	virtual bool ctf(siz min, siz sec, slot num, siz team, siz act) override;
	virtual bool award(siz min, siz sec, slot num, siz awd) override;
	virtual bool init_game(siz min, siz sec, const str_map& cvars) override;
//	virtual bool say(siz min, siz sec, const GUID& guid, const str& text) override;
//	virtual bool unknown(siz min, siz sec, const str& cmd, const str& params) override;
	virtual bool speed(siz min, siz sec, slot num, siz dist, siz time, bool has_flag) override; // zim@openmafia >= 0.1-beta
	virtual bool weapon_usage(siz min, siz sec, slot num, siz weapon, siz shots) override;
	virtual bool mod_damage(siz min, siz sec, slot num, siz mod, siz hits, siz damage, siz hitsRecv, siz damageRecv, float weightedHits) override;
	virtual bool player_stats(siz min, siz sec, slot num,
		siz fragsFace, siz fragsBack, siz fraggedInFace, siz fraggedInBack,
		siz spawnKills, siz spawnKillsRecv, siz pushes, siz pushesRecv,
		siz healthPickedUp, siz armorPickedUp, siz holyShitFrags, siz holyShitFragged) override;
	virtual bool say(siz min, siz sec, const GUID& guid, const str& text) override;
	virtual bool sayteam(siz min, siz sec, const GUID& guid, const str& text) override;

	virtual void close();
};

}} // katina::plugin

#endif // _OASTATS_KATINA_PLUGIN_STATS_H

