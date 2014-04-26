#pragma once
#ifndef _OASTATS_KATINA_PLUGIN_OLD_HAG_H
#define	_OASTATS_KATINA_PLUGIN_OLD_HAG_H
/*
 * File:   KatinaPluginOldHag.h
 * Author: SooKee oasookee@gmail.com
 *
 * Created on April 22, 2014, 01:51 AM
 */

#include <map>
#include <utility>

#include <katina/Katina.h>
#include <katina/KatinaPlugin.h>

#include <katina/GUID.h>

#include <katina/types.h>
#include <katina/log.h>

#include <pthread.h>

namespace katina { namespace plugin {

using namespace oastats;
using namespace oastats::log;
using namespace oastats::data;
using namespace oastats::types;

class KatinaPluginOldHag
: public KatinaPlugin
{
private:
	RCon& server;
	str& mapname;
	siz_guid_map& clients; // slot -> GUID
	guid_str_map& players; // GUID -> name
	guid_siz_map& teams; // GUID -> 'R' | 'B'
	
	bool active;

	siz hag_players = 2;
	siz hag_bots = 1;
	std::vector<siz> hag_player;

public:
	KatinaPluginOldHag(Katina& katina);

	// INTERFACE: KatinaPlugin

	virtual bool open();

	virtual str get_id() const;
	virtual str get_name() const;
	virtual str get_version() const;

	//virtual void cvar_event(const str& name, const str& value);
	
	virtual bool init_game(siz min, siz sec, const str_map& cvars);
	virtual bool warmup(siz min, siz sec);
	virtual bool client_connect(siz min, siz sec, siz num);
	virtual bool client_begin(siz min, siz sec, siz num);
	virtual bool client_disconnect(siz min, siz sec, siz num);
	virtual bool client_userinfo_changed(siz min, siz sec, siz num, siz team, const GUID& guid, const str& name, siz hc);
	virtual bool kill(siz min, siz sec, siz num1, siz num2, siz weap);
	virtual bool award(siz min, siz sec, siz num, siz awd);
	virtual bool ctf(siz min, siz sec, siz num, siz team, siz act);
	virtual bool ctf_exit(siz min, siz sec, siz r, siz b);
	virtual bool score_exit(siz min, siz sec, int score, siz ping, siz num, const str& name);
	virtual bool say(siz min, siz sec, const GUID& guid, const str& text);
	virtual bool shutdown_game(siz min, siz sec);
	virtual bool exit(siz min, siz sec);
	virtual bool unknown(siz min, siz sec, const str& cmd, const str& params);

	virtual void close();
};

}} // katina::plugin

#endif // _OASTATS_KATINA_PLUGIN_OLD_HAG_H

