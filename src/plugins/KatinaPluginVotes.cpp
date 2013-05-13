
#include "KatinaPluginVotes.h"

#include <katina/KatinaPlugin.h>
#include <katina/GUID.h>

#include <katina/types.h>
#include <katina/log.h>

namespace katina { namespace plugin {

using namespace oastats::log;
using namespace oastats::types;

KATINA_PLUGIN_TYPE(KatinaPluginVotes);
KATINA_PLUGIN_INFO("katina::votes", "Katina Voting", "0.1");

KatinaPluginVotes::KatinaPluginVotes(Katina& katina)
: KatinaPlugin(katina)
, active(false)
{
}

bool KatinaPluginVotes::open()
{
	db.config(
		katina.get("db.host", "localhost")
		, katina.get("db.port", 3306)
		, katina.get("db.user")
		, katina.get("db.pass", "")
		, katina.get("db.base"));
	
	if(!katina.has("db.base") || !katina.has("db.user"))
	{
		plog("FATAL: no database config found");
		return false;
	}
	
	katina.add_var_event(this, "votes_active", active);

	katina.add_log_event(this, INIT_GAME);
	katina.add_log_event(this, SAY);

	active = katina.get("plugin.votes.active", false);

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

void KatinaPluginVotes::get_votes(siz& love, siz& hate)
{
	for(guid_int_map_iter i = map_votes.begin(); i != map_votes.end(); ++i)
	{
		if(i->second > 0)
			++love;
		else
			++hate;
	}
}

bool KatinaPluginVotes::init_game(siz min, siz sec, const str_map& cvars)
{
	if(!active)
		return true;
	
	// NB. This MUST be done before mapname changes
	db.on();
	for(guid_int_map_iter i = map_votes.begin(); i != map_votes.end(); ++i)
		db.add_vote("map", mapname, i->first, i->second);
	db.off();

	map_votes.clear();

	mapname = katina.mapname;

	// load map votes for new map
	db.on();
	db.read_map_votes(mapname, map_votes);
	db.off();

	return true;
}

bool KatinaPluginVotes::say(siz min, siz sec, const GUID& guid, const str& text)
{
	if(!active)
		return true;
	
	// say(3EA47384, would be difficult with a lot of players)
	str cmd, type;
	siss iss(text);
	
	if(!(iss >> cmd >> type) || cmd.empty() || type.empty() || cmd[0] != '!')
		return true;

	lower(cmd);
	lower(type);
	
	if(type != "map")
	{
		plog("WARNING: Unknown vote type: " << type);
		return false;
	}
	
	if(cmd == "!love") // TODO:
	{
		if(map_votes.count(guid) && map_votes[guid] == 1)
			katina.server.chat("^3You have already voted for this map.");
		else if(map_votes.count(guid))
			katina.server.chat("^3Your vote has changed for this map.");
		else
			katina.server.chat("^7" + katina.players[guid] + "^7: ^3Your vote will be counted.");
		map_votes[guid] = 1;
	}
	else if(cmd == "!hate") // TODO:
	{
		if(map_votes.count(guid) && map_votes[guid] == -1)
			katina.server.chat("^3You have already voted for this map.");
		else if(map_votes.count(guid))
			katina.server.chat("^3Your vote has changed for this map.");
		else
			katina.server.chat("^7" + katina.players[guid] + "^7: ^3Your vote will be counted.");
		map_votes[guid] = -1;
	}
	
	return true;
}

void KatinaPluginVotes::close()
{

}

}} // katina::plugin