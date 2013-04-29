/*
 * File:   KatinaPluginStats.h
 * Author: SooKee oasookee@gmail.com
 *
 * Created on April 27, 2013, 10:02 AM
 */

#ifndef KATINAPLUGINSTATS_H
#define	KATINAPLUGINSTATS_H

#include <map>
#include <utility>

#include "Database.h"
#include "GUID.h"

#include "types.h"
#include "log.h"

namespace katina { namespace plugin {

using namespace oastats::log;
using namespace oastats::data;
using namespace oastats::types;

class KatinaPluginStats
: public KatinaPlugin
{
public:
	typedef std::map<GUID, guid_siz_map> onevone_map;
	typedef std::pair<const GUID, guid_siz_map> onevone_pair;
	typedef std::map<GUID, guid_siz_map>::iterator onevone_iter;
	typedef std::map<GUID, guid_siz_map>::const_iterator onevone_citer;

	typedef std::map<GUID, stats> guid_stat_map;
	typedef std::pair<const GUID, stats> guid_stat_pair;
	typedef std::map<GUID, stats>::iterator guid_stat_iter;
	typedef std::map<GUID, stats>::const_iterator guid_stat_citer;


private:
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

	Database db;

	str host;
	str port;

	onevone_map onevone; // GUID -> GUID -> <count> //
	guid_stat_map stats; // GUID -> <stat>

	bool in_game;

public:
	KatinaPluginStats(): in_game(false) {}

	// API
	
	const guid_stat_map& get_stats_ref() const { return stats; }

	// INTERFACE: KatinaPlugin

	virtual bool open(str_map& config);

	virtual str get_id() const;
	virtual str get_name() const;
	virtual str get_version() const;

	virtual bool exit(GameInfo& gi);
	virtual bool shutdown_game(GameInfo& gi);
	virtual bool warmup(GameInfo& gi);
	virtual bool client_userinfo_changed(GameInfo& gi, siz num, siz team, const GUID& guid, const str& name);
	virtual bool client_connect(GameInfo& gi, siz num);
	virtual bool client_disconnect(GameInfo& gi, siz num);
	virtual bool kill(GameInfo& gi, siz num1, siz num2, siz weap);
	virtual bool ctf(GameInfo& gi, siz num, siz team, siz act);
	virtual bool award(GameInfo& gi);
	virtual bool init_game(GameInfo& gi);
	virtual bool say(GameInfo& gi, const str& text);
	virtual bool unknown(GameInfo& gi);

	virtual void close();
};

}} // katina::plugin

#endif	/* KATINAPLUGINSTATS_H */

