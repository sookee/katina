/*
 * File:   KatinaPluginCallVoteCtrl.cpp
 * Author: SooKee oasookee@gmail.com
 *
 * Created on May 18, 2013
 */

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

#include "KatinaPluginCallVoteCtrl.h"

#include <katina/KatinaPlugin.h>

#include <katina/types.h>
#include <katina/log.h>

#include <cstdlib>
#include <unistd.h>
#include <signal.h>
#include <ctime>

#define CLOCKID CLOCK_REALTIME
#define SIG SIGRTMIN

namespace katina { namespace plugin {

using namespace oastats::log;
using namespace oastats::types;

KATINA_PLUGIN_TYPE(KatinaPluginCallVoteCtrl);
KATINA_PLUGIN_INFO("katina::callvotectrl", "Katina CallVote Control", "0.1");

timer_t timerid;
sigevent sev;
itimerspec its;
long long freq_nanosecs;
sigset_t mask;
struct sigaction sa;

static KatinaPluginCallVoteCtrl* plugin = 0;

void handler(int sig, siginfo_t* si, void* uc)
{
	log("INFO: Signal handler invoked");
	if(!plugin)
		log("ERROR: votectrl plugin not set");
	else
		plugin->vote_enable();
	signal(sig, SIG_IGN);
	
	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = handler;
	sigemptyset(&sa.sa_mask);
	if(sigaction(SIG, &sa, 0))
		log("ERROR: failed to set signal handler");
}

KatinaPluginCallVoteCtrl::KatinaPluginCallVoteCtrl(Katina& katina)
: KatinaPlugin(katina)
, server(katina.server)
, active(false)
, wait(0)
, restart_vote(0)
, votes_disabled(false)
{
}

bool KatinaPluginCallVoteCtrl::open()
{
	katina.add_var_event(this, "callvotectrl.active", active, false);
	katina.add_var_event(this, "callvotectrl.wait", wait, std::time_t(15));

	katina.add_log_event(this, INIT_GAME);
	katina.add_log_event(this, SAY);
	katina.add_log_event(this, EXIT);
	katina.add_log_event(this, SHUTDOWN_GAME);

	plugin = this;
	
	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = handler;
	sigemptyset(&sa.sa_mask);
	if(sigaction(SIG, &sa, 0))
	{
		plog("FATAL: failed to set signal handler");
		return false;
	}
	
	/* Block timer signal temporarily */
	
	sigemptyset(&mask);
	sigaddset(&mask, SIG);
	if(sigprocmask(SIG_SETMASK, &mask, NULL) == -1)
	{
		plog("FATAL: failed to mask signal");
		return false;
	}
	
	/* Create the timer */
	
	sev.sigev_notify = SIGEV_SIGNAL;
	sev.sigev_signo = SIG;
	sev.sigev_value.sival_ptr = &timerid;
	
	if(timer_create(CLOCKID, &sev, &timerid) == -1)
	{
		plog("FATAL: failed to create timer");
		return false;
	}
	
	if(sigprocmask(SIG_UNBLOCK, &mask, NULL) == -1)
	{
		plog("FATAL: failed to unmask signal");
		return false;
	}
	
	return true;
}

str KatinaPluginCallVoteCtrl::get_id() const
{
	return ID;
}

str KatinaPluginCallVoteCtrl::get_name() const
{
	return NAME;
}

str KatinaPluginCallVoteCtrl::get_version() const
{
	return VERSION;
}

bool KatinaPluginCallVoteCtrl::command(const str& cmd)
{
	if(!server.command(cmd))
		if(!server.command(cmd))
			return server.command(cmd); // two retry
	return true;
}

bool KatinaPluginCallVoteCtrl::vote_enable()
{
	if(!votes_disabled)
		return true;
	plog("CALLVOTE CONTROL: ON");
	server.cp("Voting on");
	return (votes_disabled = !command("set g_allowVote 1"));
}

bool KatinaPluginCallVoteCtrl::vote_disable()
{
	if(votes_disabled)
		return true;
	plog("CALLVOTE CONTROL: OFF");
	server.cp("Voting off");
	return (votes_disabled = command("set g_allowVote 0"));
}


bool KatinaPluginCallVoteCtrl::init_game(siz min, siz sec, const str_map& cvars)
{
	if(!active)
		return true;
	
	// Start the timer
	
	its.it_value.tv_sec = wait;
	its.it_value.tv_nsec = 0;
	its.it_interval.tv_sec = 0;
	its.it_interval.tv_nsec = 0;
	
	if(timer_settime(timerid, 0, &its, NULL) != -1)
		log("CALLVOTE CONTROL: TIMED: " << wait << " secs");
	else
	{
		plog("ERROR: setting timer, renabling voting");
		vote_enable();
	}
	
	return true;
}

bool KatinaPluginCallVoteCtrl::say(siz min, siz sec, const GUID& guid, const str& text)
{
	siss iss(text);
	str cmd, param;
	if(!(iss >> cmd >> param) || cmd.empty() || cmd[0] != '!')
		return true;

	bug("SAY: " << cmd << ' ' << param);

	//	!callvote on|off|enable|disable
	if(cmd != "!callvote")
		return true;

	if(!katina.is_admin(guid))
	{
		plog("INFO: Unauthorized admin attempt from [" << guid << "] " << katina.players[guid] << ": " << text);
		return true;
	}

	if(param == "on")
		vote_enable();
	else if(param == "off")
		vote_disable();
	else if(param == "enable")
		active = true;
	else if(param == "disable")
		active = false;
	else
		plog("WARN: Unknown !callvote parameter: " << param);

	return true;
}

bool KatinaPluginCallVoteCtrl::exit(siz min, siz sec)
{
	if(!active)
		return true;

	vote_disable();

	return true;
}

bool KatinaPluginCallVoteCtrl::shutdown_game(siz min, siz sec)
{
	if(!active)
		return true;

	vote_disable();

	return true;
}


void KatinaPluginCallVoteCtrl::close()
{

}

}} // katina::plugin
