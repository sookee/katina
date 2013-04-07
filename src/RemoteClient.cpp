/*
 * RemoteClient.cpp
 *
 *  Created on: Apr 7, 2013
 *      Author: oasookee@gmail.com
 */

#include "RemoteClient.h"

namespace oastats { namespace net {

// TODO: This should be configurable
const str irc_katina = "04K00at08i00na";

void RemoteClient::set_chans(const str& chans)
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

bool RemoteClient::say(char f, const str& text)
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


}} // oastats::net
