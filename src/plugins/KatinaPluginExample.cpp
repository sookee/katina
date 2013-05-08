
#include "KatinaPluginExample.h"

#include <katina/types.h>
#include <katina/log.h>

namespace katina { namespace plugin {

using namespace oastats::log;
using namespace oastats::types;

KATINA_PLUGIN_TYPE(KatinaPluginExample);
KATINA_PLUGIN_INFO("katina::example", "Katina Example", "0.1-dev");

KatinaPluginExample::KatinaPluginExample(Katina& katina)
: KatinaPlugin(katina)
, mapname(katina.mapname)
, clients(katina.clients)
, players(katina.players)
, teams(katina.teams)
, flag(false)
{
}

bool KatinaPluginExample::open()
{
	katina.add_var_event(this, "example_flag", flag);
	//katina.add_var_event(this, "flag", "0");
	katina.add_log_event(this, EXIT);
	katina.add_log_event(this, SHUTDOWN_GAME);
	katina.add_log_event(this, WARMUP);
	katina.add_log_event(this, CLIENT_USERINFO_CHANGED);
	katina.add_log_event(this, CLIENT_CONNECT);
	katina.add_log_event(this, CLIENT_DISCONNECT);
	katina.add_log_event(this, KILL);
	katina.add_log_event(this, CTF);
	katina.add_log_event(this, AWARD);
	katina.add_log_event(this, INIT_GAME);
	katina.add_log_event(this, SAY);
	katina.add_log_event(this, UNKNOWN);

	return true;
}

str KatinaPluginExample::get_id() const
{
	return ID;
}

str KatinaPluginExample::get_name() const
{
	return NAME;
}

str KatinaPluginExample::get_version() const
{
	return VERSION;
}

/*
void KatinaPluginExample::cvar_event(const str& name, const str& value)
{
	siss iss(value);
	if(name == "flag")
		iss >> flag;
	else
		plog("Unknown cvar: " << name);
	
	katina.server.s_chat("^3flag is now: ^7" + str(flag?"on":"off"));
}
*/

bool KatinaPluginExample::exit(siz min, siz sec)
{
	log("exit()");
	return true;
}

bool KatinaPluginExample::shutdown_game(siz min, siz sec)
{
	log("shutdown_game()");
	return true;
}

bool KatinaPluginExample::warmup(siz min, siz sec)
{
	log("warmup()");
	return true;
}

bool KatinaPluginExample::client_userinfo_changed(siz min, siz sec, siz num, siz team, const GUID& guid, const str& name)
{
	log("client_userinfo_changed(" << num << ", " << team << ", " << guid << ", " << name << ")");
	log("clients[" << num << "]         : " << clients[num]);
	log("players[clients[" << num << "]]: " << players[clients[num]]);
	return true;
}
bool KatinaPluginExample::client_connect(siz min, siz sec, siz num)
{
	log("client_connect(" << num << ")");
	return true;
}
bool KatinaPluginExample::client_disconnect(siz min, siz sec, siz num)
{
	log("client_disconnect(" << num << ")");
	return true;
}
bool KatinaPluginExample::kill(siz min, siz sec, siz num1, siz num2, siz weap)
{
	log("kill(" << num1 << ", " << num2 << ", " << weap << ")");
	return true;
}
bool KatinaPluginExample::ctf(siz min, siz sec, siz num, siz team, siz act)
{
	log("ctf(" << num << ", " << team << ", " << act<< ")");
	return true;
}
bool KatinaPluginExample::award(siz min, siz sec, siz num, siz awd)
{
	log("award(" << num << ", " << awd << ")");
	return true;
}

bool KatinaPluginExample::init_game(siz min, siz sec)
{
	log("init_game()");
	log("mapname: " << mapname);
	return true;
}

bool KatinaPluginExample::say(siz min, siz sec, const GUID& guid, const str& text)
{
	log("say(" << guid << ", " << text << ")");
	if(flag)
	{
		str s = text;
		std::reverse(s.begin(), s.end());
		katina.chat_to(guid, s);
	}
	return true;
}

bool KatinaPluginExample::unknown(siz min, siz sec, const str& cmd, const str& params)
{
	//log("unknown(" << line << ")");
	return true;
}

void KatinaPluginExample::close()
{
}

}} // katina::plugin
