/*
 * File:   KatinaPluginStats.h
 * Author: SooKee oasookee@gmail.com
 *
 * Created on April 27, 2013, 10:02 AM
 */

#ifndef KATINA_PLUGIN_STATS_REPORT_H
#define	KATINA_PLUGIN_STATS_REPORT_H

#include <map>
#include <utility>

#include "KatinaPluginStats.h"
#include "Database.h"
#include "GUID.h"

#include "types.h"
#include "log.h"

namespace katina { namespace plugin {

using namespace oastats::log;
using namespace oastats::data;
using namespace oastats::types;

class KatinaPluginStatsReport
: public KatinaPlugin
{
public:

private:
	KatinaPluginStats* kpsp;
	SkivvyClient skivvy;
	bool in_game;

public:
	KatinaPluginStatsReport(): kpsp(0), in_game(false) {}

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

#endif	// KATINA_PLUGIN_STATS_REPORT_H

