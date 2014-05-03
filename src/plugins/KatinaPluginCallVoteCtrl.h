/*
 * File:   KatinaPluginCallVoteCtrl.h
 * Author: SooKee oasookee@gmail.com
 *
 * Created on May 18, 2013
 */

#ifndef KATINA_PLUGIN_VOTES_H
#define	KATINA_PLUGIN_VOTES_H

#include <map>
#include <utility>
#include <signal.h>

#include "KatinaPluginStats.h"

#include <katina/types.h>
#include <katina/log.h>

namespace katina { namespace plugin {

using namespace oastats::log;
using namespace oastats::types;

class KatinaPluginCallVoteCtrl
: public KatinaPlugin
{
	friend void handler(int sig, siginfo_t* si, void* uc);
private:
	RCon& server;

	// cvars
	bool active;
	time_t wait;
	time_t restart_vote;
	bool votes_disabled;
	
	bool command(const str& cmd);
	bool vote_enable();
	bool vote_disable();
	
public:
	KatinaPluginCallVoteCtrl(Katina& katina);

	// API
	
	// INTERFACE: KatinaPlugin

	virtual bool open();

	virtual str get_id() const;
	virtual str get_name() const;
	virtual str get_version() const;

	virtual bool init_game(siz min, siz sec, const str_map& cvars);
	virtual bool shutdown_game(siz min, siz sec);
	virtual bool say(siz min, siz sec, const GUID& guid, const str& text);
	virtual bool exit(siz min, siz sec);

	virtual void close();
};

}} // katina::plugin

#endif	// KATINA_PLUGIN_VOTES_H

