#pragma once
#ifndef _OASTATS_KATINA_PLUGIN_EXAMPLE_H
#define	_OASTATS_KATINA_PLUGIN_EXAMPLE_H
/*
 * File:   KatinaPluginExample.h
 * Author: SooKee oasookee@gmail.com
 *
 * Created on April 27, 2013, 10:02 AM
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
#include <map>
#include <utility>

#include <katina/Katina.h>
#include <katina/KatinaPlugin.h>

#include <katina/Database.h>
#include <katina/GUID.h>

#include <katina/types.h>
#include <katina/log.h>

#include <pthread.h>

namespace katina { namespace plugin {

using namespace katina;
using namespace katina::log;
using namespace katina::data;
using namespace katina::types;

class KatinaPluginExample
: public KatinaPlugin
{
private:
	const str& mapname;
	const slot_guid_map& clients; // slot -> GUID
	const guid_str_map& players; // GUID -> name
	const guid_siz_map& teams; // GUID -> 'R' | 'B'
	
	bool active;

public:
	KatinaPluginExample(Katina& katina);

	// INTERFACE: KatinaPlugin

	virtual bool open() override;

	virtual str get_id() const override;
	virtual str get_name() const override;
	virtual str get_version() const override;

	//virtual void cvar_event(const str& name, const str& value);
	
	virtual bool init_game(siz min, siz sec, const str_map& cvars) override;
	virtual bool warmup(siz min, siz sec) override;
	virtual bool client_connect(siz min, siz sec, slot num) override;
	virtual bool client_connect_info(siz min, siz sec, slot num, const GUID& guid, const str& ip) override;
	virtual bool client_begin(siz min, siz sec, slot num) override;
	virtual bool client_disconnect(siz min, siz sec, slot num) override;
	virtual bool client_userinfo_changed(siz min, siz sec, slot num, siz team, const GUID& guid, const str& name, siz hc) override;
	virtual bool client_switch_team(siz min, siz sec, slot num, siz teamBefore, siz teamNow) override;
	virtual bool kill(siz min, siz sec, slot num1, slot num2, siz weap) override;
	virtual bool award(siz min, siz sec, slot num, siz awd) override;
	virtual bool ctf(siz min, siz sec, slot num, siz team, siz act) override;
	virtual bool ctf_exit(siz min, siz sec, siz r, siz b) override;
	virtual bool score_exit(siz min, siz sec, int score, siz ping, slot num, const str& name) override;
	virtual bool say(siz min, siz sec, slot num, const str& text) override;
	virtual bool shutdown_game(siz min, siz sec) override;
	virtual bool exit(siz min, siz sec) override;
	virtual bool unknown(siz min, siz sec, const str& cmd, const str& params) override;

	virtual bool callvote(siz min, siz sec, slot num, const str& type, const str& info) override;

	virtual bool speed(siz min, siz sec, slot num, siz dist, siz time, bool has_flag) override;

	/**
	 * Summarizing events for more detailed statistics (they only work with the katina game mod)
	 */
	virtual bool weapon_usage(siz min, siz sec, slot num, siz weapon, siz shots) override;
	virtual bool mod_damage(siz min, siz sec, slot num, siz mod, siz hits, siz damage, siz hitsRecv, siz damageRecv, float weightedHits) override;
	virtual bool player_stats(siz min, siz sec, slot num,
		siz fragsFace, siz fragsBack, siz fraggedInFace, siz fraggedInBack,
		siz spawnKills, siz spawnKillsRecv, siz pushes, siz pushesRecv,
		siz healthPickedUp, siz armorPickedUp, siz holyShitFrags, siz holyShitFragged) override;

	virtual void heartbeat(siz min, siz sec) override;
	virtual siz get_regularity(siz time_in_secs) const override;

	virtual void close() override;
};

}} // katina::plugin

#endif // _OASTATS_KATINA_PLUGIN_EXAMPLE_H

