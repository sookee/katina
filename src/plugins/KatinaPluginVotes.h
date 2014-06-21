/*
 * File:   KatinaPluginVotes.h
 * Author: SooKee oasookee@gmail.com
 *
 * Created on May 13, 2013, 10:02 AM
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
#ifndef KATINA_PLUGIN_VOTES_H
#define	KATINA_PLUGIN_VOTES_H

#include <map>
#include <utility>

#include "KatinaPluginStats.h"
#include <katina/Database.h>
#include <katina/RemoteClient.h>
#include <katina/GUID.h>

#include <katina/types.h>
#include <katina/log.h>

#include <katina/PKI.h>

namespace katina { namespace plugin {

using namespace katina::pki;
using namespace katina::log;
using namespace katina::data;
using namespace katina::types;

class VotesDatabase
: public Database
{
	bool trace = false;

public:
	VotesDatabase(): Database() {}
	virtual ~VotesDatabase() {}

//	virtual void init() override;
//	virtual void deinit() override;

	void set_trace(bool state = true) { trace = state; }

	/**
	 *
	 * @param type
	 * @param item
	 * @param guid
	 * @param count
	 * @return 0 = error, 1 = inserted, 2 = updated
	 */
	row_count add_vote(const str& type, const str& item, const GUID& guid, int count);

	bool read_map_votes(const str& mapname, guid_int_map& map_votes);

};

class KatinaPluginVotes
: public KatinaPlugin
{
public:

private:
	VotesDatabase db;

	// cvars
	bool active;

	bool votes_valid = false;
	siz announce_time = 0; // seconds before announce

public:

	/**
	 * The name of the map that map_votes applies to
	 */
	str mapname;
	guid_int_map map_votes; // GUID -> 3

	KatinaPluginVotes(Katina& katina);

	// API
	
	void get_votes(siz& love, siz& hate, siz& soso);

	// INTERFACE: KatinaPlugin

	virtual str api(const str& call, void* blob = nullptr) override;

	virtual bool open() override;

	virtual str get_id() const override;
	virtual str get_name() const override;
	virtual str get_version() const override;

	virtual bool init_game(siz min, siz sec, const str_map& cvars) override;
	virtual bool warmup(siz min, siz sec) override;
	virtual bool say(siz min, siz sec, const GUID& guid, const str& text) override;
	virtual bool sayteam(siz min, siz sec, const GUID& guid, const str& text) override;

	virtual void heartbeat(siz min, siz sec) override;
	virtual siz get_regularity(siz time_in_secs) const override { return 1; } // once per second

	virtual void close() override;
};

}} // katina::plugin

#endif	// KATINA_PLUGIN_VOTES_H

