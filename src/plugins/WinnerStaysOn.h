#pragma once
#ifndef _OASTATS_KATINA_PLUGIN_WINNER_STAYS_ON_H
#define	_OASTATS_KATINA_PLUGIN_WINNER_STAYS_ON_H
/*
 * File:   WinnerStaysOn.h
 * Author: SooKee oasookee@gmail.com
 *
 * Created on May 06, 2013
 */

#include <utility>
#include <deque>

#include <katina/Katina.h>
#include <katina/KatinaPlugin.h>

#include <katina/GUID.h>

#include <katina/types.h>
#include <katina/log.h>

namespace katina { namespace plugin {

using namespace oastats;
using namespace oastats::log;
using namespace oastats::types;

class WinnerStaysOn
: public KatinaPlugin
{
private:
	str& mapname;
	siz_guid_map& clients; // slot -> GUID
	guid_str_map& players; // GUID -> name
	guid_siz_map& teams; // GUID -> 'R' | 'B'
	
	typedef siz slot;
	typedef std::deque<slot> slot_deq;
	typedef slot_deq::iteratr slot_deq_iter;
	typedef slot_deq::const_iteratr slot_deq_citer;
	
	KatinaPluginStats* stats;
	
	slot winner;
	slot_deq q;

public:
	WinnerStaysOn(Katina& katina)
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

	virtual bool exit(siz min, siz sec);
	virtual bool shutdown_game(siz min, siz sec);
	virtual bool warmup(siz min, siz sec);
	virtual bool client_userinfo_changed(siz min, siz sec, siz num, siz team, const GUID& guid, const str& name);
	virtual bool client_connect(siz min, siz sec, siz num);
	virtual bool client_disconnect(siz min, siz sec, siz num);
	virtual bool kill(siz min, siz sec, siz num1, siz num2, siz weap);
	virtual bool ctf(siz min, siz sec, siz num, siz team, siz act);
	virtual bool award(siz min, siz sec, siz num, siz awd);
	virtual bool init_game(siz min, siz sec);
	virtual bool say(siz min, siz sec, const GUID& guid, const str& text);
	virtual bool unknown(siz min, siz sec, const str& cmd, const str& params);

	virtual void close();
};

}} // katina::plugin

#endif // _OASTATS_KATINA_PLUGIN_WINNER_STAYS_ON_H

