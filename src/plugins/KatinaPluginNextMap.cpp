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

#include "KatinaPluginNextMap.h"

#include <cstdint>

#include <katina/types.h>
#include <katina/log.h>

namespace katina { namespace plugin {

using namespace katina::log;
using namespace katina::types;

KATINA_PLUGIN_TYPE(KatinaPluginNextMap);
KATINA_PLUGIN_INFO("katina::nextmap", "Katina NextMap", "0.1-dev");

KatinaPluginNextMap::KatinaPluginNextMap(Katina& katina)
: KatinaPlugin(katina)
//, votes(0)
, mapname(katina.get_mapname())
, clients(katina.getClients())
, players(katina.getPlayers())
, teams(katina.getTeams())
, server(katina.server)
, active(true)
{
}

bool KatinaPluginNextMap::open()
{
	str host = katina.get("votes.db.host", katina.get("db.host", "localhost"));
	siz port = katina.get("votes.db.port", katina.get("db.port", 3306));
	str user = katina.get("votes.db.user", katina.get("db.user", ""));
	str pass = katina.get("votes.db.pass", katina.get("db.pass", ""));
	str base = katina.get("votes.db.base", katina.get("db.base"));

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

	katina.add_var_event(this, "nextmap.active", active, false);
	katina.add_var_event(this, "nextmap.enforcing", enforcing, false);
	katina.add_var_event(this, "nextmap.announce.active", announce_active, false);
	katina.add_var_event(this, "nextmap.announce.batch.size", announce_batch_size, siz(3));
	katina.add_var_event(this, "nextmap.announce.delay", announce_delay, siz(6));

	katina.add_log_event(this, KE_INIT_GAME);
	katina.add_log_event(this, KE_WARMUP);
	katina.add_log_event(this, KE_SAY);
	katina.add_log_event(this, KE_EXIT);

	return true;
}

str KatinaPluginNextMap::get_id() const
{
	return ID;
}

str KatinaPluginNextMap::get_name() const
{
	return NAME;
}

str KatinaPluginNextMap::get_version() const
{
	return VERSION;
}

str KatinaPluginNextMap::get_maplist(const str_vec& maps, siz batch)
{
	soss oss;
	oss << "\\n^7[^2#^3 Upcoming maps ^2#^7]";
	for(siz i = 0; i < maps.size(); ++i)
	{
		str mapname = "^5";
		for(const auto& c: maps[i])
		{
			if(std::isdigit(c))
				mapname += str("^4") + c + "^5";
			else
				mapname += c;
		}
		str idx = std::to_string((announce_batch_size * batch) + i + 1);
		if(idx.size() < 3)
			idx = str(3 - idx.size(), ' ') + idx;
		bug("idx: [" << idx << "]");
		oss << "\\n" << "\"" << idx + "^2: " + mapname << "\"";
	}
	return oss.str();
}

bool KatinaPluginNextMap::init_game(siz min, siz sec, const str_map& cvars)
{
	if(!active)
		return true;

	if(announce_active)
	{
		if(!announce_time)
			announce_time = sec + announce_delay;

		katina.add_log_event(this, KE_HEARTBEAT);
	}

	return true;
}

bool KatinaPluginNextMap::warmup(siz min, siz sec)
{
	// kybosch the announcement
	announce_time = 0;
	katina.del_log_event(this, KE_HEARTBEAT);

	return true;
}

void KatinaPluginNextMap::heartbeat(siz min, siz sec)
{
	if(!announce_time || min || sec < announce_time)
		return;

	bug_func();

	announce_time = 0; // turn off
	katina.del_log_event(this, KE_HEARTBEAT);

	pbug("HEARTBEAT");

	str_vec maps = get_mapnames(0);

	for(auto&& map: maps)
		bug_var(map);

	str msg = get_maplist(maps, 0);

	server.msg_to_all(msg, true);
}

str get_mapname(str line)
{
	siss iss(line);
	str item;
	if(!sgl(iss >> item >> item >> item >> std::ws, item, ';'))
		item.clear();
	return trim(item);
}

str_vec KatinaPluginNextMap::get_mapnames(siz batch)
{
	// TODO: avoid commented out lines
	bug_func();
	bug_var(batch);

	str reply;
	if(!server.command("nextmap", reply))
	{
		plog("ERROR: parsing nextmap reply: " << reply);
		return {};
	}

	bug_var(reply);

	str m;
	siss iss(reply);
	if(!sgl(iss >> m >> m >> std::ws, m, '^'))
	{
		plog("ERROR: parsing nextmap reply: " << reply);
		return {};
	}

	str_vec maps;

	str map_rot = katina.get_exp("nextmap.rot.file");

	if(map_rot.empty())
		return {};

	sifs ifs(map_rot);
	if(!ifs)
	{
		log("WARN: map rotation file not found: " << map_rot);
		return {};
	}

	// set m8 "map q3wcp18; set nextmap vstr m9"
	// set m9 "map oasago2j; set nextmap vstr m10"
	// set m10 "map actf18; set nextmap vstr m11"
	// set m11 "map god_oasago2z; set nextmap vstr m12"
	// set m12 "map mapel4b; set nextmap vstr m13"
	// set m13 "map q3wcp17; set nextmap vstr m14"
	// set m14 "map am_thornish; set nextmap vstr m15"
	// set m15 "map wtf01-pro; set nextmap vstr m16"
	// set m16 "map 13dream; set nextmap vstr m17"
	// set m17 "map pul1ctf; set nextmap vstr m18"

	str line;
	str item;

	while(item != m && sgl(ifs, line))
	{
		siss(line) >> item >> item;
		bug_var(item);
	}

	bug_var(line);
	bug_var(item);

	// find right batch

	siz i = 0;
	while(i < announce_batch_size * batch)
	{
		for(; i < announce_batch_size * batch && sgl(ifs, line); ++i) {}
		if(!ifs)
		{
			ifs.clear();
			ifs.seekg(0);
		}
	}

	bug_var(i);
	bug_var((announce_batch_size * batch) + announce_batch_size);

	while(i < (announce_batch_size * batch) + announce_batch_size)
	{
		do
		{
			maps.push_back(get_mapname(std::move(line)));
			++i;
		}
		while(i < (announce_batch_size * batch) + announce_batch_size && sgl(ifs, line));
		ifs.clear();
		ifs.seekg(0);
	}

//	i = 0;
//	while(i < 10)
//	{
//		for(; i < 10 && sgl(ifs, line); ++i)
//			maps.push_back(get_mapname(line));
//		ifs.clear();
//		ifs.seekg(0);
//	}

	return maps;
}

bool KatinaPluginNextMap::say(siz min, siz sec, const GUID& guid, const str& text)
{
	bug_func();
	bug_var(active);
	bug_var(guid);
	bug_var(text);
	if(!active)
		return true;

	str cmd, type;
	siss iss(text);

	// say(3EA47384, would be difficult with a lot of players)
	if(!(iss >> cmd) || cmd.empty() || (cmd[0] != '!' && cmd[0] != '?'))
		return true;

	bug_var(cmd);

	slot say_num;

	if((say_num = katina.getClientSlot(guid)) == slot::bad)
	{
		plog("ERROR: Unable to get slot number from guid: " << guid);
		return true;
	}

	if(cmd == "!maps")
	{
		siz batch = 0;
		if(!(iss >> batch))
			batch = 0;

		// !maps
		// rcon nextmap
		// "nextmap" is:"vstr m13^7" default:"^7"

		str_vec maps = get_mapnames(batch);

		for(auto&& map: maps)
			bug_var(map);

		str msg = get_maplist(maps, batch);

		server.msg_to(say_num, msg);
	}

	return true;
}

bool contains(const str_vec& v, const str& s)
{
	return std::find(v.cbegin(), v.cend(), s) != v.cend();
}

struct opine
{
	siz love, hate, soso;
	opine(): love(0), hate(0), soso(0) {}
};

TYPEDEF_MAP(str, opine, opine_map);

TYPEDEF_MAP(uint16_t, opine, guid_opine_map);

bool KatinaPluginNextMap::exit(siz min, siz sec)
{
	if(!active)
		return true;

	str sep;
	soss sql;
	for(slot_guid_map_citer i = clients.begin(); i != clients.end(); ++i)
		if(!i->second.is_bot() && i->second.is_connected())
			if(!contains(katina.get_vec("nextmap.ignore.guid"), str(i->second)))
				{ sql << sep << "'" << i->second << "'"; sep = ",";}

	if(sql.str().empty())
		return true; // no one connected/not ignored

	str insql = "(" + sql.str() + ")";

	sql.clear();
	sql.str("");
	sql << "select `item`,`count` from `votes` where `type` = 'map' and guid in " << insql;

	str_vec_vec rows;

	db_scoper on(db);

	if(!db.select(sql.str(), rows, 2))
		return true;

	opine_map votes; // mapname -> { love, hate, soso }

	const str_vec& banned = katina.get_vec("nextmap.banned");

	int vote;

	for(const str_vec& row: rows)
	{
		if(std::find(banned.begin(), banned.end(), row[0]) != banned.end())
			continue;
		if(mapname == row[0])
			continue;
		vote = to<int>(row[1]);
		if(vote > 0)
			++votes[row[0]].love;
		else if(vote < 0)
			++votes[row[0]].hate;
		else
			++votes[row[0]].soso;
	}

	str_siz_map maps;

	siz total = 0;

	for(opine_map_vt& v: votes)
	{
		vote = (v.second.love * 2) + v.second.soso - v.second.hate;
		if(vote < 1)
			continue;
		total += (maps[v.first] = vote);
	}

	if(maps.size() < katina.get("nextmap.min.samples", 10))
	{
		plog("NEXTMAP: not enough maps to select from: " << maps.size());
		return true;
	}

	siz select = total ? rand() % total : 0;

	total = 0;
	for(const str_siz_map_vt& m: maps)
	{
		total += m.second;
		if(total < select)
			continue;
		nextmap = m.first;
		break;
	}

	if(nextmap.empty())
	{
		plog("NEXTMAP: failed to select a map");
		return true;
	}

	plog("NEXTMAP SUGGESTS: " << nextmap << " from " << maps.size() << (enforcing?" ENFORCING":" NOT ENFORCING"));

	if(!enforcing)
		return true;

	if(!katina.rconset("nextmap", rot_nextmap))
		return true; // no action

	if(!server.command("set katina \"map " + nextmap + "; set nextmap " + rot_nextmap + "\""))
		return true;

	if(!server.command("set nextmap vstr katina"))
		plog("ERROR: Failed to inject nextmap: " << nextmap);

	// rot_nextmap contains original rotation script
	// running with script katina as 'nextmap'

	return true;
}

void KatinaPluginNextMap::close()
{
}

}} // katina::plugin
