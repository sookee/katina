/*
 * File:   KatinaPluginCallVoteCtrl.cpp
 * Author: SooKee oasookee@gmail.com
 *
 * Created on May 18, 2013
 */

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
	if(!plugin)
		log("ERROR: votectrl plugin not set");
	else
		plugin->vote_enable();
	signal(sig, SIG_IGN);
}

KatinaPluginCallVoteCtrl::KatinaPluginCallVoteCtrl(Katina& katina)
: KatinaPlugin(katina)
, server(katina.server)
, active(false)
, wait(0)
, restart_vote(0)
{
}

bool KatinaPluginCallVoteCtrl::open()
{
	katina.add_var_event(this, "votectrl.active", active, false);
	katina.add_var_event(this, "votectrl.wait", wait, std::time_t(15));

	katina.add_log_event(this, INIT_GAME);
	katina.add_log_event(this, EXIT);
	katina.add_log_event(this, SHUTDOWN_GAME);

	plugin = this;
	
	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = handler;
	sigemptyset(&sa.sa_mask);
	if(sigaction(SIG, &sa, NULL) == -1)
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
	if (timer_create(CLOCKID, &sev, &timerid) == -1)
	{
		plog("FATAL: failed to create timer");
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
	plog("CALLVOTE CONTROL: ON");
	server.cp("Voting on");
	return command("set g_allowVote 1");
}

bool KatinaPluginCallVoteCtrl::vote_disable()
{
	plog("CALLVOTE CONTROL: OFF");
	server.cp("Voting off");
	return command("set g_allowVote 0");
}


bool KatinaPluginCallVoteCtrl::init_game(siz min, siz sec, const str_map& cvars)
{
	if(!active)
		return true;
	
	/* Start the timer */
	
	its.it_value.tv_sec = wait;
	its.it_value.tv_nsec = 0;
	its.it_interval.tv_sec = 0;
	its.it_interval.tv_nsec = 0;
	
	if(timer_settime(timerid, 0, &its, NULL) == -1)
	{
		plog("ERROR: setting timer, renabling voting");
		vote_enable();
	}
	
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