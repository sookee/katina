#pragma once
#ifndef _OASTATS_REMOTECLIENT_H_
#define _OASTATS_REMOTECLIENT_H_
/*
 * RemoteClient.h
 *
 *  Created on: Apr 7, 2013
 *      Author: oasookee@gmail.com
 */

#include <memory>

#include "types.h"
#include "socketstream.h"
#include "log.h"
#include "irc.h"

namespace oastats { namespace net {

using namespace oastats::irc;
using namespace oastats::log;
using namespace oastats::types;

struct remote_conf
{
	bool active;
	bool do_flags;
	bool do_flags_hud;
	bool do_chats;
	bool do_kills;
	bool do_infos;
	bool do_stats;
	bool spamkill;
	str chans;

	remote_conf()
	: active(false)
	, do_flags(false)
	, do_flags_hud(false)
	, do_chats(false)
	, do_kills(false)
	, do_infos(false)
	, do_stats(false)
	, spamkill(false)
	{
	}
};

typedef std::auto_ptr<class RemoteIRCClient> RemoteIRCClientAPtr;

class RemoteIRCClient
{
public:
	static const str NONE;
	static const str SKIVVY;
	static const str KATINA;
	static const str EGGDROP;

protected:
	bool active;
	bool testing;

	typedef std::map<str, std::set<char> > chan_map;
	typedef chan_map::iterator chan_map_iter;
	typedef chan_map::const_iterator chan_map_citer;

	chan_map chans; // #channel -> {'c','f','k'}

public:
	RemoteIRCClient(): active(false), testing(false) {}
	virtual ~RemoteIRCClient() {}

	void set_testing(bool testing) { this->testing = testing; }

	void on() { active = true; }
	void off() { active = false; }

	void set_chans(const str& chans);
	bool say(char f, const str& text);

	void add_flag(const str& chan, char f) { chans[chan].insert(f); }
	void del_flag(const str& chan, char f) { chans[chan].erase(f); }
	void set_flags(const str& chan, const str& flags)
	{
		for(siz i = 0; i < flags.size(); ++i)
			add_flag(chan, flags[i]);
	}

	void clear_flags()
	{
		for(chan_map_iter chan = chans.begin(); chan != chans.end(); ++chan)
			chan->second.clear();
	}
	void clear_flags(const str& chan) { chans[chan].clear(); }

	bool chat(char f, const str& text) { return say(f, oa_to_IRC(text)); }
	bool raw_chat(char f, const str& text) { return say(f, text); }

	/**
	 * Implementing classes can configure the client from
	 * the supplied properties loaded from the config file.
	 * @param properties
	 * @return false if required properties not found.
	 */
	virtual bool config(const str_map& properties) = 0;

	/**
	 * Implementing classes need to override this function.
	 * @param msg The message being sent.
	 * @param res The response from the remote client.
	 * @return false on communications failure.
	 */
	virtual bool send(const str& channel, const str& msg, str& res) = 0;

	static str_set get_types();
	static RemoteIRCClientAPtr create(const str& type);
};

}} // oastats::net

#endif /* _OASTATS_REMOTECLIENT_H_ */
