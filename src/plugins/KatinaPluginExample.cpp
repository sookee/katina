
#include "KatinaPluginExample.h"

#include <katina/types.h>
#include <katina/log.h>

namespace katina { namespace plugin {

using namespace oastats::log;
using namespace oastats::types;

KATINA_PLUGIN_TYPE(KatinaPluginExample);
KATINA_PLUGIN_INFO("katina::example", "Katina Example", "0.1-dev");

bool KatinaPluginExample::open()
{
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

bool KatinaPluginExample::exit()
{
	log("exit()");
	return true;
}

bool KatinaPluginExample::shutdown_game()
{
	log("shutdown_game()");
	return true;
}

bool KatinaPluginExample::warmup()
{
	log("warmup()");
	return true;
}

bool KatinaPluginExample::client_userinfo_changed(siz num, siz team, const GUID& guid, const str& name)
{
	log("client_userinfo_changed(" << num << ", " << team << ", " << guid << ", " << name << ")");
	log("clients[" << num << "]         : " << clients[num]);
	log("players[clients[" << num << "]]: " << players[clients[num]]);
	return true;
}
bool KatinaPluginExample::client_connect(siz num)
{
	log("client_connect(" << num << ")");
	return true;
}
bool KatinaPluginExample::client_disconnect(siz num)
{
	log("client_disconnect(" << num << ")");
	return true;
}
bool KatinaPluginExample::kill(siz num1, siz num2, siz weap)
{
	log("kill(" << num1 << ", " << num2 << ", " << weap << ")");
	return true;
}
bool KatinaPluginExample::ctf(siz num, siz team, siz act)
{
	log("ctf(" << num << ", " << team << ", " << act<< ")");
	return true;
}
bool KatinaPluginExample::award(siz num, siz awd)
{
	log("award(" << num << ", " << awd << ")");
	return true;
}

bool KatinaPluginExample::init_game()
{
	log("init_game()");
	log("mapname: " << mapname);
	return true;
}

bool KatinaPluginExample::say(const GUID& guid, const str& text)
{
	log("say(" << guid << ", " << text << ")");
	return true;
}

bool KatinaPluginExample::unknown(const str& line)
{
	//log("unknown(" << line << ")");
	return true;
}

void KatinaPluginExample::close()
{
}

}} // katina::plugin
