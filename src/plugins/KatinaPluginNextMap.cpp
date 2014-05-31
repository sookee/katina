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

using namespace oastats::log;
using namespace oastats::types;

KATINA_PLUGIN_TYPE(KatinaPluginNextMap);
KATINA_PLUGIN_INFO("katina::nextmap", "Katina NextMap", "0.1-dev");

KatinaPluginNextMap::KatinaPluginNextMap(Katina& katina)
: KatinaPlugin(katina)
//, votes(0)
, mapname(katina.mapname)
, clients(katina.clients)
, players(katina.players)
, teams(katina.teams)
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

	db.on();

	katina.add_var_event(this, "nextmap.active", active);
	katina.add_log_event(this, INIT_GAME);
	katina.add_log_event(this, CLIENT_CONNECT_INFO);
	katina.add_log_event(this, CLIENT_DISCONNECT);
	katina.add_log_event(this, SAY);
	katina.add_log_event(this, EXIT);

	if(!katina.rconset("nextmap", rotmap))
		plog("WARN: Unable to obtain rotation mapname");

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

	// get map stats for all known players ?

	return true;
}

bool KatinaPluginNextMap::client_connect_info(siz min, siz sec, siz num, const GUID& guid, const str& ip)
{
	if(!active)
		return true;

	pbug("Finding votes for player: " << guid << " " << players[guid]);

	// get map stats for this player
	soss sql;
	sql << "select `item`,`count` from `votes` where `type` = 'map' and guid = '" << str(guid) << "'";
	pbug_var(sql.str());
	str_vec_vec rows;
	if(!db.select(sql.str(), rows, 2))
	{
		pbug("UNREPORTED DATABASE ERROR: " << db.error());
		return true;
	}
	// guid -> {mapname, count}
	for(siz row = 0; row < rows.size(); ++row)
	{
		pbug("vote: " << rows[row][0] << " [" << rows[row][1] << "]");
		votes[guid] = vote(rows[row][0], to<int>(rows[row][1]));
	}

	return true;
}

bool KatinaPluginNextMap::client_disconnect(siz min, siz sec, siz num)
{
	if(!active)
		return true;

	// drop map stats
	votes.erase(clients[num]);

	return true;
}

bool KatinaPluginNextMap::say(siz min, siz sec, const GUID& guid, const str& text)
{
	if(!active)
		return true;
	//plog("say(" << guid << ", " << text << ")");
	return true;
}

bool KatinaPluginNextMap::exit(siz min, siz sec)
{
	if(!active)
		return true;
	bug_func();

	// set nextmap here
	str_int_map maps;

	for(guid_vote_map_iter i = votes.begin(); i != votes.end(); ++i)
	{
		pbug_var(i->first);
		pbug("maps[" << i->second.mapname << "] += " << i->second.count);
		maps[i->second.mapname] += i->second.count;
		pbug_var(maps[i->second.mapname]);
	}

	str nextmap; // next mapname

	int tot = 0;
	siz_str_map sort;
	for(str_int_map_citer i = maps.begin(); i != maps.end(); ++i)
		if(i->second > 0)
			{ sort[i->second] = i->first; tot += i->second; }

	pbug_var(tot);

	if(!tot)
		return true;

	siz pick = rand() % tot;

	pbug_var(pick);

	tot = 0;
	siz_str_map_citer i;
	for(i = sort.begin(); i != sort.end(); ++i)
	{
		if((tot += i->first) < pick)
		{
			pbug_var(tot);
			continue;
		}
	}

	if(i == sort.end())
	{
		plog("WARN: No map detected");

//		if(server.command("vstr " + rotmap))
//			if(!katina.rconset("nextmap", rotmap))
//				plog("WARN: Unable to obtain rotation mapname");

		return true;
	}

	pbug_var(i->first);
	pbug_var(i->second);

	nextmap = i->second;

	pbug_var(nextmap);
	// set m1 "map oasago2; set nextmap vstr m2"

	//server.msg_to_all("^3NEXT MAP SUGGESTS: ^7" + upper_copy(nextmap));
	plog("^3NEXTMAP SUGGESTS: ^7" + upper_copy(nextmap));

//	if(server.command("set xmap \"map " + nextmap + "; set nextmap " + rotmap + "\""))
//		if(server.command("vstr xmap"))
//			played[nextmap] = 0;
//
//	if(!katina.rconset("nextmap", rotmap))
//		plog("WARN: Unable to obtain rotation mapname");

	return true;
}

void KatinaPluginNextMap::close()
{
	db.off();}

}} // katina::plugin
