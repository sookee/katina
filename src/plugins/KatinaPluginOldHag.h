#pragma once
#ifndef _OASTATS_KATINA_PLUGIN_OLD_HAG_H
#define	_OASTATS_KATINA_PLUGIN_OLD_HAG_H
/*
 * File:   KatinaPluginOldHag.h
 * Author: SooKee oasookee@gmail.com
 *
 * Created on April 22, 2014, 01:51 AM
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

	virtual bool open() override;

	virtual str get_id() const override;
	virtual str get_name() const override;
	virtual str get_version() const override;

	//virtual void cvar_event(const str& name, const str& value);
	
	virtual bool init_game(siz min, siz sec, const str_map& cvars) override;
	virtual bool warmup(siz min, siz sec) override;
	virtual bool client_connect(siz min, siz sec, slot num) override;
	virtual bool client_begin(siz min, siz sec, slot num) override;
	virtual bool client_disconnect(siz min, siz sec, slot num) override;
	virtual bool client_userinfo_changed(siz min, siz sec, slot num, siz team, const GUID& guid, const str& name, siz hc) override;
	virtual bool kill(siz min, siz sec, slot num1, slot num2, siz weap override);
	virtual bool award(siz min, siz sec, slot num, siz awd override);
	virtual bool ctf(siz min, siz sec, slot num, siz team, siz act) override;
	virtual bool ctf_exit(siz min, siz sec, siz r, siz b) override;
	virtual bool score_exit(siz min, siz sec, int score, siz ping, slot num, const str& name) override;
	virtual bool say(siz min, siz sec, const GUID& guid, const str& text) override;
	virtual bool shutdown_game(siz min, siz sec) override;
	virtual bool exit(siz min, siz sec) override;
	virtual bool unknown(siz min, siz sec, const str& cmd, const str& params) override;

	virtual void close() override;
};

}} // katina::plugin

#endif // _OASTATS_KATINA_PLUGIN_OLD_HAG_H

