/*
 * RemoteClient.cpp
 *
 *  Created on: Apr 7, 2013
 *      Author: oasookee@gmail.com
 */

#include <katina/RemoteClient.h>

#include <katina/Katina.h>
#include <katina/utils.h>

namespace oastats { namespace net {

using namespace oastats;
using namespace oastats::utils;
	
// TODO: This should be configurable
const str irc_katina = "04K00at08i00na";

void RemoteClient::set_chans(const str& chans)
{
	// bug_func();
	// bug_var(chans);
	// bug_var(this);
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
			bug_var(this->chans.size());
			set_flags(chan, flags);
			bug_var(this->chans.size());
		}
	}
}

bool RemoteClient::say(char f, const str& text)
{
	// bug_func();
	// bug_var(f);
	// bug_var(text);
	// bug_var(active);
	// bug_var(this);
	if(!active)
		return true; // not error

	str res;
	bool good = true;

	bug_var(chans.size());
	for(chan_map_iter chan = chans.begin(); chan != chans.end(); ++chan)
	{
		bug_var(chan->first);
		for(std::set<char>::iterator i = chan->second.begin(); i != chan->second.end(); ++i)
			bug_var(*i);
		if(f == '*' || chan->second.count('*') || chan->second.count(f))
			good = good && send("/say " + chan->first + " [" + irc_katina + "] " + text, res);
	}
	return good;
}

bool PKIClient::configure(const str& params)
{
	// 192.168.0.50:7334 #channel(flags)
	str chans;
	siss iss(params);
	if(!(sgl(sgl(iss, host, ':') >> port >> std::ws, chans)))
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

bool PKIClient::send(const str& cmd, str& res)
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
			log("ERROR: failed to connect to server");
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
	if(!(sgl(iss, host, ':') >> port))
	{
		log("Bad parameters: " << params);
		return false;
	}

	set_chans(chans);

	return true;	
}

bool FileClient::configure(const str& params)
{
	// bug_func();
	// bug_var(params);
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
		bug_var(type); // file
		bug_var(params); // data/irc-output.txt data/irc-input.txt #test-channel(*)
		RemoteClient* c = 0;
		if(type == "pki")
			c = new PKIClient(katina);
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

}} // oastats::net
