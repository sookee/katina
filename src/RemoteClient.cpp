/*
 * RemoteClient.cpp
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

#include <katina/RemoteClient.h>

#include <katina/Katina.h>
#include <katina/utils.h>

namespace katina { namespace net {

using namespace katina;
using namespace katina::utils;
	
// TODO: This should be configurable
const str irc_katina = "04K00at08i00na";

void RemoteClient::set_chans(const str& chans)
{
	this->chans.clear();
	str chan;
	siss iss(chans);
	while(iss >> chan) // #channel(flags)
	{
		str flags;
		siss iss(chan);
		std::getline(iss, chan, '(');
		if(std::getline(iss, flags, ')'))
			set_flags(chan, flags);
	}
}

bool RemoteClient::say(char f, const str& text)
{
	if(!active)
		return true; // not error

	str res;
	bool good = true;

//	bug_var(chans.size());
	for(chan_map_iter chan = chans.begin(); chan != chans.end(); ++chan)
	{
//		bug_var(chan->first);
//		for(std::set<char>::iterator i = chan->second.begin(); i != chan->second.end(); ++i)
//			bug_var(*i);
		if(f == '*' || chan->second.count('*') || chan->second.count(f))
			good = good && send("/say " + chan->first + " [" + irc_katina + "] " + text, res);
	}
	return good;
}

bool PkiClient::configure(const str& params)
{
	// 192.168.0.50:7334 remotekey_id #channel(flags)
	str id, chans;
	siss iss(params);
	if(!(sgl(sgl(iss, host, ':') >> port >> id >> std::ws, chans)))
	{
		log("Bad parameters: " << params);
		return false;
	}
	
	set_chans(chans);
	
	if(!katina.pki.get_public_key_as_text(key))
	{
		log("FATAL: PKIClient requires public key");
		return false;
	}

	siz pos;
	while((pos = key.find_first_of("\n")) != str::npos)
		key.erase(pos, 1);

	if(!katina.pki.get_signature(sig))
	{
		log("FATAL: PKIClient unable to create signature");
		return false;
	}
	
	while((pos = sig.find_first_of("\n")) != str::npos)
		sig.erase(pos, 1);

	return true;
}

bool PkiClient::send(const str& cmd, str& res)
{
	if(!active)
		return true;

	if(!ss.open(host, port))
	{
		log("error: " << std::strerror(errno));
		return false;
	}

	// connect
	// c: pki::req: <key>:<sig>
	// s: pki::acc: <ref>:<key>:<sig>

	// communicate
	// c: pki::cmd: <ref>:<sig>:<cmd>
	// s: pki::cmd: ok
	
	str line;
	
	if(!connected)
	{
		ss << "pki::req: " << key << ':' << sig << std::flush;
		if(!sgl(ss, line))
		{
			log("ERROR: failed to connect to remote");
			return false;
		}

		siss iss(line);
		str head, ref, key, sig;

		if(!(sgl(sgl(sgl(sgl(iss, head, ' '), ref, ':'), key, ':'), sig)))
		{
			log("ERROR: bad response from server: " << line);
			return false;
		}
		
		//sessions[ref] = session(key, sig);
		connected = true;
	}
	
	if(connected)
	{
	}
	
	(ss << cmd).put('\0') << std::flush;
	return std::getline(ss, res, '\0');
}

bool InsecureClient::configure(const str& params)
{
	// 192.168.0.50:7334 #chan1(*) #chan2(*)
	str chans;
	siss iss(params);
	if(!sgl(sgl(iss, host, ':') >> port >> std::ws, chans))
	{
		log("Bad parameters: " << params);
		return false;
	}

	set_chans(chans);

	return true;	
}

bool FileClient::configure(const str& params)
{
	str chans;
	siss iss(params);
	if(!sgl(iss >> ofile >> ifile >> std::ws, chans))
	{
		log("Bad parameters: " << params);
		return false;
	}
	
	set_chans(chans);
	
	ofs.open(ofile.c_str());
	if(!ofs.is_open())
	{
		log("error: " << std::strerror(errno));
		return false;
	}
	ifs.open(ifile.c_str());
	if(!ofs.is_open())
	{
		log("error: " << std::strerror(errno));
		return false;
	}
		
	return true;	
}

RemoteClient* RemoteClient::create(Katina& katina, const str& config)
{
	// bug_func();
	// bug_var(config);
	// config = <type> <params>
	str type, params;
	siss iss(config);
	
	if(sgl(iss >> type >> std::ws, params))
	{
		RemoteClient* c = 0;
		if(type == "pki")
			c = new PkiClient(katina);
		else if(type == "file")
			c = new FileClient(katina);
		else if(type == "insecure")
			c = new InsecureClient(katina);
		else
		{
			log("Unknown type: " << type);
			return 0;
		}
		if(!c)
		{
			log("Error creating client: " << type);
			return 0;
		}
		if(c->configure(params))
			return c;
		delete c;
	}
	
	return 0;
}

}} // katina::net
