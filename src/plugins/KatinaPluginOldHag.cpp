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

#include "KatinaPluginOldHag.h"

#include <katina/types.h>
#include <katina/log.h>

namespace katina { namespace plugin {

using namespace oastats::log;
using namespace oastats::types;

KATINA_PLUGIN_TYPE(KatinaPluginOldHag);
KATINA_PLUGIN_INFO("katina::oldhag", "Katina Old Hag Game Management", "0.1-dev");

KatinaPluginOldHag::KatinaPluginOldHag(Katina& katina)
: KatinaPlugin(katina)
, server(katina.server)
, mapname(katina.mapname)
, clients(katina.clients)
, players(katina.players)
, teams(katina.teams)
, active(true)
{
}

bool KatinaPluginOldHag::open()
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

	return true;
}

str KatinaPluginOldHag::get_id() const
{
	return ID;
}

str KatinaPluginOldHag::get_name() const
{
	return NAME;
}

str KatinaPluginOldHag::get_version() const
{
	return VERSION;
}

bool KatinaPluginOldHag::init_game(siz min, siz sec, const str_map& cvars)
{
	if(!active)
		return true;
	plog("init_game()");
	plog("mapname: " << mapname);
	for(str_map_citer i = cvars.begin(); i != cvars.end(); ++i)
		plog("cvar: " << i->first << " = " << i->second);
	return true;
}

bool KatinaPluginOldHag::warmup(siz min, siz sec)
{
	if(!active)
		return true;
	plog("warmup()");
	return true;
}

bool KatinaPluginOldHag::client_connect(siz min, siz sec, slot num)
{
	if(!active)
		return true;
	plog("client_connect(" << num << ")");
	return true;
}

bool KatinaPluginOldHag::client_begin(siz min, siz sec, slot num)
{
	if(!active)
		return true;
	plog("client_begin(" << num << ")");
	katina.server.chat("BEGIN: " + players[clients[num]]);
	return true;
}

bool KatinaPluginOldHag::client_disconnect(siz min, siz sec, slot num)
{
	if(!active)
		return true;
	plog("client_disconnect(" << num << ")");
	return true;
}

bool KatinaPluginOldHag::client_userinfo_changed(siz min, siz sec, slot num, siz team, const GUID& guid, const str& name)
{
	if(!active)
		return true;
	plog("client_userinfo_changed(" << num << ", " << team << ", " << guid << ", " << name << ")");
	plog("clients[" << num << "]         : " << clients[num]);
	plog("players[clients[" << num << "]]: " << players[clients[num]]);
	return true;
}

bool KatinaPluginOldHag::kill(siz min, siz sec, slot num1, slot num2, siz weap)
{
	if(!active)
		return true;
	plog("kill(" << num1 << ", " << num2 << ", " << weap << ")");
	return true;
}

bool KatinaPluginOldHag::ctf(siz min, siz sec, slot num, siz team, siz act)
{
	if(!active)
		return true;
	plog("ctf(" << num << ", " << team << ", " << act << ")");
	return true;
}

bool KatinaPluginOldHag::ctf_exit(siz min, siz sec, siz r, siz b)
{
	if(!active)
		return true;
	plog("ctf_exit(" << r << ", " << b << ")");
	return true;
}

bool KatinaPluginOldHag::score_exit(siz min, siz sec, int score, siz ping, slot num, const str& name)
{
	if(!active)
		return true;
	plog("score_exit(" << score << ", " << ping << ", " << num << ", " << name << ")");
	return true;	
}

bool KatinaPluginOldHag::award(siz min, siz sec, slot num, siz awd)
{
	if(!active)
		return true;
	plog("award(" << num << ", " << awd << ")");
	return true;
}

bool KatinaPluginOldHag::say(siz min, siz sec, const GUID& guid, const str& text)
{
	if(!active)
		return true;
	plog("say(" << guid << ", " << text << ")");

	if(text.empty() || text[0] != '!')
		return true;

	std::istringstream iss(text);

	str cmd;

	if(!(iss >> cmd >> std::ws))
		return true;

	if(cmd == "!set_players")
	{
		if(iss >> hag_players)
			server.s_chat("Number of players set to: " + to_string(hag_players));
	}
	else if(cmd == "!set_bots")
	{
		if(iss >> hag_bots)
			server.s_chat("Number of bots set to: " + to_string(hag_bots));
	}
	else if(cmd == "!add")
	{
		if(hag_player.size() >= hag_players)
		{
			server.s_chat("ERROR: attempt to add too many players");
			return true;
		}
		siz num;
		if(iss >> num)
		{
			hag_player.push_back(num);
			server.s_chat("Adding player: " + players[clients[num]]);
		}
	}
	else if(cmd == "!rem")
	{
		if(hag_player.empty())
		{
			server.s_chat("ERROR: There are no players to remove.");
			return true;
		}
		siz num;
		if(iss >> num)
		{
			hag_player.push_back(num);
			server.s_chat("Adding player: " + players[clients[num]]);
		}
	}

	return true;
}

bool KatinaPluginOldHag::shutdown_game(siz min, siz sec)
{
	if(!active)
		return true;
	plog("shutdown_game()");
	return true;
}

bool KatinaPluginOldHag::exit(siz min, siz sec)
{
	if(!active)
		return true;
	plog("exit()");
	return true;
}

bool KatinaPluginOldHag::unknown(siz min, siz sec, const str& cmd, const str& params)
{
	if(!active)
		return true;
	return true;
}

void KatinaPluginOldHag::close()
{
}

}} // katina::plugin
