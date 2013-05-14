
#include "WinnerStaysOn.h"

#include <katina/types.h>
#include <katina/log.h>
#include <katina/codes.h>


#include <algorithm>

namespace katina { namespace plugin {

using namespace oastats::log;
using namespace oastats::types;

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
	katina.add_log_event(this, INIT_GAME);
	katina.add_log_event(this, WARMUP);
	katina.add_log_event(this, CLIENT_CONNECT);
	katina.add_log_event(this, CLIENT_DISCONNECT);
	katina.add_log_event(this, CLIENT_USERINFO_CHANGED);
	katina.add_log_event(this, CTF);
	katina.add_log_event(this, CTF_EXIT);
	katina.add_log_event(this, SAY);
	katina.add_log_event(this, SHUTDOWN_GAME);
	katina.add_log_event(this, EXIT);
	
	server.chat("^7== ^3Winner Stays On ^1v^7" + str(VERSION) + " ^7==");

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
	siz_deq_iter i = q.begin();
	
	if(i != q.end() && teams[clients[*i]] != win_team)
		server.command("!putteam " + to_str(*i++) + " " + get_team(win_team));

	if(i != q.end() && teams[clients[*i]] != win_team)
		server.command("!putteam " + to_str(*i++) + " " + get_team(opp_team));

	while(i != q.end())
		if(teams[clients[*i]] != TEAM_S)
			server.command("!putteam " + to_str(*i++) + " s");	
}

void WinnerStaysOn::dump_queue()
{
	if(q.empty())
		return;
	
	con("== QUEUE ============================");
	
	siz_deq_iter i = q.begin();
	if(i != q.end())
		con("! " + players[clients[*i++]]);
	if(i != q.end())
		con("? " + players[clients[*i++]]);
	for(siz pos = 1; i != q.end(); ++pos)
		con(to_str(pos) + " " + players[clients[*i++]]);
}

void WinnerStaysOn::announce_queue()
{
	if(q.empty())
		return;
	
	server.chat("^1== ^3QUEUE ^1============================");
	
	siz_deq_iter i = q.begin();
	if(i != q.end())
		con("^3! " + players[clients[*i++]]);
	if(i != q.end())
		con("^3? " + players[clients[*i++]]);
	for(siz pos = 1; i != q.end(); ++pos)
		con("^3" + to_str(pos) + " ^7" + players[clients[*i++]]);
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
	command("!lock r");
	command("!lock b");
}

bool WinnerStaysOn::unlock_teams()
{
	command("!unlock r");
	command("!unlock b");
}

bool WinnerStaysOn::warmup(siz min, siz sec)
{
	dump_queue();

	vote_enable();
	server.chat("^7== ^3Winner Stays On ^1v^7" + str(VERSION) + " ^7==");
	server.cp("^7== ^3Winner Stays On ^1v^7" + str(VERSION) + " ^7==");

	lock_teams();
	
	ensure_teams();
	announce_queue();
	
	server.cp("Please vote for your map now");

	dump_queue();
	return true;
}

bool WinnerStaysOn::client_connect(siz min, siz sec, siz num)
{
	dump_queue();
	siz_deq_iter i = std::find(q.begin(), q.end(), num);
	if(i != q.end())
		q.erase(i);
	q.push_back(num);
	
	katina.chat_to(num, "You have been moved to the back of the queue.");
	katina.chat_to(num, "There are " + to_str(q.size() - 1) + " people in front of you.");
		
	dump_queue();
	return true;
}

bool WinnerStaysOn::client_disconnect(siz min, siz sec, siz num)
{
	dump_queue();
	siz_deq_iter i = q.begin();
	
	if(i != q.end() && *i++ == num)
	{
		server.chat("The defender has left, starting a new game.");
		server.command("!restart");
	}
	
	if(i != q.end() && *i++ == num)
	{
		server.chat("The challenger has left, starting a new game.");
		server.command("!restart");
	}

	while(i != q.end())
	{
		if(*i++ != num)
			continue;
		q.erase(i);
		ensure_teams();
		announce_queue();
		break;
	}

	dump_queue();
	return true;
}

bool WinnerStaysOn::client_userinfo_changed(siz min, siz sec, siz num, siz team, const GUID& guid, const str& name)
{
	dump_queue();
	siz_deq_iter i = std::find(q.begin(), q.end(), num);
	if(i == q.end())
	{
		q.push_back(num);
		katina.chat_to(num, "You have been moved to the back of the queue.");
		katina.chat_to(num, "There are " + to_str(q.size() - 1) + " people in front of you.");
	}
	dump_queue();
	
	return true;
}

bool WinnerStaysOn::ctf(siz min, siz sec, siz num, siz team, siz act)
{
	return true;
}

bool WinnerStaysOn::ctf_exit(siz min, siz sec, siz r, siz b)
{
	dump_queue();
	siz team = r > b ? TEAM_R : (b > r ? TEAM_B: TEAM_S);
	
	siz_deq_iter i = q.begin();

	if(team == TEAM_S || win_team == team)
	{
		if(i != q.end())
		{
			server.cp("^7" + players[clients[*i]] + " ^3wins!");
			server.chat("^3The defender: ^7" + players[clients[*i]] + " ^3stays on!");
		}
		
		if(++i != q.end())
		{
			server.chat("^3The challenger: ^7" + players[clients[*i]] + " ^3goes to the back of the queue.");
			siz num = *i;
			q.erase(i);
			q.push_back(*i);
		}
	}
	else
	{
		if(i != q.end())
		{
			server.chat("^3The defender: ^7" + players[clients[*i]] + " ^3goes to the back of the queue!");
			siz num = *i;
			q.erase(i);
			q.push_back(*i);
		}
		
		if(++i != q.end())
		{
			server.cp("^7" + players[clients[*i]] + " ^3wins!");
			server.chat("^3The winner: ^7" + players[clients[*i]] + " ^3stays on!");
		}
	}

	win_team = win_team == TEAM_R ? TEAM_B : TEAM_R;
	dump_queue();
	return true;
}

bool WinnerStaysOn::init_game(siz min, siz sec)
{
	dump_queue();
	lock_teams();

	vote_disable();	
	command("set g_doWarmup 1");
	command("set g_warmup " + katina.get("wso.warmup", "30"));
	
	ensure_teams();
	announce_queue();

	dump_queue();
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
	dump_queue();
	return true;
}

void WinnerStaysOn::close()
{
	unlock_teams();
}

}} // katina::plugin
