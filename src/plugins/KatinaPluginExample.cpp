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

#include "KatinaPluginExample.h"

#include <katina/types.h>
#include <katina/log.h>

namespace katina { namespace plugin {

using namespace katina::log;
using namespace katina::types;

KATINA_PLUGIN_TYPE(KatinaPluginExample);
KATINA_PLUGIN_INFO("katina::example", "Katina Example", "0.1-dev");

KatinaPluginExample::KatinaPluginExample(Katina& katina)
: KatinaPlugin(katina)
, mapname(katina.get_mapname())
, clients(katina.getClients())
, players(katina.getPlayers())
, teams(katina.getTeams())
, active(true)
{
}

bool KatinaPluginExample::open()
{
	katina.add_var_event(this, "example.active", active);
	//katina.add_var_event(this, "flag", "0");
	katina.add_log_event(this, INIT_GAME);
	katina.add_log_event(this, WARMUP);
	katina.add_log_event(this, CLIENT_CONNECT);
	katina.add_log_event(this, CLIENT_CONNECT_INFO);
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
	katina.add_log_event(this, HEARTBEAT);
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

static siz r = 1;

bool KatinaPluginExample::init_game(siz min, siz sec, const str_map& cvars)
{
	r = 1;
	if(!active)
		return true;
	plog("init_game()");
	plog("mapname: " << mapname);
	for(str_map_citer i = cvars.begin(); i != cvars.end(); ++i)
		plog("cvar: " << i->first << " = " << i->second);
	return true;
}

bool KatinaPluginExample::warmup(siz min, siz sec)
{
	if(!active)
		return true;
	plog("warmup()");
	return true;
}

bool KatinaPluginExample::client_connect(siz min, siz sec, slot num)
{
	if(!active)
		return true;
	plog("client_connect(" << num << ")");
	return true;
}

bool KatinaPluginExample::client_connect_info(siz min, siz sec, slot num, const GUID& guid, const str& ip)
{
	if(!active)
		return true;
	plog("client_connect_info(" << num << ", " << guid << ", " << ip << ")");
	katina.server.chat("CONNECT INFO: " + katina.getPlayerName(num));
	return true;
}

bool KatinaPluginExample::client_begin(siz min, siz sec, slot num)
{
	if(!active)
		return true;
	plog("client_begin(" << num << ")");
	katina.server.chat("BEGIN: " + katina.getPlayerName(num));
	return true;
}

bool KatinaPluginExample::client_disconnect(siz min, siz sec, slot num)
{
	if(!active)
		return true;
	plog("client_disconnect(" << num << ")");
	return true;
}

bool KatinaPluginExample::client_userinfo_changed(siz min, siz sec, slot num, siz team
		, const GUID& guid, const str& name, siz hc)
{
	if(!active)
		return true;
	plog("client_userinfo_changed(" << num << ", " << team << ", " << guid << ", " << name << ")");
	plog("katina.getClientGuid(" << num << "): " << katina.getClientGuid(num));
	plog("katina.getPlayerName(" << num << "): " << katina.getPlayerName(num));
	return true;
}

bool KatinaPluginExample::client_switch_team(siz min, siz sec, slot num, siz teamBefore, siz teamNow)
{
	if(!active)
		return true;
	plog("client_switch_team(" << num << ", " << teamBefore << ", " << teamNow << ")");
	return true;
}

bool KatinaPluginExample::callvote(siz min, siz sec, slot num, const str& type, const str& info)
{
	if(!active)
		return true;
	plog("callvote(" << num << ", " << type << ", " << info << ")");
	return true;
}

bool KatinaPluginExample::kill(siz min, siz sec, slot num1, slot num2, siz weap)
{
	if(!active)
		return true;
	plog("kill(" << num1 << ", " << num2 << ", " << weap << ")");
	return true;
}

bool KatinaPluginExample::ctf(siz min, siz sec, slot num, siz team, siz act)
{
	if(!active)
		return true;
	plog("ctf(" << num << ", " << team << ", " << act << ")");
	return true;
}

bool KatinaPluginExample::ctf_exit(siz min, siz sec, siz r, siz b)
{
	if(!active)
		return true;
	plog("ctf_exit(" << r << ", " << b << ")");
	return true;
}

bool KatinaPluginExample::score_exit(siz min, siz sec, int score, siz ping, slot num, const str& name)
{
	if(!active)
		return true;
	plog("score_exit(" << score << ", " << ping << ", " << num << ", " << name << ")");
	return true;	
}

bool KatinaPluginExample::award(siz min, siz sec, slot num, siz awd)
{
	if(!active)
		return true;
	plog("award(" << num << ", " << awd << ")");
	return true;
}

bool KatinaPluginExample::say(siz min, siz sec, const GUID& guid, const str& text)
{
	if(!active)
		return true;
	plog("say(" << guid << ", " << text << ")");
	str s = text;
	std::reverse(s.begin(), s.end());
	katina.chat_to(guid, s);
	return true;
}

bool KatinaPluginExample::shutdown_game(siz min, siz sec)
{
	if(!active)
		return true;
	plog("shutdown_game()");
	return true;
}

bool KatinaPluginExample::exit(siz min, siz sec)
{
	if(!active)
		return true;
	plog("exit()");
	return true;
}

bool KatinaPluginExample::unknown(siz min, siz sec, const str& cmd, const str& params)
{
	if(!active)
		return true;
	return true;
}

bool KatinaPluginExample::speed(siz min, siz sec, slot num, siz dist, siz time, bool has_flag)
{
	if(!active)
		return true;
	plog("speed(" << num << ", " << dist << ", " << time << ", " << has_flag << ")");
	return true;
}

/**
 * Summarizing events for more detailed statistics (they only work with the katina game mod)
 */
bool KatinaPluginExample::weapon_usage(siz min, siz sec, slot num, siz weapon, siz shots)
{
	if(!active)
		return true;
	plog("weapon_usage(" << num << ", " << weapon << ", " << shots << ")");
	return true;
}

bool KatinaPluginExample::mod_damage(siz min, siz sec, slot num, siz mod, siz hits
		, siz damage, siz hitsRecv, siz damageRecv, float weightedHits)
{
	if(!active)
		return true;
	plog("mod_damage(" << num << ", " << mod << ", " << hits << ", " << damage
			 << ", " << hitsRecv  << ", " << damageRecv  << ", " << weightedHits << ")");
	return true;
}

bool KatinaPluginExample::player_stats(siz min, siz sec, slot num,
	siz fragsFace, siz fragsBack, siz fraggedInFace, siz fraggedInBack,
	siz spawnKills, siz spawnKillsRecv, siz pushes, siz pushesRecv,
	siz healthPickedUp, siz armorPickedUp, siz holyShitFrags, siz holyShitFragged)
{
	if(!active)
		return true;
	plog("player_stats(" << num << ", " << fragsFace << ", " << fragsBack
			<< ", " << fraggedInFace << ", " << fraggedInBack << ", " << spawnKills << ", " << spawnKillsRecv
			<< ", " << pushes << ", " << pushesRecv << ", " << healthPickedUp << ", " << armorPickedUp
			<< ", " << holyShitFrags << ", " << holyShitFragged << ")");
	return true;
}

void KatinaPluginExample::heartbeat(siz min, siz sec)
{
	plog("heartbeat(" << min << ", " << sec << ")");
	++r;
}

siz KatinaPluginExample::get_regularity(siz time_in_secs) const
{
	plog("get_regularity(" << time_in_secs << ")-> " << r);
	return r;
}

void KatinaPluginExample::close()
{
}

}} // katina::plugin
