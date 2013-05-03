/*
 * File:   KatinaPluginReports.h
 * Author: SooKee oasookee@gmail.com
 *
 * Created on April 27, 2013, 10:02 AM
 */

#ifndef KATINA_PLUGIN_REPORTS_H
#define	KATINA_PLUGIN_REPORTS_H

#include <map>
#include <utility>

#include "KatinaPluginStats.h"
#include <katina/Database.h>
#include <katina/RemoteClient.h>
#include <katina/GUID.h>

#include <katina/types.h>
#include <katina/log.h>

namespace katina { namespace plugin {

using namespace oastats::log;
using namespace oastats::data;
using namespace oastats::types;

class KatinaPluginReports
: public KatinaPlugin
{
public:

private:
	KatinaPluginStats* stats;
	RemoteClient* remote;

public:
	KatinaPluginReports(Katina& katina)
	: KatinaPlugin(katina)
	, stats(0)
	, remote(0)
	{
	}

	// INTERFACE: KatinaPlugin

	virtual bool open();

	virtual str get_id() const;
	virtual str get_name() const;
	virtual str get_version() const;

	virtual bool init_game();
	virtual bool warmup();
	virtual bool client_connect(siz num);
	virtual bool client_disconnect(siz num);
	virtual bool client_userinfo_changed(siz num, siz team, const GUID& guid, const str& name);
	virtual bool kill(siz num1, siz num2, siz weap);
	virtual bool ctf(siz num, siz team, siz act);
	virtual bool award(siz num, siz awd);
	virtual bool say(const GUID& guid, const str& text);
	virtual bool unknown(const str& line);
	virtual bool shutdown_game();
	virtual bool exit();

	virtual void close();
};

}} // katina::plugin

#endif	// KATINA_PLUGIN_REPORTS_H

