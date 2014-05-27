
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
//	if(katina.get_plugin("katina::votes", "0.0", votes))
//		plog("Found: " << votes->get_name());
//	else
//	{
//		plog("WARN: Unable to factor in player votes in map selection.");
//	}

	if(!katina.has("stats.db.base") || !katina.has("stats.db.user"))
	{
		plog("FATAL: no database config found");
		return false;
	}

	db.config(
		katina.get("stats.db.host", "localhost")
		, katina.get("stats.db.port", 3306)
		, katina.get("stats.db.user")
		, katina.get("stats.db.pass", "")
		, katina.get("stats.db.base"));

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

	bug("Finding votes for player: " << guid << players[guid]);

	// get map stats for this player
	soss sql;
	sql << "select `item`, `count` from `votes` where `type` = 'map' and guid = '" << guid << "'";

	str_vec_vec rows;
	if(!db.select(sql.str(), rows, 2))
		return true;

	// guid -> {mapname, count}
	for(siz row = 0; row < rows.size(); ++row)
	{
		bug("vote: " << rows[row][0] << " [" << rows[row][1] << "]");
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
	plog("say(" << guid << ", " << text << ")");
	return true;
}

bool KatinaPluginNextMap::exit(siz min, siz sec)
{
	if(!active)
		return true;

	// set nextmap here
	str_siz_map maps;

	for(guid_vote_map_iter i = votes.begin(); i != votes.end(); ++i)
	{
		bug_var(i->first);
		bug("maps[" << i->second.mapname << "] += " << i->second.count);
		maps[i->second.mapname] += i->second.count;
		bug_var(maps[i->second.mapname]);
	}

	str nextmap; // next mapname

	siz tot = 0;
	siz_str_map sort;
	for(str_siz_map_citer i = maps.begin(); i != maps.end(); ++i)
		if(i->second > 0)
			{ sort[i->second] = i->first; tot += i->second; }

	bug_var(tot);

	siz pick = rand() % tot;

	bug_var(pick);

	tot = 0;
	siz_str_map_citer i;
	for(i = sort.begin(); i != sort.end(); ++i)
	{
		if((tot += i->first) < pick)
		{
			bug_var(tot);
			continue;
		}
	}

	if(i == sort.end())
	{
		plog("WARN: No map delected");

		if(server.command("vstr " + rotmap))
			if(!katina.rconset("nextmap", rotmap))
				plog("WARN: Unable to obtain rotation mapname");

		return true;
	}

	bug_var(i->first);
	bug_var(i->second);

	nextmap = i->second;

	bug_var(nextmap);
	// set m1 "map oasago2; set nextmap vstr m2"

	server.msg_to_all("^3NEXT MAP SUGGESTS: ^7" + upper_copy(nextmap));

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
}

}} // katina::plugin
