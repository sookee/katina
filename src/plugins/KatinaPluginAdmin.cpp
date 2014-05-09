
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

		if(s.expires > std::time(0))
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
	S_MUTED
};

bool KatinaPluginAdmin::apply_sanction(sanction_lst_iter& s)
{
	if(s->expires > std::time(0))
	{
		s = sanctions.erase(s);
		return true;
	}

	++s;

	siz num = katina.getClientNr(s->guid);

	if(num == siz(-1))
	{
		plog("Client slot number not found for " << s->guid);
		return false;
	}

	if(s->type == S_MUTED)
	{
		if(s->applied)
			return true;
		server.command("!mute " + to_string(num));
		s->applied = true;
	}
	else
	{
		plog("Unknown sanction type: " << s->type);
	}

	return true;
}

bool KatinaPluginAdmin::apply_sanctions()
{
	for(sanction_lst_iter s = sanctions.begin(); s != sanctions.end();)
		apply_sanction(s);

	return true;
}

bool KatinaPluginAdmin::open()
{
	katina.add_var_event(this, "example.active", active);
	//katina.add_var_event(this, "flag", "0");
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

	load_sanctions();
	apply_sanctions();

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
	plog("init_game()");
	plog("mapname: " << mapname);
	for(str_map_citer i = cvars.begin(); i != cvars.end(); ++i)
		plog("cvar: " << i->first << " = " << i->second);
	return true;
}

bool KatinaPluginAdmin::warmup(siz min, siz sec)
{
	if(!active)
		return true;
	plog("warmup()");
	return true;
}

bool KatinaPluginAdmin::client_connect(siz min, siz sec, siz num)
{
	if(!active)
		return true;
	plog("client_connect(" << num << ")");
	return true;
}

bool KatinaPluginAdmin::client_begin(siz min, siz sec, siz num)
{
	if(!active)
		return true;
	plog("client_begin(" << num << ")");
	katina.server.chat("BEGIN: " + players[clients[num]]);
	return true;
}

bool KatinaPluginAdmin::client_disconnect(siz min, siz sec, siz num)
{
	if(!active)
		return true;
	plog("client_disconnect(" << num << ")");
	return true;
}

bool KatinaPluginAdmin::client_userinfo_changed(siz min, siz sec, siz num, siz team
		, const GUID& guid, const str& name, siz hc)
{
	if(!active)
		return true;
	plog("client_userinfo_changed(" << num << ", " << team << ", " << guid << ", " << name << ")");
	plog("clients[" << num << "]         : " << clients[num]);
	plog("players[clients[" << num << "]]: " << players[clients[num]]);
	return true;
}

bool KatinaPluginAdmin::kill(siz min, siz sec, siz num1, siz num2, siz weap)
{
	if(!active)
		return true;
	plog("kill(" << num1 << ", " << num2 << ", " << weap << ")");
	return true;
}

bool KatinaPluginAdmin::ctf(siz min, siz sec, siz num, siz team, siz act)
{
	if(!active)
		return true;
	plog("ctf(" << num << ", " << team << ", " << act << ")");
	return true;
}

bool KatinaPluginAdmin::ctf_exit(siz min, siz sec, siz r, siz b)
{
	if(!active)
		return true;
	plog("ctf_exit(" << r << ", " << b << ")");
	return true;
}

bool KatinaPluginAdmin::score_exit(siz min, siz sec, int score, siz ping, siz num, const str& name)
{
	if(!active)
		return true;
	plog("score_exit(" << score << ", " << ping << ", " << num << ", " << name << ")");
	return true;	
}

bool KatinaPluginAdmin::award(siz min, siz sec, siz num, siz awd)
{
	if(!active)
		return true;
	plog("award(" << num << ", " << awd << ")");
	return true;
}

/**
 * Potentially this function could
 * translate commands to avoid conflicting
 * with commands for other plugins.
 */
str trans(const str& cmd)
{
	return cmd;
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

bool KatinaPluginAdmin::say(siz min, siz sec, const GUID& guid, const str& text)
{
	if(!active)
		return true;

	plog("say(" << guid << ", " << text << ")");

	// !cmd <parans>

	siss iss(text);

	str cmd, params;

	if(!sgl(iss >> cmd >> std::ws, params))
	{
		plog("ERROR parsing admin command.");
		return true;
	}

	iss.clear();
	iss.str(params);

	if(cmd == trans("!mute++"))
	{
		// !mute++ <num> <duration>? <reason>?
		if(!check_admin(guid))
			return true;

		siz num = siz(-1);
		str duration = "5m";
		str reason;

		sgl(iss >> num >> duration >> std::ws, reason);

		sanction s;
		s.guid = guid;
	}

	str s = text;
	std::reverse(s.begin(), s.end());
	katina.chat_to(guid, s);
	return true;
}

bool KatinaPluginAdmin::shutdown_game(siz min, siz sec)
{
	if(!active)
		return true;
	plog("shutdown_game()");
	return true;
}

bool KatinaPluginAdmin::exit(siz min, siz sec)
{
	if(!active)
		return true;
	plog("exit()");
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
