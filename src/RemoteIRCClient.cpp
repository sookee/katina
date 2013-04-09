/*
 * RemoteClient.cpp
 *
 *  Created on: Apr 7, 2013
 *      Author: oasookee@gmail.com
 */

#include "RemoteIRCClient.h"

namespace oastats { namespace net {

// TODO: This should be configurable
const str irc_katina = "04K00at08i00na";

class NullClient
: public RemoteIRCClient
{
public:

	virtual bool send(const str& cmd, str& res) { return true; }
};

class SkivvyClient
: public RemoteIRCClient
{
	net::socketstream ss;

public:

	// RemoteClient Interface

	virtual bool send(const str& cmd, str& res)
	{
		if(!active)
			return true;

		if(!ss.open(host, port))
		{
			log("error: " << std::strerror(errno));
			return false;
		}
		(ss << cmd).put('\0') << std::flush;
		return std::getline(ss, res, '\0');
	}
};

void RemoteIRCClient::set_chans(const str& chans)
{
	bug("set_chans(): " << chans);
	this->chans.clear();
	str chan;
	siss iss(chans);
	while(iss >> chan) // #channel(flags)
	{
		bug("chan: " << chan);
		str flags;
		siss iss(chan);
		std::getline(iss, chan, '(');
		if(std::getline(iss, flags, ')'))
		{
			// config flags c = chats f = flags k = kills
			set_flags(chan, flags);
		}
	}
}

bool RemoteIRCClient::say(char f, const str& text)
{
	if(!active)
		return true; // not error

	str res;
	bool good = true;

	for(chan_map_iter chan = chans.begin(); chan != chans.end(); ++chan)
		if(f == '*' || chan->second.count(f))
			good = good && send("/say " + chan->first + " [" + irc_katina + "] " + text, res);

	return good;
}

const str NULL_CLIENT = "null";
const str SKIVVY_CLIENT = "skivvy";
const str KATINA_CLIENT = "katina";
const str EGGDROP_CLIENT = "eggdrop";

str_set RemoteIRCClient::get_types()
{
	str_set types;
	types.insert(NULL_CLIENT);
	types.insert(SKIVVY_CLIENT);
	return types;
}

RemoteIRCClient* create(const str& type)
{
	if(type == NULL_CLIENT)
		return new NullClient();
	else if(type == SKIVVY_CLIENT)
		return new SkivvyClient();

	log("Unknown RemoteClient: " << type << " disabling feature.");

	return new NullClient();
}

}} // oastats::net
