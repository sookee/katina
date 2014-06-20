//#pragma once
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

#include <functional>

#include "types.h"
#include "time.h"
#include "log.h"
#include "str.h"

namespace katina { namespace net {

using namespace katina::log;
using namespace katina::time;
using namespace katina::types;
using namespace katina::string;

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
	bool active;
	
	str host;
	siz port;
	str pass;

public:
	RCon(): active(false), port(0) {}
	RCon(const str& host, siz port, const str& pass): active(false), host(host), port(port), pass(pass) {}

	void config(const str& host, siz port, const str& pass)
	{
		this->host = host;
		this->port = port;
		this->pass = pass;
	}

	void on()
	{
		active = true;
		log("rcon: active");
	}
	void off()
	{
		active = false;
		log("rcon: inactive");
	}
	
	bool command(const str& cmd)
	{
		if(!active)
			return true;
		str reply;
		return command(cmd, reply);
	}

	bool command(const str& cmd, str& reply)
	{
		if(!active)
			return true;
		return rcon("rcon " + pass + " " + cmd, reply, host, port, 2000);
	}

	str chat(const str& msg) const
	{
		if(!active)
			return "";
		str ret;
		rcon("rcon " + pass + " chat ^1K^7at^3i^7na^8: ^7" + msg, ret, host, port);
		return ret;
	}

	bool has_chatnobeep() const
	{
		if(!active)
			return false;

		static int y = -1;

		if(y < 0)
		{
			str ret;
			rcon("rcon " + pass + " chatnobeep", ret, host, port);

			// FIXME: This test must be a POSSITIVE test for success
			// ����      unknown command: chatnobeep
			y = (ret.find("unknown command: chatnobeep") != str::npos) ? 0 : 1;
		}
		return y;
	}

	bool has_msg_to() const
	{
		if(!active)
			return false;

		static int y = -1;

		if(y < 0)
		{
			str ret;
			if(!rcon("rcon " + pass + " msg_to", ret, host, port))
				rcon("rcon " + pass + " msg_to", ret, host, port);

			// FIXME: This test must be a POSSITIVE test for success
			// ����      unknown command: chatnobeep
			y = (ret.find("usage: msg_to") != str::npos) ? 1 : 0;
		}
		return y;
	}

	bool msg_to(slot num, const str& message, bool beep = false)
	{
		if(!active)
			return true;
		str ret;
		if(!has_msg_to())
		{
			if(beep)
				chat(message);
			else
				chat_nobeep(message);
			return true;
		}
		return rcon("rcon " + pass + " msg_to" + (beep?"_beep ":" ") + to_string(num) + " " + message, ret, host, port);
	}

	bool msg_to_all(const str& message, bool beep = false)
	{
		return msg_to(slot::all, message, beep);
	}

	str chat_nobeep(const str& msg) const
	{
		if(!active)
			return "";

		if(!has_chatnobeep())
			return chat(msg);

		str ret;
		rcon("rcon " + pass + " chatnobeep ^1K^7at^3i^7na^8: ^7" + msg, ret, host, port);

		return ret;
	}

	void cp(const str& msg) const
	{
		if(!active)
			return;
		str ret;
		rcon("rcon " + pass + " cp " + msg, ret, host, port);
	}
	
	bool s_chat(const str& msg) const
	{
		if(!active)
			return true;
		str ret;
		return s_chat(msg, ret);
	}

	bool s_chat(const str& msg, str& ret) const
	{
		if(!active)
			return true;
		return rcon("rcon " + pass + " chat ^1K^7at^3i^7na^8: ^7" + msg, ret, host, port);
	}
};

//TYPEDEF_DEQ(std::function<void>, funky_deq);
typedef std::deque<std::function<void()>> funky_deq;

class DelayChamber
{
private:
	const siz MAX_DEPTH = 100;

	funky_deq dq;
	std::mutex mtx_dq;

public:
	bool done = false;

	bool add(std::function<void()>& func)
	{
		lock_guard lock(mtx_dq);
		if(dq.size() >= MAX_DEPTH)
			return false;
		dq.push_back(func);
		return true;
	}

	void process()
	{
		while(!done)
		{
			thread_sleep_millis(1000);
			lock_guard lock(mtx_dq);
			if(dq.empty())
				continue;
			dq.front()();
			dq.pop_front();
		}
	}
};

}} // katina::net

#endif /* _OASTATS_RCON_H_ */
