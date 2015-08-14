/*
 * File:   KatinaPluginReports.h
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
#ifndef KATINA_PLUGIN_REPORTS_H
#define	KATINA_PLUGIN_REPORTS_H

#include <map>
#include <utility>

#include "KatinaPluginStats.h"
#include "KatinaPluginVotes.h"

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

class RemoteClientList
{
	std::vector<RemoteClient*> clients;

public:
	RemoteClientList(Katina& katina);
	~RemoteClientList();

	void on() { for(siz i = 0; i < clients.size(); ++i) clients[i]->on(); }
	void off() { for(siz i = 0; i < clients.size(); ++i) clients[i]->off(); }

	void add(RemoteClient* client) { if(client) { clients.push_back(client); } }
	void clear()
	{
		for(siz i = 0; i < clients.size(); ++i)
		{
			clients[i]->off();
			delete clients[i];
		}
		clients.clear();
	}

	bool chat(char f, const str& text) { for(siz i = 0; i < clients.size(); ++i) clients[i]->chat(f, text); return true; }
	bool raw_chat(char f, const str& text) { for(siz i = 0; i < clients.size(); ++i) clients[i]->raw_chat(f, text); return true; }

	bool send(const str& cmd, str& res);
};

class KatinaPluginReports
: public KatinaPlugin
{
public:

private:
//	KatinaPluginStats* stats;
//	KatinaPluginVotes* votes;
	KatinaPlugin* stats;
	KatinaPlugin* votes;

	RemoteClientList client;

	// cvars
	bool active;
	bool do_flags;
	bool do_flags_hud;
	bool do_caps;
	bool do_chats;
	bool do_kills;
	bool do_fc_kills; // kiils when carrying the flag
	bool do_pushes;
	bool do_announce_pushes; // rcon them to game
	bool do_infos;
	bool do_stats;
	str stats_cols; // %time %fph %cph %kpd %cpd %acc(RG|RL|LG)
	str stats_sort; // %fph
	bool spamkill;
	str chans;

	str_siz_map spam;
	siz spam_limit;

	slot fc[2] = {slot::bad, slot::bad}; // flag carriers

	siz flags[2];
	guid_siz_map caps; // GUID -> <count>

	str old_mapname;

	str_vec notspam; // spam exceptions

	str get_nums_team(slot num);
//	str get_nums_team(const GUID& guid);

	guid_str_map names;

public:
	KatinaPluginReports(Katina& katina);

	// INTERFACE: KatinaPlugin

	str_vec get_parent_plugin_ids() const override;

	str api(const str& cmd, void* blob = nullptr) override;

	bool open() override;

	str get_id() const override;
	str get_name() const override;
	str get_version() const override;

	bool init_game(siz min, siz sec, const str_map& cvars) override;
	bool kill(siz min, siz sec, slot num1, slot num2, siz weap) override;
	bool push(siz min, siz sec, slot num1, slot num2) override;
	bool ctf(siz min, siz sec, slot num, siz team, siz act) override;
	bool say(siz min, siz sec, slot num, const str& text) override;
	bool exit(siz min, siz sec) override;

	void close() override;
};

}} // katina::plugin

#endif	// KATINA_PLUGIN_REPORTS_H

