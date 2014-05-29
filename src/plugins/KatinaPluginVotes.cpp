
#include "KatinaPluginVotes.h"

#include <katina/KatinaPlugin.h>
#include <katina/GUID.h>

#include <katina/types.h>
#include <katina/log.h>

namespace katina { namespace plugin {

using namespace oastats;
using namespace oastats::log;
using namespace oastats::types;

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

	katina.add_var_event(this, "votes.active", active, false);

	katina.add_log_event(this, INIT_GAME);
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

void KatinaPluginVotes::get_votes(siz& love, siz& hate)
{
	love = 0;
	hate = 0;

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
	bug_func();
	
	// NB. This MUST be done before mapname changes
	db.on();
	for(guid_int_map_iter i = map_votes.begin(); i != map_votes.end(); ++i)
		db.add_vote("map", mapname, i->first, i->second);
//	db.off();

	map_votes.clear();
	mapname = katina.mapname;

	// load map votes for new map
//	db.on();
	db.read_map_votes(mapname, map_votes);
	db.off();

	announce_time = 30;
	katina.add_log_event(this, HEARTBEAT);

	return true;
}

void KatinaPluginVotes::heartbeat(siz min, siz sec)
{
	if(!announce_time || min || sec < announce_time)
		return;

	siz num;
	for(guid_int_map_citer i = map_votes.begin(); i != map_votes.end(); ++i)
	{
		pbug("ANNOUNCING VOTE TO: " << i->first << " " << katina.players[i->first]);

		if((num = katina.getClientNr(i->first)) == siz(-1))
			continue;

		bug_var(num);

		if(i->second > 0)
			katina.server.msg_to(num, katina.get_name() + " ^3You ^1LOVE ^3this map");
		else if(i->second < 0)
			katina.server.msg_to(num, katina.get_name() + " ^3You ^1HATE ^3this map");
		else
		{
			katina.server.msg_to(num, katina.get_name() + " ^3You have not yet voted for this map.", true);
			katina.server.msg_to(num, katina.get_name() + " ^3You can say ^1!love map ^3 or ^1!hate map ^3 to express a preference.");
		}
	}
	announce_time = 0; // turn off
	katina.del_log_event(this, HEARTBEAT);
}

bool KatinaPluginVotes::sayteam(siz min, siz sec, const GUID& guid, const str& text)
{
	return say(min, sec, guid, text);
}

bool KatinaPluginVotes::say(siz min, siz sec, const GUID& guid, const str& text)
{
	if(!active)
		return true;
	
	siz say_num = katina.getClientNr(guid);

	if(say_num == siz(-1))
	{
		plog("ERROR: Unable to get slot number from guid: " << guid);
		return true;
	}

	if(!katina.check_slot(say_num))
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
	else if(cmd == "!love") // TODO:
	{
		if(map_votes.count(guid) && map_votes[guid] == 1)
			katina.server.msg_to(say_num, "^3You have already voted for this map.", true);
		else if(map_votes.count(guid))
			katina.server.msg_to(say_num, "^3Your vote has changed for this map.", true);
		else
			katina.server.msg_to(say_num, "^7" + katina.players[guid] + "^7: ^3Your vote will be counted.", true);
		map_votes[guid] = 1;
	}
	else if(cmd == "!hate") // TODO:
	{
		if(map_votes.count(guid) && map_votes[guid] == -1)
			katina.server.msg_to(say_num, "^3You have already voted for this map.", true);
		else if(map_votes.count(guid))
			katina.server.msg_to(say_num, "^3Your vote has changed for this map.", true);
		else
			katina.server.msg_to(say_num, "^7" + katina.players[guid] + "^7: ^3Your vote will be counted.", true);
		map_votes[guid] = -1;
	}
	
	return true;
}

void KatinaPluginVotes::close()
{

}

}} // katina::plugin
