
#include "KatinaPluginAdmin.h"

#include <list>

#include <katina/types.h>
#include <katina/log.h>
#include <katina/str.h>

namespace katina { namespace plugin {

using namespace oastats::log;
using namespace oastats::types;

KATINA_PLUGIN_TYPE(KatinaPluginAdmin);
KATINA_PLUGIN_INFO("katina::admin", "Katina Admin", "0.1-dev");

KatinaPluginAdmin::KatinaPluginAdmin(Katina& katina)
: KatinaPlugin(katina)
, mapname(katina.mapname)
, clients(katina.clients)
, players(katina.players)
, teams(katina.teams)
, server(katina.server)
, active(true)
{
}

bool KatinaPluginAdmin::load_sanctions()
{
	sifs ifs((katina.config_dir + "/sanctions.dat").c_str());

	if(!ifs)
	{
		plog("ERROR: can not open sanctions file.");
		return false;
	}

	siz n = 0;
	str line;
	katina::plugin::sanction s;
	while(sgl(ifs, line))
	{
		++n;
		if(trim(line).empty())
			continue;
		siss iss(line);
		if(!(iss >> s))
		{
			plog("ERROR: parsing sanction file line: " << n);
			return false;
		}

		if(s.expires && s.expires < std::time(0))
			continue;

		sanctions.push_back(s);
	}

	return true;
}

bool KatinaPluginAdmin::save_sanctions()
{
	sofs ofs((katina.config_dir + "/sanctions.dat").c_str());

	if(!ofs)
	{
		plog("ERROR: can not open sanctions file.");
		return false;
	}

	for(sanction_lst_iter i = sanctions.begin(); i != sanctions.end(); ++i)
		ofs << *i << '\n';

	return true;
}

enum
{
	S_NONE = 0
	, S_MUTEPP
	, S_FIXNAME
	, S_WARN_ON_SIGHT
};

bool KatinaPluginAdmin::mutepp(siz num)
{
	str reply;
	server.command("!mute " + to_string(num), reply);
	// ����      broadcast: print "^3!mute: ^7^1S^2oo^3K^5ee^7 has been muted by ^7console\n"
	if(reply.find("broadcast: print") != str::npos
	&& reply.find("has been muted by") != str::npos)
		return true;
	return false;
}

bool KatinaPluginAdmin::fixname(siz num, const str& name)
{
	str reply;
	server.command("!rename " + to_string(num) + " " + name, reply);
	// ����      broadcast: print "^1S^2oo^3K^5ee^7 renamed to wibble\n"
	if(reply.find("broadcast: print") != str::npos
	&& reply.find("renamed to") != str::npos)
		return true;
	return false;
}

bool KatinaPluginAdmin::warn_on_sight(siz num, const str& reason)
{
	str reply;
	server.command("!warn " + to_string(num) + " " + reason, reply);
	// ����
	// ����      ^/warn: no connected player by that name or slot #
	if(reply.find("warn: no connected player by that name or slot #") != str::npos)
		return false;
	return true;
}

bool KatinaPluginAdmin::open()
{
	bug_func();

	bug("Adding var events");
	katina.add_var_event(this, "admin.active", active);
	//katina.add_var_event(this, "flag", "0");
	bug("Adding log events");
	katina.add_log_event(this, INIT_GAME);
	katina.add_log_event(this, WARMUP);
	katina.add_log_event(this, CLIENT_CONNECT);
	katina.add_log_event(this, CLIENT_BEGIN);
	katina.add_log_event(this, CLIENT_DISCONNECT);
	katina.add_log_event(this, CLIENT_USERINFO_CHANGED);
	katina.add_log_event(this, KILL);
	katina.add_log_event(this, CTF);
	katina.add_log_event(this, CTF_EXIT);
	katina.add_log_event(this, SCORE_EXIT);
	katina.add_log_event(this, AWARD);
	katina.add_log_event(this, SAY);
	katina.add_log_event(this, SHUTDOWN_GAME);
	katina.add_log_event(this, EXIT);
	katina.add_log_event(this, UNKNOWN);

	bug("Loading sanctions");
	load_sanctions();

	for(sanction_lst_iter s = sanctions.begin(); s != sanctions.end(); ++s)
	{
		if(players.find(s->guid) != players.end())
		{
			// !mute++
			if(s->type == S_MUTEPP)
				if(mutepp(katina.getClientNr(s->guid)))
					s->applied = true;
			// !fixname
			if(s->type == S_FIXNAME && !s->params.empty()
			&& s->params[0] == players[s->guid])
				if(fixname(katina.getClientNr(s->guid), s->params[0]))
					s->applied = true;
		}
	}

	bug("setting config");
	active = katina.get("admin.active", false);

	return true;
}

str KatinaPluginAdmin::get_id() const
{
	return ID;
}

str KatinaPluginAdmin::get_name() const
{
	return NAME;
}

str KatinaPluginAdmin::get_version() const
{
	return VERSION;
}

bool KatinaPluginAdmin::init_game(siz min, siz sec, const str_map& cvars)
{
	if(!active)
		return true;

	// !muted become unstuck after every game
	for(sanction_lst_iter s = sanctions.begin(); s != sanctions.end(); ++s)
		if(players.find(s->guid) != players.end())
			if(s->type == S_MUTEPP)
				if(mutepp(katina.getClientNr(s->guid)))
					s->applied = true;

	return true;
}

bool KatinaPluginAdmin::warmup(siz min, siz sec)
{
	if(!active)
		return true;
	return true;
}

bool KatinaPluginAdmin::client_connect(siz min, siz sec, siz num)
{
	if(!active)
		return true;
	return true;
}

bool KatinaPluginAdmin::client_begin(siz min, siz sec, siz num)
{
	if(!active)
		return true;
	return true;
}

bool KatinaPluginAdmin::client_disconnect(siz min, siz sec, siz num)
{
	if(!active)
		return true;
	return true;
}

bool KatinaPluginAdmin::client_userinfo_changed(siz min, siz sec, siz num, siz team
		, const GUID& guid, const str& name, siz hc)
{
	if(!active)
		return true;
//	plog("client_userinfo_changed(" << num << ", " << team << ", " << guid << ", " << name << ")");
//	plog("clients[" << num << "]         : " << clients[num]);
//	plog("players[clients[" << num << "]]: " << players[clients[num]]);

	for(sanction_lst_iter s = sanctions.begin(); s != sanctions.end();)
	{
		if(s->guid == guid)
		{
			if(s->type == S_FIXNAME)
			{
				if(!s->params.empty() && name != s->params[0])
					if(fixname(num, s->params[0]))
						s->applied = true;
				++s;
			}
			else if(s->type == S_WARN_ON_SIGHT && !s->params.empty())
			{
				if(warn_on_sight(num, s->params[0]))
					{ s = sanctions.erase(s); save_sanctions(); }
				else
					++s;
			}
		}
	}

	return true;
}

bool KatinaPluginAdmin::kill(siz min, siz sec, siz num1, siz num2, siz weap)
{
	if(!active)
		return true;
//	plog("kill(" << num1 << ", " << num2 << ", " << weap << ")");
	return true;
}

bool KatinaPluginAdmin::ctf(siz min, siz sec, siz num, siz team, siz act)
{
	if(!active)
		return true;
//	plog("ctf(" << num << ", " << team << ", " << act << ")");
	return true;
}

bool KatinaPluginAdmin::ctf_exit(siz min, siz sec, siz r, siz b)
{
	if(!active)
		return true;
//	plog("ctf_exit(" << r << ", " << b << ")");
	return true;
}

bool KatinaPluginAdmin::score_exit(siz min, siz sec, int score, siz ping, siz num, const str& name)
{
	if(!active)
		return true;
//	plog("score_exit(" << score << ", " << ping << ", " << num << ", " << name << ")");
	return true;	
}

bool KatinaPluginAdmin::award(siz min, siz sec, siz num, siz awd)
{
	if(!active)
		return true;
//	plog("award(" << num << ", " << awd << ")");
	return true;
}

const str ADMIN_ALIASES_FILE_KEY = "admin.aliases.file";
const str ADMIN_ALIASES_FILE_VAL = "admin.aliases.text";

/**
 * Potentially this function could
 * translate commands to avoid conflicting
 * with commands for other plugins.
 */
str KatinaPluginAdmin::trans(const str& cmd) const
{
	static str_map* aliases = 0;

	if(!aliases)
	{
		aliases = new str_map;

		sifs ifs((katina.config_dir + "/"
				+ katina.get(ADMIN_ALIASES_FILE_KEY, ADMIN_ALIASES_FILE_VAL)).c_str());

		if(!ifs)
			return cmd;

		siz pos;
		str line, key, val;
		while(sgl(ifs, line))
		{
			if((pos = line.find("//")) != str::npos)
				line.erase(pos);
			if(trim(line).empty() || line[0] == '#')
				continue;
			else
			{
				siss iss(line);
				if(sgl(sgl(iss, key, ':') >> std::ws, val))
					(*aliases)[key] = val;
			}
		}
	}

	if(aliases->empty())
		return cmd;

	if(aliases->find(cmd) == aliases->end())
		return cmd;

	return (*aliases)[cmd];
}

bool KatinaPluginAdmin::check_admin(const GUID& guid)
{
	if(!katina.is_admin(guid))
	{
		plog("WARN: Admin attempt by non admin player: [" << guid << "] " << players[guid]);
		return false;
	}
	return true;
}

bool KatinaPluginAdmin::check_slot(siz num)
{
	if(clients.find(num) == clients.end())
	{
		plog("WARN: Unknown client number: " << num);
		server.chat_nobeep("^7!ADMIN: ^3Unknown client number: ^7" + to_string(num));
		return false;
	}
	return true;
}

std::time_t parse_duration(const str& duration, std::time_t dflt)
{
	std::time_t t = dflt;

	// 30 w|d|h|m|s

	siss iss(duration);

	str units = "m";
	if(!(iss >> t >> std::ws >> units))
	{
		plog("ERROR: parsing duration: " << duration);
		return dflt;
	}

	if(units == "s")
		t *= 1;
	else if (units == "m")
		t *= 60;
	else if (units == "h")
		t *= 60 * 60;
	else if (units == "d")
		t *= 60 * 60 * 24;
	else if (units == "w")
		t *= 60 * 60 * 24 * 7;

	return t;
}

void KatinaPluginAdmin::remove_sanctions(const GUID& guid, siz type)
{
	for(sanction_lst_iter s = sanctions.begin(); s != sanctions.end();)
		if(s->guid == guid && s->type == type)
			s = sanctions.erase(s);
		else
			++s;
	save_sanctions();
}

bool KatinaPluginAdmin::say(siz min, siz sec, const GUID& guid, const str& text)
{
	if(!active)
		return true;

	plog("say(" << guid << ", " << text << ")");

	// !cmd <parans>

	siz say_num = katina.getClientNr(guid);

	if(say_num == siz(-1))
	{
		plog("ERROR: Unable to get slot number from guid: " << guid);
		return true;
	}

	if(!check_slot(say_num))
		return true;

	siss iss(text);

	str cmd, params;

	if(!(iss >> cmd >> std::ws))
	{
		plog("ERROR parsing admin command.");
		return true;
	}

	sgl(iss, params);

	iss.clear();
	iss.str(params);

	if(cmd == trans("!help") || cmd == trans("?help"))
	{
		server.msg_to(say_num, "^7ADMIN: ^2?request^7", true);

		if(!check_admin(guid))
			return true;

		server.msg_to(say_num, "^7ADMIN: ^2?sanctions^7, ^2?mute++^7, ^2?fixname^7");
		server.msg_to(say_num, "^7ADMIN: ^2?warnonsight^7");
	}
	else if(cmd == trans("!request") || cmd == trans("?request"))
	{
		if(cmd[0] == '?')
		{
			server.msg_to(say_num, "^7ADMIN: ^3Request a map? Or any other suggestion..", true);
			server.msg_to(say_num, "^7ADMIN: ^3!request <request>");
			return true;
		}

		str request;
		sgl(iss >> std::ws, request);

		sofs ofs((katina.config_dir + "/requests.txt").c_str(), sofs::app);
		if(ofs << clients[say_num] << ": " << request << " [" << players[clients[say_num]] << "]"<< '\n')
		{
			server.msg_to(say_num, "^7ADMIN: "
					+  players[guid]
					           + "^3, your request has been logged.", true);
		}
	}
	else if(cmd == trans("!sanctions") || cmd == trans("?sanctions"))
	{
		if(!check_admin(guid))
			return true;

		if(cmd[0] == '?')
		{
			server.msg_to(say_num, "^7ADMIN: ^3manage sanctions.", true);
			server.msg_to(say_num, "^7ADMIN: ^3!sanctions = list all sanctions.");
			server.msg_to(say_num, "^7ADMIN: ^3!sanctions <num> = list sanctions for player.");
			return true;
		}
	}
	else if(cmd == trans("!mute++") || cmd == trans("?mute++"))
	{
		// !mute++ <num> <duration>? <reason>?
		if(!check_admin(guid))
			return true;

		if(cmd[0] == '?')
		{
			server.msg_to(say_num, "^7ADMIN: ^3Keep a player muted for the specified duration.", true);
			server.msg_to(say_num, "^7ADMIN: ^3!mute++ <num> <duration>? <reason>?");
			server.msg_to(say_num, "^7ADMIN: ^3        <duration> = N(s|m|h|d|w) [eg, 5w = 5 weeks]");
			server.msg_to(say_num, "^7ADMIN: ^3!mute++ remove = remove mute");
			return true;
		}

		siz num = siz(-1);
		str duration = "5m";
		str reason;

		sgl(iss >> num >> duration >> std::ws, reason);

		if(!check_slot(num))
			return true;

		if(duration == "remove")
		{
			remove_sanctions(clients[num], S_MUTEPP);
			if(un_mutepp(num))
			{
				server.msg_to(num, "^7ADMIN: ^3Removed mute from: ^2" + players[clients[num]], true);
				if(num != say_num)
					server.msg_to(say_num, "^7ADMIN: ^3Removed mute from: ^2" + players[clients[num]], true);
			}
			return true;
		}

		sanction s;
		s.type = S_MUTEPP;
		s.guid = clients[num];
		s.expires = parse_duration(duration, 5 * 60);

		if(mutepp(num))
			s.applied = true;

		sanctions.push_back(s);
		save_sanctions();
	}
	else if(cmd == trans("!fixname") || cmd == trans("?fixname"))
	{
		// !fixname <slot> <name>
		if(!check_admin(guid))
			return true;

		if(cmd[0] == '?')
		{
			server.msg_to(say_num, "^7ADMIN: ^3Force a player to have a specific name.", true);
			server.msg_to(say_num, "^7ADMIN: ^3!fixname <num> <name>");
			server.msg_to(say_num, "^7ADMIN: ^3!fixname <num> remove");
			return true;
		}

		siz num = siz(-1);
		str name;

		sgl(iss >> num >> std::ws, name);

		if(!check_slot(num))
			return true;

		if(name == "remove")
		{
			remove_sanctions(clients[num], S_FIXNAME);
			server.msg_to(say_num, "^7ADMIN: ^3Removed fixed name from: ^2" + players[clients[num]], true);
			if(num != say_num)
				server.msg_to(num, "^7ADMIN: ^3Removed fixed name from: ^2" + players[clients[num]], true);
			return true;
		}

		sanction s;
		s.type = S_FIXNAME;
		s.guid = clients[num];
		s.expires = 0;
		s.params.push_back(name);

		if(fixname(num, name))
			s.applied = true;

		sanctions.push_back(s);
		save_sanctions();
	}
	else if(cmd == trans("!warnonsight") || cmd == trans("?warnonsight"))
	{
		// !warnonsight <GUID> <reason>
		if(!check_admin(guid))
			return true;

		if(cmd[0] == '?')
		{
			server.msg_to(say_num, "^7ADMIN: ^3Warn a player next time they connect.", true);
			server.msg_to(say_num, "^7ADMIN: ^3!warnonsight <GUID> <reason>");
			return true;
		}

		GUID guid;
		str reason;

		sgl(iss >> guid >> std::ws, reason);

		if(!guid)
		{
			server.msg_to(say_num, "^7ADMIN: ^1Error: ^3Bad GUID entered: ^2" + guid, true);
			return true;
		}

		if(trim(reason).empty())
		{
			server.msg_to(say_num, "^7ADMIN: ^1Error: ^3Must give reason to warn: ^2" + guid, true);
			return true;
		}

		if(reason == "remove")
		{
			remove_sanctions(guid, S_WARN_ON_SIGHT);
			server.msg_to(say_num, "^7ADMIN: ^3Removed warn-on-sight from: ^2" + guid, true);
			if(katina.getClientNr(guid) != siz(-1))
				server.msg_to(say_num, "^7ADMIN: ^3Removed warn-on-sight from: ^2" + guid, true);

			return true;
		}

		sanction s;
		s.type = S_WARN_ON_SIGHT;
		s.guid = guid;
		s.expires = 0;
		s.params.push_back(reason);

		sanctions.push_back(s);
		save_sanctions();
	}

	return true;
}

bool KatinaPluginAdmin::shutdown_game(siz min, siz sec)
{
	if(!active)
		return true;
//	plog("shutdown_game()");
	return true;
}

bool KatinaPluginAdmin::exit(siz min, siz sec)
{
	if(!active)
		return true;
//	plog("exit()");
	return true;
}

bool KatinaPluginAdmin::unknown(siz min, siz sec, const str& cmd, const str& params)
{
	if(!active)
		return true;
	return true;
}

void KatinaPluginAdmin::close()
{
}

}} // katina::plugin
