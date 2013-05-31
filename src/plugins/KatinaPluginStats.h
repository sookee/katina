#pragma once
#ifndef _OASTATS_KATINA_PLUGIN_STATS_H
#define	_OASTATS_KATINA_PLUGIN_STATS_H
/*
 * File:   KatinaPluginStats.h
 * Author: SooKee oasookee@gmail.com
 *
 * Created on April 27, 2013, 10:02 AM
 */

#include <map>
#include <utility>

#include <katina/Katina.h>
#include <katina/KatinaPlugin.h>

#include <katina/Database.h>
#include <katina/GUID.h>

#include <katina/types.h>
#include <katina/log.h>

namespace katina { namespace plugin {

using namespace oastats;
using namespace oastats::log;
using namespace oastats::data;
using namespace oastats::types;

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

	time_t joined_time;
	siz logged_time;

	str name;

	stats() :
		kills(), deaths(), flags(), awards(), weapon_usage(), mod_damage(),
		fragsFace(0), fragsBack(0), fraggedInFace(0), fraggedInBack(0),
		spawnKills(0), spawnKillsRecv(0), pushes(0), pushesRecv(0),
		healthPickedUp(0), armorPickedUp(0), holyShitFrags(0), holyShitFragged(0),
		carrierFrags(0), carrierFragsRecv(0),
		joined_time(0), logged_time(0)
	{}
};

typedef std::map<GUID, guid_siz_map> onevone_map;
typedef std::pair<const GUID, guid_siz_map> onevone_pair;
typedef std::map<GUID, guid_siz_map>::iterator onevone_iter;
typedef std::map<GUID, guid_siz_map>::const_iterator onevone_citer;

typedef std::map<GUID, stats> guid_stat_map;
typedef std::pair<const GUID, stats> guid_stat_pair;
typedef std::map<GUID, stats>::iterator guid_stat_iter;
typedef std::map<GUID, stats>::const_iterator guid_stat_citer;

//class Database
//: public oastats::data::Database
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
	str& mapname;
	siz_guid_map& clients; // slot -> GUID
	guid_str_map& players; // GUID -> name
	guid_siz_map& teams; // GUID -> 'R' | 'B'
	RCon& server;

	Database db;

	str host;
	str port;

	// cvars
	bool active;
	bool write;

	bool in_game;
	bool have_bots; // are any bots playing?
	siz human_players_r; // number of human players on red team
	siz human_players_b; // number of human players on blue team

	// Current flag carriers (slot number, -1 if nobody carries the flag)
	int carrierBlue;
	int carrierRed;

	siz_set db_weaps; // which weapons to record

	void stall_client(siz num);
	void unstall_client(siz num);
	void stall_clients();
	void unstall_clients(siz num = siz(-1));
	void check_bots_and_players(std::time_t now, siz num = siz(-1));

public:
	KatinaPluginStats(Katina& katina);

	// API

	//const guid_stat_map& get_stats_ref() const { return stats; }

	// INTERFACE: KatinaPlugin

	virtual bool open();

	virtual str get_id() const;
	virtual str get_name() const;
	virtual str get_version() const;

	virtual bool exit(siz min, siz sec);
	virtual bool shutdown_game(siz min, siz sec);
	virtual bool warmup(siz min, siz sec);
	virtual bool client_userinfo_changed(siz min, siz sec, siz num, siz team, const GUID& guid, const str& name);
//	virtual bool client_connect(siz min, siz sec, siz num);
	virtual bool client_disconnect(siz min, siz sec, siz num);
	virtual bool kill(siz min, siz sec, siz num1, siz num2, siz weap);
	virtual bool ctf(siz min, siz sec, siz num, siz team, siz act);
	virtual bool award(siz min, siz sec, siz num, siz awd);
	virtual bool init_game(siz min, siz sec, const str_map& cvars);
//	virtual bool say(siz min, siz sec, const GUID& guid, const str& text);
//	virtual bool unknown(siz min, siz sec, const str& cmd, const str& params);
	virtual bool weapon_usage(siz min, siz sec, siz num, siz weapon, siz shots);
	virtual bool mod_damage(siz min, siz sec, siz num, siz mod, siz hits, siz damage, siz hitsRecv, siz damageRecv, float weightedHits);
	virtual bool player_stats(siz min, siz sec, siz num,
		siz fragsFace, siz fragsBack, siz fraggedInFace, siz fraggedInBack,
		siz spawnKills, siz spawnKillsRecv, siz pushes, siz pushesRecv,
		siz healthPickedUp, siz armorPickedUp, siz holyShitFrags, siz holyShitFragged);
	virtual bool say(siz min, siz sec, const GUID& guid, const str& text);
	virtual bool sayteam(siz min, siz sec, const GUID& guid, const str& text);

	virtual void close();
};

}} // katina::plugin

#endif // _OASTATS_KATINA_PLUGIN_STATS_H

