
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
	katina.add_log_event(this, EXIT);
	katina.add_log_event(this, SHUTDOWN_GAME);
	katina.add_log_event(this, WARMUP);
//	katina.add_log_event(this, CLIENT_USERINFO_CHANGED);
	katina.add_log_event(this, CLIENT_CONNECT);
	katina.add_log_event(this, CLIENT_DISCONNECT);
	katina.add_log_event(this, CTF);
	katina.add_log_event(this, CTF_EXIT);
	katina.add_log_event(this, INIT_GAME);
	katina.add_log_event(this, SAY);
	
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
	siz_deq_riter i = q.rbegin();
	
	if(i != q.rend() && teams[clients[*i]] != win_team)
		server.command("!putteam " + to_str(*i++) + " " + get_team(win_team));

	if(i != q.rend() && teams[clients[*i]] != win_team)
		server.command("!putteam " + to_str(*i++) + " " + get_team(opp_team));

	while(i != q.rend())
		if(teams[clients[*i]] != TEAM_S)
			server.command("!putteam " + to_str(*i++) + " s");	
}

void WinnerStaysOn::announce_queue()
{
	if(q.empty())
		return;
	
	server.chat("^1== ^3QUEUE ^1============================");
	
	siz pos = 0;
	for(siz_deq_iter i = q.begin(); i != q.end(); ++i, ++pos)
		server.chat("^2#" + to_str(pos) + " ^7" + players[clients[*i]]);
}

bool WinnerStaysOn::vote_enable()
{
	server.cp("Voting on");
	if(!server.command("set g_allowVote 1"))
		if(!server.command("set g_allowVote 1"))
			return server.command("set g_allowVote 1"); // two retry
	return true;
}

bool WinnerStaysOn::vote_disable()
{
	server.cp("Voting off");
	if(!server.command("set g_allowVote 0"))
		if(!server.command("set g_allowVote 0"))
			return server.command("set g_allowVote 0"); // two retry
	return true;
}

// Warmup = time to vote for map to play

bool WinnerStaysOn::warmup(siz min, siz sec)
{
	server.chat("^7== ^3Winner Stays On ^1v^7" + str(VERSION) + " ^7==");
	server.cp("^7== ^3Winner Stays On ^1v^7" + str(VERSION) + " ^7==");

	server.command("!lock r");
	server.command("!lock b");
	
	vote_enable();	
	ensure_teams();
	announce_queue();
	
	server.cp("Please vote for your map now");

	return true;
}

bool WinnerStaysOn::client_connect(siz min, siz sec, siz num)
{
	siz_deq_iter i = std::find(q.begin(), q.end(), num);
	if(i != q.end())
		q.erase(i);
	q.push_back(num);
	
	katina.chat_to(num, "You have been moved to the back of the queue.");
	katina.chat_to(num, "There are " + to_str(q.size() - 1) + " people in front of you.");
		
	return true;
}

bool WinnerStaysOn::client_disconnect(siz min, siz sec, siz num)
{
	siz_deq_riter i = q.rbegin();
	
	if(i != q.rend() && *i++ == num)
		server.command("!restart");
	
	if(i != q.rend() && *i++ == num)
		server.command("!restart");

	while(i != q.rend())
	{
		if(*i++ != num)
			continue;
		q.erase(i.base());
		ensure_teams();
		announce_queue();
		break;
	}

	return true;
}

bool WinnerStaysOn::ctf(siz min, siz sec, siz num, siz team, siz act)
{
	return true;
}

bool WinnerStaysOn::ctf_exit(siz min, siz sec, siz r, siz b)
{
	siz team = r > b ? TEAM_R: b > r ? TEAM_B: TEAM_S;
	
	siz_deq_riter i = q.rbegin();

	if(team == TEAM_S || win_team == team)
	{
		if(i != q.rend())
		{
			server.cp("^7" + players[clients[*i]] + " ^3wins!");
			server.chat("^3The defender: ^7" + players[clients[*i]] + " ^3stays on!");
		}
		
		if(++i != q.rend())
		{
			server.chat("^3The challenger: ^7" + players[clients[*i]] + " ^3goes to the back of the queue.");
			siz num = *i;
			q.erase(i.base());
			q.push_back(*i);
		}
	}
	else
	{
		if(i != q.rend())
		{
			server.chat("^3The defender: ^7" + players[clients[*i]] + " ^3goes to the back of the queue!");
			siz num = *i;
			q.erase(i.base());
			q.push_back(*i);
		}
		
		if(++i != q.rend())
		{
			server.cp("^7" + players[clients[*i]] + " ^3wins!");
			server.chat("^3The winner: ^7" + players[clients[*i]] + " ^3stays on!");
		}
	}

	return true;
}

bool WinnerStaysOn::init_game(siz min, siz sec)
{
	server.command("!lock r");
	server.command("!lock b");

	vote_disable();	
	server.command("set g_doWarmup 1");
	server.command("set g_warmup " + katina.get("wso.warmup", "30"));
	
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
	// game finished properly
	return true;
}

bool WinnerStaysOn::shutdown_game(siz min, siz sec)
{
	log("shutdown_game()");
	return true;
}

void WinnerStaysOn::close()
{
	server.command("!unlock r");
	server.command("!unlock b");
}

}} // katina::plugin
