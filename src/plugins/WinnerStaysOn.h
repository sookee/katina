#pragma once
#ifndef _OASTATS_KATINA_PLUGIN_WINNER_STAYS_ON_H
#define	_OASTATS_KATINA_PLUGIN_WINNER_STAYS_ON_H
/*
 * File:   WinnerStaysOn.h
 * Author: SooKee oasookee@gmail.com
 *
 * Created on May 06, 2013
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
	const slot_guid_map& clients; // slot -> GUID
	const guid_str_map& players; // GUID -> name
	const guid_siz_map& teams; // GUID -> 'R' | 'B'
	RCon& server;
	
	typedef std::deque<slot> slot_deq;
	typedef slot_deq::iterator slot_deq_iter;
	typedef slot_deq::const_iterator slot_deq_citer;
	typedef slot_deq::reverse_iterator slot_deq_riter;
	typedef slot_deq::const_reverse_iterator slot_deq_criter;
	
	//KatinaPluginStats* stats;
	
	siz win_team;
	siz opp_team;
	
	slot_deq q; // pos 0 = winner, 1 = opponent
	
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
	, clients(katina.getClients())
	, players(katina.getPlayers())
	, teams(katina.getTeams())
	, server(katina.server)
	, win_team(1)
	, opp_team(2)
	{
//		caps[0] = 0;
//		caps[1] = 0;
	}

	// INTERFACE: KatinaPlugin

	virtual bool open() override;

	virtual str get_id() const override;
	virtual str get_name() const override;
	virtual str get_version() const override;

	virtual bool init_game(siz min, siz sec, const str_map& svars) override;
	virtual bool warmup(siz min, siz sec) override;
	virtual bool client_connect(siz min, siz sec, slot num) override;

	virtual bool client_disconnect(siz min, siz sec, slot num) override;
	virtual bool client_userinfo_changed(siz min, siz sec, slot num, siz team, const GUID& guid, const str& name, siz hc) override;
	virtual bool ctf(siz min, siz sec, slot num, siz team, siz act) override;
	virtual bool ctf_exit(siz min, siz sec, siz r, siz b) override;
	virtual bool say(siz min, siz sec, const GUID& guid, const str& text) override;
	virtual bool shutdown_game(siz min, siz sec) override;
	virtual bool exit(siz min, siz sec) override;

	virtual void close() override;
};

}} // katina::plugin

#endif // _OASTATS_KATINA_PLUGIN_WINNER_STAYS_ON_H

