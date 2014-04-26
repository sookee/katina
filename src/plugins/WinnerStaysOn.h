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
	RCon& server;
	
	typedef siz slot;
	typedef std::deque<siz> siz_deq;
	typedef siz_deq::iterator siz_deq_iter;
	typedef siz_deq::const_iterator siz_deq_citer;
	typedef siz_deq::reverse_iterator siz_deq_riter;
	typedef siz_deq::const_reverse_iterator siz_deq_criter;
	
	//KatinaPluginStats* stats;
	
	siz win_team;
	siz opp_team;
	
	siz_deq q; // pos 0 = winner, 1 = opponent
	
	void ensure_teams();
	void dump_queue();
	void announce_queue();
	
	bool command(const str& cmd);
	bool vote_enable();
	bool vote_disable();
	bool lock_teams();
	bool unlock_teams();

public:
	WinnerStaysOn(Katina& katina)
	: KatinaPlugin(katina)
	, mapname(katina.mapname)
	, clients(katina.clients)
	, players(katina.players)
	, teams(katina.teams)
	, server(katina.server)
	, win_team(1)
	, opp_team(2)
	{
//		caps[0] = 0;
//		caps[1] = 0;
	}

	// INTERFACE: KatinaPlugin

	virtual bool open();

	virtual str get_id() const;
	virtual str get_name() const;
	virtual str get_version() const;

	virtual bool exit(siz min, siz sec);
	virtual bool shutdown_game(siz min, siz sec);
	virtual bool warmup(siz min, siz sec);
//	virtual bool client_userinfo_changed(siz min, siz sec, siz num, siz team, const GUID& guid, const str& name, siz hc);
	virtual bool client_connect(siz min, siz sec, siz num);
	virtual bool client_disconnect(siz min, siz sec, siz num);
	virtual bool client_userinfo_changed(siz min, siz sec, siz num, siz team, const GUID& guid, const str& name, siz hc);
	virtual bool ctf(siz min, siz sec, siz num, siz team, siz act);
	virtual bool ctf_exit(siz min, siz sec, siz r, siz b);
	virtual bool init_game(siz min, siz sec);
	virtual bool say(siz min, siz sec, const GUID& guid, const str& text);

	virtual void close();
};

}} // katina::plugin

#endif // _OASTATS_KATINA_PLUGIN_WINNER_STAYS_ON_H

