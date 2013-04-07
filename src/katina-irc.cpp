/*
 * katina-irc.cpp
 *
 *  Created on: 07 Apr 2013
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

#include <fstream>
#include <cstring>
#include <cstdlib>

#include "types.h"
#include "log.h"
#include "socketstream.h"
#include "str.h"

using namespace oastats;
using namespace oastats::log;
using namespace oastats::types;
using namespace oastats::string;

inline
str stamp()
{
	time_t rawtime = std::time(0);
	tm* timeinfo = std::localtime(&rawtime);
	char buffer[9];
	std::strftime(buffer, 32, "%H:%M:%S", timeinfo);

	return std::string(buffer);
}

#define prompt(m) do{std::cout << "[" << stamp() << "] " << m << "\n> " << std::flush;}while(false)

/*
 * Wait for a condition to become true else timeout.
 *
 * @param cond - a reference to the bool condition that
 * this function is waiting for.
 * @param secs - The number of seconds to wait.
 * @param res - The pause resolution - how long to wait between
 * testing the condition.
 * @return false - timout reached
 */
bool spin_wait(const bool& cond, siz secs, siz res = 1000)
{
	while(!cond && --secs)
		std::this_thread::sleep_for(std::chrono::milliseconds(res));
	return secs;
}

class Bot
{
	str host;
	int port;
	net::socketstream ss;
	bool connected = false;
	bool done = false;

	str_vec nick = {"Skivlet", "Skivlet_2", "Skivlet_3", "Skivlet_4", "Skivlet_5"};
	siz uniq = 0;

	std::future<void> responder_fut;
	std::future<void> connecter_fut;

public:
	Bot(const str& host, int port): host(host), port(port) {}

	bool start()
	{
		std::cout << "> " << std::flush;
		func();
		std::srand(std::time(0));
		if(!ss.open(host, port))
		{
			log("Failed to open connection to server: " << strerror(errno));
			return false;
		}
		connecter_fut = std::async(std::launch::async, [&]{ connecter(); });
		responder_fut = std::async(std::launch::async, [&]{ responder(); });
		return true;
	}

	// ss << cmd.substr(0, 510) << "\r\n" << std::flush;

	void send(const str& cmd)
	{
//		bug("send: " << cmd);
		static std::mutex mtx;
		lock_guard lock(mtx);
		ss << cmd.substr(0, 510) << "\r\n" << std::flush;
	}

	str ping;
	str pong;

	void connecter()
	{
		func();
		send("PASS none");
		send("NICK " + nick[uniq]);
		while(!done)
		{
			if(!connected)
			{
				prompt("Attempting to connect...");
				send("USER Skivlet 0 * :Skivlet");
				spin_wait(done, 30);
			}
			if(connected)
			{
//				bug_var(uniq);
				if(uniq)
					send("NICK " + nick[0]); // try to regain primary nick
				send("PING " + (ping = std::to_string(std::rand())));
				spin_wait(done, 60);
				if(ping != pong)
					connected = false;
			}
		}
	}

	void cli()
	{
		func();
		str cmd;
		str line;
		str channel;
		std::istringstream iss;
//		std::cout << "> " << std::flush;
		while(!done)
		{
			if(!std::getline(std::cin, line))
			{
				done = true;
				log("Can not read input:");
				continue;
			}
			iss.clear();
			iss.str(line);
			if(!(iss >> cmd >> std::ws))
				continue;

			if(cmd == "/exit")
			{
				done = true;
				send("QUIT :leaving");
			}
			else if(cmd == "/join")
			{
				if(!channel.empty())
					send("PART " + channel + ": ...places to go people to see...");
				iss >> channel;
				send("JOIN " + channel);
			}
			else if(cmd == "/part")
			{
				if(!channel.empty())
					send("PART " + channel + " ...places to go people to see...");
			}
			else
			{
				send("PRIVMSG " + channel + " :" + line);
			}
			std::cout << "> " << std::flush;
			//prompt("");
		}
	}

	void join()
	{
		if(responder_fut.valid())
			responder_fut.get();
		if(connecter_fut.valid())
			connecter_fut.get();
		ss.close();
	}

	void responder()
	{
		func();
		str line;
		message msg;
		while(!done)
		{
			if(!std::getline(ss, line))
				continue;
//			bug("recv: " << line);
			//std::istringstream iss(line);
			parsemsg(line, msg);
//			bug_msg(msg);

			if(msg.command == "001")
				connected = true;
			else if(msg.command == "JOIN")
			{
				// :Skivlet!~Skivlet@cpc21-pool13-2-0-cust125.15-1.cable.virginmedia.com JOIN #teammega
//				prompt("recv: " << line);
//				bug_msg(msg);
				prompt(msg.get_nick() << " has joined " << msg.get_chan());
			}
			else if(msg.command == "QUIT")
				prompt(msg.get_nick() << " has quit: " << msg.get_trailing());
			else if(msg.command == "PING")
				send("PONG " + msg.get_trailing());
			else if(msg.command == "332") // RPL_TOPIC
				;
			else if(msg.command == "333") // Undocumented
				;
			else if(msg.command == "353") // RPL_NAMREPLY
			{
				str nick;
				str_vec ops, vocs, nons;
				std::istringstream iss(msg.get_trailing());
				while(iss >> nick)
				{
					if(nick.empty())
						continue;
					if(nick[0] == '@')
						ops.push_back(nick);
					else if(nick[0] == '+')
						vocs.push_back(nick);
					else
						nons.push_back(nick);
				}
				std::sort(ops.begin(), ops.end());
				std::sort(vocs.begin(), vocs.end());
				std::sort(nons.begin(), nons.end());
				for(const str& nick: ops)
					prompt(nick);
				for(const str& nick: vocs)
					prompt(nick);
				for(const str& nick: nons)
					prompt(nick);
			}
			else if(msg.command == "372") // RPL_MOTD
				prompt("MOTD: " << msg.get_trailing());
			else if(msg.command == "376") // RPL_ENDOFMOTD
				prompt("MOTD: " << msg.get_trailing());
			else if(msg.command == "433") // NICK IN USE
			{
				++uniq;
				if(uniq >= nick.size())
					uniq = 0;
			}
			else if(msg.command == "PONG")
				pong = msg.get_trailing();
			else if(msg.command == "NOTICE")
				prompt("NOTICE: " << msg.get_trailing());
			else if(msg.command == "PRIVMSG")
				prompt(msg.get_to() << ": <" << msg.get_nick() << "> " << msg.get_trailing());
			else
				prompt("recv: " << line);
		}
	}
};

int main()
{
	Bot bot("irc.quakenet.org", 6667);
//	Bot bot("irc.se.quakenet.org", 6667);
	if(bot.start())
	{
		bot.cli();
		bot.join();
	}
}
