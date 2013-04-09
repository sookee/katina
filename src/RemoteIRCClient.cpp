/*
 * RemoteClient.cpp
 *
 *  Created on: Apr 7, 2013
 *      Author: oasookee@gmail.com
 */

#include "RemoteIRCClient.h"
#include "str.h"

namespace oastats { namespace net {

using namespace oastats::string;

// TODO: This should be configurable
const str irc_katina = "04K00at08i00na";

class NullClient
: public RemoteIRCClient
{
public:

	virtual bool config(const str_map& properties) { return true; }
	virtual bool send(const str& cmd, str& res) { return true; }
};

class SkivvyClient
: public RemoteIRCClient
{
	static const str PROP_HOST;
	static const str PROP_PORT;
	static const str PROP_USER;
	static const str PROP_PASS;

	str host;
	siz port;
	net::socketstream ss;

public:

	// RemoteClient Interface

	virtual bool config(const str_map& properties)
	{
		if(!properties.count(PROP_HOST))
			return log_error("SKIVVY CLIENT ERROR: missing property: " + PROP_HOST);
		if(!properties.count(PROP_PORT))
			return log_error("SKIVVY CLIENT ERROR: missing property: " + PROP_PORT);

		host = properties.at(PROP_HOST);
		port = to<siz>(properties.at(PROP_PORT));
	}

	virtual bool send(const str& cmd, str& res)
	{
		if(!active)
			return true;

		if(!ss.open(host, port))
			return log_errno("SKIVVY CLIENT ERROR");

		(ss << cmd).put('\0') << std::flush;
		return std::getline(ss, res, '\0');
	}
};

const str SkivvyClient::PROP_HOST = "skivvy.host";
const str SkivvyClient::PROP_PORT = "skivvy.port";
const str SkivvyClient::PROP_USER = "skivvy.user";
const str SkivvyClient::PROP_PASS = "skivvy.pass";

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

const str RemoteIRCClient::NONE = "none";
const str RemoteIRCClient::SKIVVY = "skivvy";
const str RemoteIRCClient::KATINA = "katina";
const str RemoteIRCClient::EGGDROP = "eggdrop";

str_set RemoteIRCClient::get_types()
{
	str_set types;
	types.insert(NONE);
	types.insert(SKIVVY);
	return types;
}

RemoteIRCClient* RemoteIRCClient::create(const str& type)
{
	if(type == NONE)
		return new NullClient();
	else if(type == SKIVVY)
		return new SkivvyClient();

	log("Unknown RemoteClient: " << type << " disabling feature.");

	return new NullClient();
}

}} // oastats::net
