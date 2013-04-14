#pragma once
#ifndef _OASTATS_KATINA_H_
#define _OASTATS_KATINA_H_

/*
 * katina.h
 *
 *  Created on: Apr 14, 2013
 *      Author: oasookee@gmail.com
 */

#include "types.h"
#include "GUID.h"
#include "str.h"
#include "log.h"
#include "rcon.h"
#include "RemoteIRCClient.h"
#include "Database.h"

#include <pthread.h>

namespace oastats {

using namespace oastats;
using namespace oastats::net;
using namespace oastats::types;
using namespace oastats::data;
using namespace oastats::string;
using namespace oastats::log;

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

typedef std::map<GUID, stats> guid_stat_map;
typedef std::pair<const GUID, stats> guid_stat_pair;
typedef std::map<GUID, stats>::iterator guid_stat_iter;
typedef std::map<GUID, stats>::const_iterator guid_stat_citer;

class Katina
{
	friend void* rconthread(void* vp);

	bool done;
	milliseconds thread_delay;

	RCon server;
	RemoteIRCClientAPtr remote;
	Database db;

	//bool katina_active = false;
	server_conf svr_cfg;
	remote_conf rep_cfg;

	str_map recs; // high scores/ config etc

	siz_guid_map clients; // slot -> GUID
	guid_str_map players; // GUID -> name
	onevone_map onevone; // GUID -> GUID -> <count> //
	guid_siz_map caps; // GUID -> <count> // TODO: caps container duplicated in stats::caps
	guid_stat_map stats; // GUID -> <stat>
	guid_siz_map teams; // GUID -> 'R' | 'B'
	str mapname, old_mapname; // current/previous map name

	guid_int_map map_votes; // GUID -> 3

	static const str flag[2];

	void report_clients(const siz_guid_map& clients);

	void report_players(const guid_str_map& players);

	void report_onevone(const onevone_map& onevone, const guid_str_map& players);

	void report_caps(const guid_siz_map& caps, const guid_str_map& players, siz flags[2]);

	siz map_get(const siz_map& m, siz key);

	void report_stats(const guid_stat_map& stats, const guid_str_map& players);

	void save_records(const str_map& recs);

	void load_records(str_map& recs);

	GUID guid_from_name(const str& name);

	bool extract_name_from_text(const str& line, GUID& guid, str& text);

	str expand_env(const str& var);

	/**
	 *
	 * @param m
	 * @param s
	 * @param dasher
	 * @param killtype 0 = none, 1 = red killed, 2 = blue killed
	 * @return
	 */
	str get_hud(siz m, siz s, GUID dasher[2], siz killtype = 0);

	bool is_guid(const str& s);

public:
	Katina(): done(false), thread_delay(6000) {}

	int run(const int argc, const char* argv[]);
};

class Processor
{
public:
	virtual ~Process() {}

	virtual bool process_exit() = 0;
	virtual bool process_shutdown_game() = 0;
	virtual bool process_warmup() = 0;
	virtual bool process_client_user_info_changed() = 0;
	virtual bool process_kill() = 0;
	virtual bool process_ctf() = 0;
	virtual bool process_award() = 0;
	virtual bool process_init_game() = 0;
	virtual bool process_say() = 0;
	virtual bool process_sayteam() = 0;
	virtual bool process_chat() = 0;
	virtual bool process_player_score() = 0;
};

} // oastats

#endif /* _OASTATS_KATINA_H_ */
