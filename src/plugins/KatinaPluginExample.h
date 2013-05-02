#pragma once
#ifndef _OASTATS_KATINA_PLUGIN_EXAMPLE_H
#define	_OASTATS_KATINA_PLUGIN_EXAMPLE_H
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

class KatinaPluginExample
: public KatinaPlugin
{
private:
	str& mapname;
	siz_guid_map& clients; // slot -> GUID
	guid_str_map& players; // GUID -> name
	guid_siz_map& teams; // GUID -> 'R' | 'B'

public:
	KatinaPluginExample(Katina& katina)
	: KatinaPlugin(katina)
	, mapname(katina.mapname)
	, clients(katina.clients)
	, players(katina.players)
	, teams(katina.teams)
	{
	}

	// INTERFACE: KatinaPlugin

	virtual bool open();

	virtual str get_id() const;
	virtual str get_name() const;
	virtual str get_version() const;

	virtual bool exit();
	virtual bool shutdown_game();
	virtual bool warmup();
	virtual bool client_userinfo_changed(siz num, siz team, const GUID& guid, const str& name);
	virtual bool client_connect(siz num);
	virtual bool client_disconnect(siz num);
	virtual bool kill(siz num1, siz num2, siz weap);
	virtual bool ctf(siz num, siz team, siz act);
	virtual bool award(siz num, siz awd);
	virtual bool init_game();
	virtual bool say(const GUID& guid, const str& text);
	virtual bool unknown(const str& line);

	virtual void close();
};

}} // katina::plugin

#endif // _OASTATS_KATINA_PLUGIN_EXAMPLE_H

