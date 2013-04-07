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

#include "time.h"
#include "types.h"
#include "log.h"
#include "socketstream.h"
#include "str.h"
#include "message.h"

#include <pthread.h>

using namespace oastats;
using namespace oastats::log;
using namespace oastats::time;
using namespace oastats::types;
using namespace oastats::string;
using namespace oastats::ircbot;

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

void save_records(const str_map& recs)
{
	log("save_records:");
	std::ofstream ofs((str(getenv("HOME")) + "/.katina/records.txt").c_str());

	str sep;
	for(str_map_citer r = recs.begin(); r != recs.end(); ++r)
		{ ofs << sep << r->first << ": " << r->second; sep = "\n"; }
}

void load_records(str_map& recs)
{
	log("load_records:");
	std::ifstream ifs((str(getenv("HOME")) + "/.katina/records.txt").c_str());

	recs.clear();
	str key;
	str val;
	while(std::getline(std::getline(ifs, key, ':') >> std::ws, val))
		recs[key] = val;
}

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
		thread_sleep_millis(res);
	return secs;
}

class Bot;

struct thread_data
{
	Bot* bot;
	void (Bot::*func)();
	thread_data(Bot* bot, void (Bot::*func)()): bot(bot), func(func) {}
};

void* thread_run(void* data_vp)
{
	thread_data* data = reinterpret_cast<thread_data*>(data_vp);
	(data->bot->*data->func)();
	delete data;
	pthread_exit(0);
}

class Bot
{
	str host;
	int port;
	bool connected;
	bool done;
	net::socketstream ss;

	str_map recs;
	str_vec nick;
	siz uniq;

	pthread_t responder_fut;
	pthread_t connecter_fut;

public:
	Bot(const str& host, int port)
	: host(host), port(port), connected(false), done(false), uniq(0) {}

	bool start()
	{
		load_records(recs);
		siss iss(recs["ircbot.nicks"]);
		str n;
		while(iss >> n)
			nick.push_back(n);

		if(nick.empty())
			nick.push_back("Katina");

		std::cout << "> " << std::flush;
		std::srand(std::time(0));
		if(!ss.open(host, port))
		{
			log("Failed to open connection to server: " << strerror(errno));
			return false;
		}

		pthread_create(&connecter_fut, NULL, &thread_run, new thread_data(this, &Bot::connecter));
		pthread_create(&responder_fut, NULL, &thread_run, new thread_data(this, &Bot::responder));

		return true;
	}

	// ss << cmd.substr(0, 510) << "\r\n" << std::flush;

	void send(const str& cmd)
	{
//		bug("send: " << cmd);
		static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

		pthread_mutex_lock(&mtx);
		ss << cmd.substr(0, 510) << "\r\n" << std::flush;
		pthread_mutex_unlock(&mtx);
	}

	str ping;
	str pong;

	void connecter()
	{
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
				send("PING " + (ping = to_string(std::rand())));
				spin_wait(done, 60);
				if(ping != pong)
					connected = false;
			}
		}
	}

	void cli()
	{
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
		pthread_join(responder_fut, 0);
		pthread_join(connecter_fut, 0);
		ss.close();
	}

	void responder()
	{
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
				for(siz i = 0; i < ops.size(); ++i)
					prompt(ops[i]);
				for(siz i = 0; i < vocs.size(); ++i)
					prompt(vocs[i]);
				for(siz i = 0; i < nons.size(); ++i)
					prompt(nons[i]);
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
