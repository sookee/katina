#pragma once
#ifndef _OASTATS_KATINA_PLUGIN_NEXT_MAP_H
#define	_OASTATS_KATINA_PLUGIN_NEXT_MAP_H
/*
 * File:   KatinaPluginNextMap.h
 * Author: SooKee oasookee@gmail.com
 *
 * Created on May 27, 2014, 01:12 AM
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

struct vote
{
	str mapname;
	int count;
	vote(): count(0) {}
	vote(const str& mapname, int count): mapname(mapname), count(count) {}
};

typedef std::map<GUID, vote> guid_vote_map;
typedef guid_vote_map::value_type guid_vote_map_pair;
typedef guid_vote_map::iterator guid_vote_map_iter;
typedef guid_vote_map::const_iterator guid_vote_map_citer;

class KatinaPluginNextMap
: public KatinaPlugin
{
private:
//	KatinaPluginVotes* votes;

	const str& mapname;
	const slot_guid_map& clients; // slot -> GUID
	const guid_str_map& players; // GUID -> name
	const guid_siz_map& teams; // GUID -> 'R' | 'B'
	RCon& server;

	Database db;

	guid_vote_map votes;

	str_siz_map played; // when was each map last played?

	str rotmap; // next map command on rotation

	bool active;
	str nextmap;

public:
	KatinaPluginNextMap(Katina& katina);

	// INTERFACE: KatinaPlugin

	virtual bool open() override;

	virtual str get_id() const override;
	virtual str get_name() const override;
	virtual str get_version() const override;

	//virtual void cvar_event(const str& name, const str& value);
	
//	virtual bool init_game(siz min, siz sec, const str_map& cvars) override;
//	virtual bool client_connect_info(siz min, siz sec, slot num, const GUID& guid, const str& ip) override;
//	virtual bool client_disconnect(siz min, siz sec, slot num) override;
	virtual bool say(siz min, siz sec, const GUID& guid, const str& text) override;
	virtual bool exit(siz min, siz sec) override;
	virtual bool shutdown_game(siz min, siz sec) override;

	virtual void close() override;
};

}} // katina::plugin

#endif // _OASTATS_KATINA_PLUGIN_NEXT_MAP_H

