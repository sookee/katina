#pragma once
#ifndef _OASTATS_RCON_H_
#define _OASTATS_RCON_H_
/*
 * rcon.h
 *
 *  Created on: 07 Apr 2013
 *      Author: oaskivvy@gmail.com
 */

/*-----------------------------------------------------------------.
| Copyright (C) 2013 SooKee oaskivvy@gmail.com               |
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
#include "log.h"

namespace oastats { namespace net {

using namespace oastats::log;
using namespace oastats::types;

struct server_conf
{
	bool active;
	bool do_flags;
	bool do_dashes;
	bool do_db; // do database writes
	std::set<siz> db_weaps; // which weapons to record

	server_conf()
	: active(false)
	, do_flags(false)
	, do_dashes(false)
	, do_db(false)
	{
	}
};

#define TIMEOUT 1000

/**
 * IPv4 IPv6 agnostic OOB (out Of Band) comms
 * @param cmd
 * @param packets Returned packets
 * @param host Host to connect to
 * @param port Port to connect on
 * @param wait Timeout duration in milliseconds
 * @return false if failed to connect/send or receive else true
 */
bool aocom(const str& cmd, str_vec& packets, const str& host, int port
	, siz wait = TIMEOUT);

bool rcon(const str& cmd, str& reply, const str& host, int port, siz wait = TIMEOUT);

class RCon
{
private:
	str host;
	siz port;
	str pass;

public:
	RCon() {}
	RCon(const str& host, siz port, const str& pass): host(host), port(port), pass(pass) {}

	void config(const str& host, siz port, const str& pass)
	{
		this->host = host;
		this->port = port;
		this->pass = pass;
	}

	bool command(const str& cmd, str& reply) const
	{
		return rcon("rcon " + pass + " " + cmd, reply, host, port, 2000);
	}

	str chat(const str& msg) const
	{
		str ret;
		rcon("rcon " + pass + " chat ^1K^7at^3i^7na^8: ^7" + msg, ret, host, port);
		return ret;
	}

	void cp(const str& msg) const
	{
		str ret;
		rcon("rcon " + pass + " cp " + msg, ret, host, port);
	}

	/**
	 * Set a variable from a cvar using rcon.
	 * @param cvar The name of the cvar whose value is wanted
	 * @param val The variable to set to the cvar's value.
	 */
	bool get_cvar(const str& cvar, str& val) const
	{
		str response;
		if(!command(cvar, response))
		{
			log("WARN: rconset failure: " << cvar);
			return false;
		}

		// Possible responses:
		// -> unknown command: <var>
		// -> "<var>" is:"<val>^7", the default
		// -> "katina_skivvy_chans" is:"#katina-test(c) #katina(c)^7" default:"#katina-test(c)^7"

		str sval;

		if(response.find("unknown command:"))
		{
			str skip;
			siss iss(response);
			if(!std::getline(std::getline(iss, skip, ':').ignore(), sval, '^'))
			{
				log("ERROR: parsing rconset response: " << response);
				return false;
			}
		}

		val = sval;
		return true;
	}

	/**
	 * Set a variable from a cvar using rcon.
	 * @param cvar The name of the cvar whose value is wanted
	 * @param val The variable to set to the cvar's value.
	 * @return thrue if the call was successful otherwise false
	 * and the value of val remains unchanged.
	 */
	template<typename T>
	bool get_cvar(const str& cvar, T& val) const
	{
		str sval;
		if(!get_cvar(cvar, sval))
			return false;
		siss iss(sval);
		T tval;
		if(!(iss >> tval))
			return false;
		val = tval;
		return true;
	}
};

}} // oastats::net

#endif /* _OASTATS_RCON_H_ */
