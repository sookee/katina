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

	katina.add_log_event(this, INIT_GAME);
	katina.add_log_event(this, SAY);
	katina.add_log_event(this, EXIT);

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

bool KatinaPluginNextMap::init_game(siz min, siz sec, const str_map& cvars)
{
	if(!active)
		return true;

	if(rot_nextmap.empty())
		return true;

	if(enforcing && !server.command("set nextmap " + rot_nextmap))
	{
		plog("ERROR: can't reset rotation");
		return true;
	}

	rot_nextmap.clear();

	return true;
}

bool KatinaPluginNextMap::say(siz min, siz sec, const GUID& guid, const str& text)
{
	if(!active)
		return true;

	return true;
}

bool KatinaPluginNextMap::exit(siz min, siz sec)
{
	if(!active)
		return true;

	str sep;
	soss sql;
	for(slot_guid_map_citer i = clients.begin(); i != clients.end(); ++i)
		if(!i->second.is_bot() && katina.is_connected(i->first))
			{ sql << sep << "'" << i->second << "'"; sep = ",";}
	str insql = "(" + sql.str() + ")";


	sql.clear();
	sql.str("");
	sql << "select `item`,`count` from `votes` where `type` = 'map' and guid in " << insql;

	str_vec_vec rows;

	db_scoper on(db);

	if(!db.select(sql.str(), rows, 2))
		return true;

	struct opine
	{
		siz love, hate, soso;
		opine(): love(0), hate(0), soso(0) {}
	};

	TYPEDEF_MAP(str, opine, opine_map);

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

	siz select = rand() % total;

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

	if(rot_nextmap.empty())  // don't splat a rot_nextmap that failed to take
		if(!katina.rconset("nextmap", rot_nextmap))
			return true; // no action

	if(!server.command("set katina \"map " + nextmap + "; set nextmap " + rot_nextmap + "\""))
		return true;

	if(enforcing)
		if(!server.command("set nextmap vstr katina"))
			plog("ERROR: can't inject nextmap: " << nextmap);

	return true;
}

void KatinaPluginNextMap::close()
{
//	db.off();
}

}} // katina::plugin
