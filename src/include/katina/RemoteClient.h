#pragma once
#ifndef _OASTATS_REMOTECLIENT_H_
#define _OASTATS_REMOTECLIENT_H_
/*
 * RemoteClient.h
 *
 *  Created on: Apr 7, 2013
 *      Author: oasookee@gmail.com
 */

#include "types.h"
#include "socketstream.h"
#include "log.h"
#include "irc.h"

namespace oastats {

class Katina;
	
namespace net {

using namespace oastats::irc;
using namespace oastats::log;
using namespace oastats::types;

class RemoteClient
{
protected:
	class Katina& katina;

	bool active;

	typedef std::map<str, std::set<char> > chan_map;
	typedef chan_map::iterator chan_map_iter;
	typedef chan_map::const_iterator chan_map_citer;

	chan_map chans; // #channel -> {'c','f','k'}

public:
	RemoteClient(Katina& katina): katina(katina), active(false) {}
	virtual ~RemoteClient() {}

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

	virtual bool configure(const str& params) { return true; }

	/**
	 * Implementing classes need to override this function.
	 * @param msg The message being sent.
	 * @param res The response from the remote client.
	 * @return false on communications failure.
	 */
	virtual bool send(const str& msg, str& res) = 0;
	
	static RemoteClient* create(Katina& katina, const str& config);
};

class PKIClient
: public RemoteClient
{
	net::socketstream ss;
	bool connected;
/*	
	struct session
	{
		str key;
		str sig;
		session() {}
		session(str key, str sig): key(key), sig(sig) {}
		session(const session& s): key(s.key), sig(s.sig) {}
	};
	
	typedef std::map<str, session> session_map;
	typedef session_map::iterator session_map_iter;
	typedef session_map::const_iterator session_map_citer;
*/
	str host;
	siz port;
	str key, sig;
	
	str ref; // session ref

	//session_map sessions;
	
public:
	PKIClient(Katina& katina): RemoteClient(katina), connected(false), port(0) {}
	// RemoteClient Interface

	virtual bool configure(const str& params);
	virtual bool send(const str& cmd, str& res);
};

}} // oastats::net

#endif /* _OASTATS_REMOTECLIENT_H_ */
