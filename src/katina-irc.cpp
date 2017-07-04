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

#include <katina/time.h>
#include <katina/types.h>
#include <katina/utils.h>
#include <katina/log.h>
#include <katina/socketstream.h>
#include <katina/str.h>
#include <katina/message.h>

#include <pthread.h>
#include <list>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>

using namespace katina;
using namespace katina::log;
using namespace katina::time;
using namespace katina::types;
using namespace katina::utils;
using namespace katina::string;
using namespace katina::ircbot;

class Bot;

struct processor_params
{
	Bot* bot;
	int cs;
	processor_params(Bot* bot, int cs): bot(bot), cs(cs) {}
};

void* processor(void* vp);


inline
str stamp_secs_dot_ms()
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
	thread_data(Bot* bot, void (Bot::*func)()): bot(bot), func(func), i(-1) {}

	int i;
	void (Bot::*func_i)(int);
	thread_data(Bot* bot, void (Bot::*func_i)(int), int i): bot(bot), func_i(func_i), i(i) {}
};

void* thread_run(void* data_vp)
{
	thread_data* data = reinterpret_cast<thread_data*>(data_vp);
	if(data->i > 0)
		(data->bot->*data->func_i)(data->i);
	else
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
	pthread_t rlistener_fut;
	
	typedef std::list<pthread_t> thread_list;
	typedef thread_list::iterator thread_list_iter;
	typedef thread_list::const_iterator thread_list_citer;
	
	std::list<pthread_t> futs;

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

		pthread_create(&responder_fut, NULL, &thread_run, new thread_data(this, &Bot::responder));
		pthread_create(&connecter_fut, NULL, &thread_run, new thread_data(this, &Bot::connecter));
		pthread_create(&rlistener_fut, NULL, &thread_run, new thread_data(this, &Bot::rlistener));

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

	void responder()
	{
		str line;
		message msg;
		while(!done)
		{
			if(!std::getline(ss, line))
			{
				ss.clear();
				connected = false;
				spin_wait(done, 30);
				continue;
			}

			if(!parsemsg(line, msg))
			{
				log("ERROR: parsing message: " << line);
				continue;
			}

			if(msg.command == "001")
				connected = true;
			else if(msg.command == "JOIN")
				prompt(msg.get_nick() << " has joined " << msg.get_chan());
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
	
	void exec(const str& cmd, const str& params, std::ostream* os = 0)
	{
		str channel;
		siss iss(params);
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
			send("PRIVMSG " + channel + " :" + params);
		}
	}
	
	void cli()
	{
		str cmd;
		str line;
		str params;
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
			if(!sgl(iss >> cmd >> std::ws, params))
				continue;

			exec(cmd, params);
			//std::cout << "> " << std::flush;
			//prompt("");
		}
	}

	void join()
	{
		pthread_join(responder_fut, 0);
		pthread_join(connecter_fut, 0);
		ss.close();
	}
	
	void rlistener()
	{
		bug_func();
		
		int ss = ::socket(PF_INET, SOCK_STREAM, 0);
		
		siz p = 3777;
		str host = "0.0.0.0";
		sockaddr_in addr;
		std::memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(p);
		addr.sin_addr.s_addr = inet_addr(host.c_str());
		if(::bind(ss, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1)
		{
			log("ERROR: " << std::strerror(errno));
			return;
		}
		if(::listen(ss, 10) == -1)
		{
			log("ERROR: " << std::strerror(errno));
			return;
		}
		while(!done)
		{
			sockaddr connectee;
			socklen_t connectee_len = sizeof(sockaddr);
			int cs;
	
			while(!done)
			{
				while(!done && (cs = ::accept4(ss, &connectee, &connectee_len, SOCK_NONBLOCK)) == -1)
				{
					if(!done)
					{
						if(errno ==  EAGAIN || errno == EWOULDBLOCK)
							thread_sleep_millis(1000);
						else
						{
							log("ERROR: " << strerror(errno));
							::close(cs);
							return;
						}
					}
				}
	
				if(!done)
				{
					for(thread_list_iter i = futs.begin(); i != futs.end();)
					{
						if(pthread_kill(*i, 0) == ESRCH)
							futs.erase(i);
						else
							++i;
					}
							
					pthread_t fut;
					futs.push_back(fut);
					pthread_create(&fut, NULL, &thread_run, new thread_data(this, &Bot::process, cs));
					//std::async(std::launch::async, [&]{ process(cs); });
				}
			}
		}
	}
	
	void process(int cs)
	{
		str cmd;
		str line;
		str params;
		net::socketstream ss(cs);
		// receive null terminated string
		if(!sgl(ss, line, '\0'))
		{
			done = true;
			log("ERROR: can not read input:");
			return;
		}
		siss iss(line);
		if(!sgl(iss >> cmd >> std::ws, params))
		{
			log("ERROR: parsing remote: " << line);
			return;
		}

		if(!trim(cmd).empty())
		{
	/*
			if(!cmd.find("pki::"))
			{
				// c: pki::req: <key>:<sig>
				// s: pki::acc: <ref>:<key>:<sig>
	
				// c: pki::cmd: <ref>:<sig>:<cmd>
				// s: pki::cmd: ok
	
				if(!cmd.find("pki::req: ")) // initial connection
				{
					// c: pki::req: <key>:<sig>
					// s: pki::acc: <ref>:<key>:<sig>
	
					str skip, key, sig;
					if(!sgl(sgl(sgl(siss(cmd), skip, ' '), key, ':'), sig))
					{
						ss << "pki::err: garbage received" << '\0' << std::flush;
						return;
					}
	
					str ref;
					if(!pki.get_ref(key, ref))
					{
						ss << "pki::err: unknown client" << '\0' << std::flush;
						return;
					}
	
					bool is_good = false;
					if(!pki.verify_signature(ref, sig, is_good))
					{
						ss << "pki::err: server error" << '\0' << std::flush;
						return;
					}
	
					if(!is_good)
					{
						ss << "pki::err: bad signature" << '\0' << std::flush;
						return;
					}
	
					if(!pki.create_signature(sig))
					{
						ss << "pki::err: server error" << '\0' << std::flush;
						return;
					}
	
					ss << "pki::acc: " << ref << ':' << pki.get_public_key() << ':' << sig << '\0' << std::flush;
				}
				else if(!cmd.find("pki::cmd: "))
				{
					// c: pki::cmd: <ref>:<sig>:<cmd>
					// s: pki::cmd: ok
					str skip, ref, sig, cmd;
					if(!sgl(sgl(sgl(sgl(siss(cmd), skip, ' '), ref, ':'), sig, ':'), cmd)
					{
						ss << "pki::err: garbage received" << '\0' << std::flush;
						return;
					}
	
					bool is_good = false;
					if(!pki.verify_signature(ref, sig, is_good))
					{
						ss << "pki::err: server error" << '\0' << std::flush;
						return;
					}
	
					if(!is_good)
					{
						ss << "pki::err: bad signature" << '\0' << std::flush;
						return;
					}
	
					soss oss;
					bot.exec(cmd, &oss);
					ss << "pki::cmd: " << oss.str() << '\0' << std::flush;
				}
				return;
			}
	*/
			soss oss;
			exec(cmd, params, &oss);
			ss << oss.str() << '\0' << std::flush;
		}
	}

};

void* processor(void* vp)
{
	Bot* bot = reinterpret_cast<processor_params*>(vp)->bot;
	int cs = reinterpret_cast<processor_params*>(vp)->cs;
	if(!bot)
		log("ERROR: no bot* for new processor thread");
	else
		bot->process(cs);		
	
	pthread_exit(0);
}

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
