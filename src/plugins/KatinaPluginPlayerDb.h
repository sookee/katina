#pragma once
#ifndef _OASTATS_KATINA_PLUGIN_PLAYER_DB_H
#define	_OASTATS_KATINA_PLUGIN_PLAYER_DB_H
/*
 * File:   KatinaPluginPlayerDb.h
 * Author: SooKee oasookee@gmail.com
 *
 * Created on April 27, 2013, 10:02 AM
 *
 * This plugin stored player info such as GUID, IP address and name.
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

using namespace oastats;
using namespace oastats::log;
using namespace oastats::data;
using namespace oastats::types;

class KatinaPluginPlayerDb
: public KatinaPlugin
{
private:
	RCon& server;

	str& mapname;
	const siz_guid_map& clients; // slot -> GUID
	const guid_str_map& players; // GUID -> name
	const guid_siz_map& teams; // GUID -> 'R' | 'B'
	
	bool active;

	void add_player(siz num);
	void sub_player(siz num);

public:
	KatinaPluginPlayerDb(Katina& katina);

	// INTERFACE: KatinaPlugin

	virtual bool open();

	virtual str get_id() const;
	virtual str get_name() const;
	virtual str get_version() const;

	//virtual void cvar_event(const str& name, const str& value);
	
	virtual bool client_connect_info(siz min, siz sec, siz num, const GUID& guid, const str& ip);
	virtual bool client_disconnect(siz min, siz sec, siz num);
	virtual bool client_userinfo_changed(siz min, siz sec, siz num, siz team, const GUID& guid, const str& name, siz hc);

	virtual void close();
};

}} // katina::plugin

#endif // _OASTATS_KATINA_PLUGIN_PLAYER_DB_H

