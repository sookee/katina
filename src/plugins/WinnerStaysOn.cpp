
#include "WinnerStaysOn.h"

#include <katina/types.h>
#include <katina/log.h>

namespace katina { namespace plugin {

using namespace oastats::log;
using namespace oastats::types;

KATINA_PLUGIN_TYPE(WinnerStaysOn);
KATINA_PLUGIN_INFO("katina::winner-stays-on", "Winner Stays On", "0.1-dev");

bool WinnerStaysOn::open()
{
	if(!(stats = dynamic_cast<KatinaPluginStats*>(katina.get_plugin("katina::stats", "0.0"))))
	{
		log(" Found: " << stats->get_name());
		return false;
	}
	return true;
}

str WinnerStaysOn::get_id() const
{
	return ID;
}

str WinnerStaysOn::get_name() const
{
	return NAME;
}

str WinnerStaysOn::get_version() const
{
	return VERSION;
}

bool WinnerStaysOn::exit(siz min, siz sec)
{
	log("exit()");
	return true;
}

bool WinnerStaysOn::shutdown_game(siz min, siz sec)
{
	log("shutdown_game()");
	return true;
}

bool WinnerStaysOn::warmup(siz min, siz sec)
{
	log("warmup()");
	return true;
}

bool WinnerStaysOn::client_userinfo_changed(siz min, siz sec, siz num, siz team
	, const GUID& guid, const str& name)
{
	return true;
}

bool WinnerStaysOn::client_connect(siz min, siz sec, siz num)
{
	slot_deq_iter i = std:find(q.begin(), q.end(), num);
	if(i != q.end())
		erase(i);
	q.push_back(num);
	
	katina.chat_to(num, "You have been moved to the back of the queue.");
	katina.chat_to(num, "There are " + to_string(q.size() - 1) + " people in front of you.");
		
	return true;
}

bool WinnerStaysOn::client_disconnect(siz min, siz sec, siz num)
{
	slot_deq_iter i = std:find(q.begin(), q.end(), num);
	if(i != q.end())
		erase(i);
	return true;
}

bool WinnerStaysOn::kill(siz min, siz sec, siz num1, siz num2, siz weap)
{
	log("kill(" << num1 << ", " << num2 << ", " << weap << ")");
	return true;
}

bool WinnerStaysOn::ctf(siz min, siz sec, siz num, siz team, siz act)
{
	log("ctf(" << num << ", " << team << ", " << act<< ")");
	return true;
}

bool WinnerStaysOn::award(siz min, siz sec, siz num, siz awd)
{
	log("award(" << num << ", " << awd << ")");
	return true;
}

bool WinnerStaysOn::init_game(siz min, siz sec)
{
	log("init_game()");
	log("mapname: " << mapname);
	return true;
}

bool WinnerStaysOn::say(siz min, siz sec, const GUID& guid, const str& text)
{
	log("say(" << guid << ", " << text << ")");
	return true;
}

bool WinnerStaysOn::unknown(siz min, siz sec, const str& cmd, const str& params)
{
	//log("unknown(" << line << ")");
	return true;
}

void WinnerStaysOn::close()
{
}

}} // katina::plugin
