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

#include "KatinaPluginVotes.h"

#include <thread>
#include <future>

#include <katina/KatinaPlugin.h>
#include <katina/GUID.h>

#include <katina/types.h>
#include <katina/time.h>
#include <katina/log.h>

namespace katina { namespace plugin {

using namespace katina;
using namespace katina::log;
using namespace katina::time;
using namespace katina::types;

KATINA_PLUGIN_TYPE(KatinaPluginVotes);
KATINA_PLUGIN_INFO("katina::votes", "Katina Voting", "0.1");

KatinaPluginVotes::KatinaPluginVotes(Katina& katina)
: KatinaPlugin(katina)
, active(false)
, announce_time(0)
{
}

bool KatinaPluginVotes::open()
{
	str default_db = katina.get("db");

	if(!default_db.empty())
	{
		plog("DEFAULT DB: " << default_db);
		default_db += ".";
	}

	str host = katina.get("votes.db.host", katina.get(default_db + "db.host", "localhost"));
	siz port = katina.get("votes.db.port", katina.get(default_db + "db.port", 3306));
	str user = katina.get("votes.db.user", katina.get(default_db + "db.user", ""));
	str pass = katina.get("votes.db.pass", katina.get(default_db + "db.pass", ""));
	str base = katina.get("votes.db.base", katina.get(default_db + "db.base"));

	if(base.empty())
	{
		plog("FATAL: Database config not found");
		return false;
	}

	db.config(host, port, user, pass, base);

	if(!db.check())
	{
		plog("FATAL: Database can not connect");
		return false;
	}

	katina.add_var_event(this, "votes.active", active, false);

	katina.add_log_event(this, INIT_GAME);
	katina.add_log_event(this, WARMUP);
	katina.add_log_event(this, SAY);
	katina.add_log_event(this, SAYTEAM);
//	katina.add_log_event(this, HEARTBEAT);

	return true;
}

str KatinaPluginVotes::get_id() const
{
	return ID;
}

str KatinaPluginVotes::get_name() const
{
	return NAME;
}

str KatinaPluginVotes::get_version() const
{
	return VERSION;
}

str KatinaPluginVotes::api(const str& call, void* blob)
{
	siss iss(call);

	str cmd;

	if(!(iss >> cmd))
		return "ERROR: bad call: " + call;

	if(trim(cmd) == "get_votes")
	{
		siz love, hate, soso;
		get_votes(love, hate, soso);
		return "OK: " + std::to_string(love) + " " + std::to_string(hate) + " " + std::to_string(soso);
	}
	else
	{
		return "ERROR: unknown request: " + cmd;
	}

	return KatinaPlugin::api(call);
}

void KatinaPluginVotes::get_votes(siz& love, siz& hate, siz &soso)
{
	love = 0;
	hate = 0;
	soso = 0;

	for(guid_int_map_iter i = map_votes.begin(); i != map_votes.end(); ++i)
	{
		if(i->second > 0)
			++love;
		else if(i->second < 0)
			++hate;
		else
			++soso;
	}
}

bool KatinaPluginVotes::init_game(siz min, siz sec, const str_map& cvars)
{
	if(!active)
		return true;
	bug_func();
	
	// NB. This MUST be done before mapname changes
	db_scoper on(db);
	for(guid_int_map_iter i = map_votes.begin(); i != map_votes.end(); ++i)
		db.add_vote("map", mapname, i->first, i->second);
//	db.off();

	map_votes.clear();
	mapname = katina.get_mapname();

	// load map votes for new map
	if(!db.read_map_votes(mapname, map_votes))
	{
		map_votes.clear();
		plog("WARN: failed to read map votes");
		return true;
		announce_time = 0;
	}

	if(!announce_time)
		announce_time = sec + katina.get("votes.announce.delay", 10);

	katina.add_log_event(this, HEARTBEAT);

	return true;
}

bool KatinaPluginVotes::warmup(siz min, siz sec)
{
	// kybosch the announcement
	announce_time = 0;
	katina.del_log_event(this, HEARTBEAT);
	return true;
}

void KatinaPluginVotes::heartbeat(siz min, siz sec)
{
	if(!announce_time || min || sec < announce_time)
		return;


	pbug("HEARTBEAT");

	announce_time = 0; // turn off

	std::async(std::launch::async, [this]
	{
		const slot_guid_map clients = katina.getClients();

		for(slot_guid_map_citer i = clients.begin(); i != clients.end(); ++i)
		{
			// were they connected when the thread began?
			if(i->second.is_bot() || !i->second.is_connected())
				continue;

			{
				// have they disconnected since the thread began?
				lock_guard lock(katina.get_data_mutex());
				const GUID& guid = katina.getClientGuid(i->first);
				if(guid == null_guid || !guid.is_connected())
					continue;
			}

			if(katina.is_live())
				thread_sleep_millis(2000);

			pbug("ANNOUNCING VOTE TO: " << i->second << " " << katina.getPlayerName(i->second));

			bug_var(i->first);
			if(i->first == slot::bad)
			{
				plog("ERROR: Bad client number: " << i->first);
				continue;
			}

			if(i->first >= slot::max)
			{
				plog("ERROR: Client number too large: " << i->first);
				continue;
			}

			pbug_var(i->first);
			pbug_var(i->second);
	//		pbug_var(map_votes[i->second]);

			if(map_votes.count(i->second) && map_votes[i->second] > 0)
				katina.server.msg_to(i->first, katina.get_name() + " ^3You ^1LOVE ^3this map");
			else if(map_votes.count(i->second) && map_votes[i->second] < 0)
				katina.server.msg_to(i->first, katina.get_name() + " ^3You ^1HATE ^3this map");
			else if(map_votes.count(i->second))
				katina.server.msg_to(i->first, katina.get_name() + " ^3You neither ^1LOVE ^3nor ^1HATE ^3this map");
			else
			{
				katina.server.msg_to(i->first, katina.get_name() + " ^3You have not yet voted for this map.", true);
				katina.server.msg_to(i->first, katina.get_name() + " ^3You can say ^1!love map ^3 or ^1!hate map ^3 to express a preference.");
				katina.server.msg_to(i->first, katina.get_name() + " ^3If you don't care say ^1!soso map ^3 to make this message disappear.");
			}
		}
	});
}

bool KatinaPluginVotes::sayteam(siz min, siz sec, const GUID& guid, const str& text)
{
	return say(min, sec, guid, text);
}

bool KatinaPluginVotes::say(siz min, siz sec, const GUID& guid, const str& text)
{
	if(!active)
		return true;
	
	str cmd, type;
	siss iss(text);

	// say(3EA47384, would be difficult with a lot of players)
	if(!(iss >> cmd) || cmd.empty() || (cmd[0] != '!' && cmd[0] != '?'))
		return true;

	slot say_num;

	if((say_num = katina.getClientSlot(guid)) == slot::bad)
	{
		plog("ERROR: Unable to get slot number from guid: " << guid);
		return true;
	}

	iss >> type;

	lower(cmd);
	lower(type);

	if(cmd == "!lovemap")
	{
		cmd = "!love";
		type = "map";
	}

	if(cmd == "!hatemap")
	{
		cmd = "!hate";
		type = "map";
	}

	if(cmd == "!help" || cmd == "?help")
	{
		katina.server.msg_to(say_num, "^7VOTES: ^2?love^7, ^2?hate^7");
	}
	else if(cmd == "?love")
	{
		katina.server.msg_to(say_num, "^7VOTES: ^3Vote to keep a feature like the current map");
		katina.server.msg_to(say_num, "^7VOTES: ^3Use^7: !^2love map ^3if you want to keep the current map");
	}
	else if(cmd == "?hate")
	{
		katina.server.msg_to(say_num, "^7VOTES: ^3Vote to lose a feature like the current map");
		katina.server.msg_to(say_num, "^7VOTES: ^3Use^7: !^2hate map ^3if you want to lose the current map");
	}
	else if(cmd == "?soso")
	{
		katina.server.msg_to(say_num, "^7VOTES: ^3Vote if you don't care one way or the other about a feature");
		katina.server.msg_to(say_num, "^7VOTES: ^3Use^7: !^2soso map ^3if you really don't mind if the map stays or goes");
	}
	else if(cmd == "!love")
	{
		if(type != "map")
		{
			plog("WARNING: Unknown vote type: " << type);
			return true;
		}

		if(map_votes.count(guid) && map_votes[guid] == 1)
			katina.server.msg_to(say_num, "^3You have already voted for this map.", true);
		else if(map_votes.count(guid))
			katina.server.msg_to(say_num, "^3Your vote has changed for this map.", true);
		else
			katina.server.msg_to(say_num, "^7" + katina.getPlayerName(guid) + "^7: ^3Your vote will be counted.", true);
		map_votes[guid] = 1;
	}
	else if(cmd == "!hate")
	{
		if(type != "map")
		{
			plog("WARNING: Unknown vote type: " << type);
			return true;
		}

		if(map_votes.count(guid) && map_votes[guid] == -1)
			katina.server.msg_to(say_num, "^3You have already voted for this map.", true);
		else if(map_votes.count(guid))
			katina.server.msg_to(say_num, "^3Your vote has changed for this map.", true);
		else
			katina.server.msg_to(say_num, "^7" + katina.getPlayerName(guid) + "^7: ^3Your vote will be counted.", true);
		map_votes[guid] = -1;
	}
	else if(cmd == "!soso")
	{
		if(type != "map")
		{
			plog("WARNING: Unknown vote type: " << type);
			return true;
		}

		if(map_votes.count(guid) && map_votes[guid] == 0)
			katina.server.msg_to(say_num, "^3You have already voted for this map.", true);
		else if(map_votes.count(guid))
			katina.server.msg_to(say_num, "^3Your vote has changed for this map.", true);
		else
			katina.server.msg_to(say_num, "^7" + katina.getPlayerName(guid) + "^7: ^3Your vote will be counted.", true);
		map_votes[guid] = 0;
	}
	
	return true;
}

void KatinaPluginVotes::close()
{

}

row_count VotesDatabase::add_vote(const str& type, const str& item, const GUID& guid, int count)
{
	if(trace)
		log("DATABASE: add_vote(" << type << ", " << item << ", " << guid << ", " << count << ")");

	soss oss;
	oss << "insert into `votes` (`type`,`item`,`guid`,`count`) values ('"
		<< type << "','" << item << "','" << guid << "','" << count << "')"
		<< " on duplicate key update `count` = '" << count << "'";

	str sql = oss.str();

	my_ulonglong update_count = 0;

	if(!update(sql, update_count))
		return 0;

	return update_count;
}

bool VotesDatabase::read_map_votes(const str& mapname, guid_int_map& map_votes)
{
	if(trace)
		log("DATABASE: read_map_votes(" << mapname << ")");

	map_votes.clear();

	str safe_mapname;
	if(!escape(mapname, safe_mapname))
	{
		log("DATABASE: ERROR: failed to escape: " << mapname);
		return bad_id;
	}

	soss oss;
	oss << "select `guid`,`count` from `votes` where `type` = 'map' and `item` = '" << safe_mapname << "'";

	str sql = oss.str();

	str_vec_vec rows;
	if(!select(sql, rows, 2))
		return false;

	for(siz i = 0; i < rows.size(); ++i)
	{
		if(trace)
		log("DATABASE: restoring vote: " << rows[i][0] << ": " << rows[i][1]);
		map_votes[GUID(rows[i][0])] = to<int>(rows[i][1]);
	}

	return true;
}

}} // katina::plugin
