//#pragma once
#ifndef _OASTATS_REMOTECLIENT_H_
#define _OASTATS_REMOTECLIENT_H_
/*
 * RemoteClient.h
 *
 *  Created on: Apr 7, 2013
 *      Author: oasookee@gmail.com
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

#include "types.h"
#include "socketstream.h"
#include "log.h"
#include "irc.h"

namespace katina {

class Katina;
	
namespace net {

using namespace katina::irc;
using namespace katina::log;
using namespace katina::types;

class RemoteClient
{
protected:
	class Katina& katina;

	bool active;

	typedef std::map<str, std::set<char> > chan_map;
	typedef chan_map::iterator chan_map_iter;
	typedef chan_map::const_iterator chan_map_citer;
	typedef chan_map::value_type chan_map_vt;

	chan_map chans; // #channel -> {'c','f','k'}

public:
	RemoteClient(Katina& katina): katina(katina), active(false) {}
	virtual ~RemoteClient() {}

	void on() { active = true; }
	void off() { active = false; }

	void set_chans(const str& chans);
	bool say(char f, const str& text);

	str_vec get_chans_vec() const
	{
		str_vec v;
		for(const chan_map_vt& c: chans)
			v.push_back(c.first);
		return v;
	}

	void add_flag(const str& chan, char f) { chans[chan].insert(f); }
	void del_flag(const str& chan, char f) { chans[chan].erase(f); }
	void set_flags(const str& chan, const str& flags)
	{
		// bug_func();
		// bug_var(chan);
		// bug_var(flags);
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
	
	bool send(const str& msg)
	{
		str res;
		return send(msg, res);
	}

	static RemoteClient* create(Katina& katina, const str& config);
};

class PkiClient
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
	PkiClient(Katina& katina): RemoteClient(katina), connected(false), port(0) {}
	// RemoteClient Interface

	virtual bool configure(const str& params);
	virtual bool send(const str& cmd, str& res);
};

class InsecureClient
: public RemoteClient
{
	net::socketstream ss;
	str host;
	siz port = 7334;

public:
	InsecureClient(Katina& katina): RemoteClient(katina) {}

	// RemoteClient Interface

	virtual bool configure(const str& params);

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

class FileClient
: public RemoteClient
{
	str ofile;
	str ifile;
	std::ofstream ofs;
	std::ifstream ifs;
	
	str IRC_to_plain(const str& s)
	{
		return s;
		str p;
		for(siz i = 0; i < s.size(); ++i)
			if(std::isprint(s[i]))
				p += s[i];
			else
				i += 2;
		return p;
	}
	
public:
	FileClient(Katina& katina): RemoteClient(katina) {}

	// RemoteClient Interface

	virtual bool configure(const str& params);

	virtual bool send(const str& cmd, str& res)
	{
		if(!active)
			return true;

		ofs << IRC_to_plain(cmd) << std::endl;
		return ifs ? std::getline(ifs, res) : "";
	}
};

}} // katina::net

#endif /* _OASTATS_REMOTECLIENT_H_ */
