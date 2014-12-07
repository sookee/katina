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

#include "WinnerStaysOn.h"

#include <katina/types.h>
#include <katina/log.h>
#include <katina/codes.h>


#include <algorithm>

namespace katina { namespace plugin {

using namespace katina::log;
using namespace katina::types;

KATINA_PLUGIN_TYPE(WinnerStaysOn);
KATINA_PLUGIN_INFO("katina::winner-stays-on", "Winner Stays On", "0.1-dev");

// 1 = red, 2 = blue, 3 = spec
const siz TEAM_R = 1;
const siz TEAM_B = 2;
const siz TEAM_S = 3;

str get_team(siz team) { return team == TEAM_R ? "r" : team == TEAM_B ? "b" : "s"; }

str to_str(siz n)
{
	soss oss;
	oss << n;
	return oss.str();
}

bool WinnerStaysOn::open()
{
	katina.add_log_event(this, KE_INIT_GAME);
	katina.add_log_event(this, KE_WARMUP);
	katina.add_log_event(this, KE_CLIENT_CONNECT);
	katina.add_log_event(this, KE_CLIENT_DISCONNECT);
	katina.add_log_event(this, KE_CLIENT_USERINFO_CHANGED);
	katina.add_log_event(this, KE_CTF);
	katina.add_log_event(this, KE_CTF_EXIT);
	katina.add_log_event(this, KE_SAY);
	katina.add_log_event(this, KE_SHUTDOWN_GAME);
	katina.add_log_event(this, KE_EXIT);
	
	server.chat("^7== ^3Winner Stays On ^1v^7" + str(VERSION) + " ^7==");
	command("set bot_enable 0");
	command("set bot_minplayers 0");

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

void WinnerStaysOn::ensure_teams()
{
	slot_deq_iter i = q.begin();
	
	if(i != q.end() && katina.getTeam(*i) != win_team)
		server.command("!putteam " + str(*i++) + " " + get_team(win_team));

	if(i != q.end() && katina.getTeam(*i) != win_team)
		server.command("!putteam " + str(*i++) + " " + get_team(opp_team));

	while(i != q.end())
		if(katina.getTeam(*i) != TEAM_S)
			server.command("!putteam " + str(*i++) + " s");
}

void WinnerStaysOn::dump_queue()
{
	if(q.empty())
		return;
	
	con("== QUEUE ============================");
	
	slot_deq_iter i = q.begin();
	if(i != q.end())
		con("! " + katina.getPlayerName(*i++));
	if(i != q.end())
		con("? " + katina.getPlayerName(*i++));
	for(siz pos = 1; i != q.end(); ++pos)
		con(to_str(pos) + " " + katina.getPlayerName(*i++));
}

void WinnerStaysOn::announce_queue()
{
	if(q.empty())
		return;
	
	server.chat("^1== ^3QUEUE ^1============================");
	
	slot_deq_iter i = q.begin();
	if(i != q.end())
		con("^3! " + katina.getPlayerName(*i++));
	if(i != q.end())
		con("^3? " + katina.getPlayerName(*i++));
	for(siz pos = 1; i != q.end(); ++pos)
		con("^3" + to_str(pos) + " ^7" + katina.getPlayerName(*i++));
}

bool WinnerStaysOn::command(const str& cmd)
{
	if(!server.command(cmd))
		if(!server.command(cmd))
			return server.command(cmd); // two retry
	return true;
}

bool WinnerStaysOn::vote_enable()
{
	server.cp("Voting on");
	return command("set g_allowVote 1");
}

bool WinnerStaysOn::vote_disable()
{
	server.cp("Voting off");
	return command("set g_allowVote 0");
}

bool WinnerStaysOn::lock_teams()
{
	return command("!lock r") && command("!lock b");
}

bool WinnerStaysOn::unlock_teams()
{
	return command("!unlock r") && command("!unlock b");
}

bool WinnerStaysOn::warmup(siz min, siz sec)
{

	vote_enable();
	server.chat("^7== ^3Winner Stays On ^1v^7" + str(VERSION) + " ^7==");
	server.cp("^7== ^3Winner Stays On ^1v^7" + str(VERSION) + " ^7==");

	lock_teams();
	
	ensure_teams();
	announce_queue();
	
	server.cp("Please vote for your map now");

	return true;
}

bool WinnerStaysOn::client_connect(siz min, siz sec, slot num)
{
	slot_deq_iter i = std::find(q.begin(), q.end(), num);
	if(i != q.end())
		q.erase(i);
	q.push_back(num);
	
	katina.chat_to(num, "You have been moved to the back of the queue.");
	katina.chat_to(num, "There are " + to_str(q.size() - 1) + " people in front of you.");
		
	return true;
}

bool WinnerStaysOn::client_disconnect(siz min, siz sec, slot num)
{
	slot_deq_iter i = q.begin();
	
	if(i != q.end() && *i++ == num)
	{
		server.chat("^3 The defender has left, starting a new game.");
		server.command("!restart");
	}
	
	if(i != q.end() && *i++ == num)
	{
		server.chat("^3 The challenger has left, starting a new game.");
		server.command("!restart");
	}

	for(siz pos = 1; i != q.end(); ++pos)
	{
		if(*i++ != num)
			continue;
		server.chat("^3Player ^7" + to_str(pos) + "^3: ^7" + katina.getPlayerName(num) + " ^3has left.");
		q.erase(i);
		ensure_teams();
		announce_queue();
		break;
	}

	return true;
}

bool WinnerStaysOn::client_userinfo_changed(siz min, siz sec, slot num, siz team, const GUID& guid, const str& name, siz hc)
{
	slot_deq_iter i = std::find(q.begin(), q.end(), num);
	if(i == q.end())
	{
		q.push_back(num);
		katina.chat_to(num, "You have been moved to the back of the queue.");
		katina.chat_to(num, "There are " + to_str(q.size() - 1) + " people in front of you.");
	}
	
	return true;
}

bool WinnerStaysOn::ctf(siz min, siz sec, slot num, siz team, siz act)
{
	return true;
}

bool WinnerStaysOn::ctf_exit(siz min, siz sec, siz r, siz b)
{
	siz team = r > b ? TEAM_R : (b > r ? TEAM_B: TEAM_S);
	
	slot_deq_iter i = q.begin();

	if(team == TEAM_S || win_team == team)
	{
		if(i != q.end())
		{
			server.cp("^7" + katina.getPlayerName(*i) + " ^3wins!");
			server.chat("^3The defender: ^7" + katina.getPlayerName(*i) + " ^3stays on!");
		}
		
		if(++i != q.end())
		{
			server.chat("^3The challenger: ^7" + katina.getPlayerName(*i) + " ^3goes to the back of the queue.");
			slot num = *i;
			q.erase(i);
			q.push_back(num);
		}
	}
	else
	{
		if(i != q.end())
		{
			server.chat("^3The defender: ^7" + katina.getPlayerName(*i) + " ^3goes to the back of the queue!");
			slot num = *i;
			q.erase(i);
			q.push_back(num);
		}
		
		if(++i != q.end())
		{
			server.cp("^7" + katina.getPlayerName(*i) + " ^3wins!");
			server.chat("^3The winner: ^7" + katina.getPlayerName(*i) + " ^3stays on!");
		}
	}

	win_team = win_team == TEAM_R ? TEAM_B : TEAM_R;
	return true;
}

bool WinnerStaysOn::init_game(siz min, siz sec, const str_map& svars)
{
	lock_teams();

	vote_disable();	
	command("set g_doWarmup 1");
	command("set g_warmup " + katina.get("wso.warmup", "30"));
	
	ensure_teams();
	announce_queue();

	return true;
}

bool WinnerStaysOn::say(siz min, siz sec, const GUID& guid, const str& text)
{
	return true;
}

bool WinnerStaysOn::exit(siz min, siz sec)
{
	return true;
}

bool WinnerStaysOn::shutdown_game(siz min, siz sec)
{
	return true;
}

void WinnerStaysOn::close()
{
	unlock_teams();
	command("set bot_enable 1");
	command("set bot_minplayers 1");
}

}} // katina::plugin
