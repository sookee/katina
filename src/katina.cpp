/*
 * katina.cpp
 *
 *  Created on: 18 Jun 2012
 *      Author: oasookee@googlemail.com
 */


/*-----------------------------------------------------------------.
| Copyright (C) 2012 SooKee oasookee@googlemail.com               |
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

//#include "logrep.h"
//#include "str.h"
//#include "types.h"
//#include "rcon.h"
#include "codes.h"

#include <fstream>
#include <sstream>
#include <iostream>
//#include <thread>
#include <exception>
#include <stdexcept>

#include <map>
//#include <array>

#include <ctime>

//#include <chrono>

#include <sys/time.h>
#include <pthread.h>

// STACK TRACE
#include <cstdio>
#include <cstdlib>
#include <sys/wait.h>
#include <unistd.h>

#include <sys/resource.h>
#include <cassert>

#include <vector>
#include <map>
#include <set>

// rcon
#include <cerrno>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "socketstream.h"

using namespace oastats;
//using namespace oastats::types;
//using namespace oastats::string;

//-- TYPES ---------------------------------------------

typedef std::size_t siz;

typedef std::string str;
typedef str::iterator str_iter;
typedef str::const_iterator str_citer;

typedef std::vector<int> int_vec;
typedef std::vector<siz> siz_vec;

typedef std::vector<str> str_vec;
typedef str_vec::iterator str_vec_iter;
typedef str_vec::const_iterator str_vec_citer;

// sets
typedef std::set<str> str_set;
typedef str_set::iterator str_set_iter;
typedef str_set::const_iterator str_set_citer;

typedef std::multiset<str> str_mset;

// maps
typedef std::map<str, str> str_map;
typedef str_map::iterator str_map_iter;
typedef str_map::const_iterator str_map_citer;

typedef std::pair<const str, str> str_map_pair;

typedef std::map<siz, siz> siz_map;
typedef siz_map::iterator siz_map_iter;
typedef siz_map::const_iterator siz_map_citer;
typedef std::pair<const siz, siz> siz_map_pair;

typedef std::map<str, siz> str_siz_map;
typedef str_siz_map::iterator str_siz_map_iter;
typedef str_siz_map::const_iterator str_siz_map_citer;
typedef std::pair<const str, siz> str_siz_map_pair;

typedef std::map<siz, str> siz_str_map;
typedef siz_str_map::iterator siz_str_map_iter;
typedef siz_str_map::const_iterator siz_str_map_citer;
typedef std::pair<const siz, str> siz_str_map_pair;

typedef std::map<str, time_t> str_time_map;
typedef str_time_map::iterator str_time_map_iter;
typedef str_time_map::const_iterator str_time_map_citer;
typedef std::pair<const str, time_t> str_time_map_pair;

typedef std::map<str, str_set> str_set_map;
typedef str_set_map::iterator str_set_map_iter;
typedef str_set_map::const_iterator str_set_map_citer;
typedef std::pair<const str, str_set> str_set_map_pair;

typedef std::map<const str, str_vec> str_vec_map;
typedef str_vec_map::iterator str_vec_map_iter;
typedef str_vec_map::const_iterator str_vec_map_citer;
typedef std::pair<const str, str_vec> str_vec_map_pair;

typedef std::multimap<str, str> str_mmap;
typedef str_mmap::iterator str_mmap_iter;
typedef str_mmap::const_iterator str_mmap_citer;

// streams
typedef std::istream sis;
typedef std::ostream sos;
typedef std::iostream sios;

typedef std::stringstream sss;
typedef std::istringstream siss;
typedef std::ostringstream soss;

typedef std::fstream sfs;
typedef std::ifstream sifs;
typedef std::ofstream sofs;

typedef std::stringstream sss;


//typedef st_clk clock_p;
//typedef clock_p::period period_p;
//typedef clock_p::time_point time_p;

// -- STRING -------------------------------------------------

/**
 * Remove leading characters from a std::string.
 * @param s The std::string to be modified.
 * @param t The set of characters to delete from the beginning
 * of the string.
 * @return The same string passed in as a parameter reference.
 */
inline std::string& ltrim(std::string& s, const char* t = " \t\n\r\f\v")
{
	s.erase(0, s.find_first_not_of(t));
	return s;
}

/**
 * Remove trailing characters from a std::string.
 * @param s The std::string to be modified.
 * @param t The set of characters to delete from the end
 * of the string.
 * @return The same string passed in as a parameter reference.
 */
inline std::string& rtrim(std::string& s, const char* t = " \t\n\r\f\v")
{
	s.erase(s.find_last_not_of(t) + 1);
	return s;
}

/**
 * Remove surrounding characters from a std::string.
 * @param s The string to be modified.
 * @param t The set of characters to delete from each end
 * of the string.
 * @return The same string passed in as a parameter reference.
 */
inline std::string& trim(std::string& s, const char* t = " \t\n\r\f\v")
{
	return ltrim(rtrim(s, t), t);
}

// -- LOGGING ------------------------------------------------

str get_stamp()
{
	time_t rawtime = std::time(0);
	tm* timeinfo = std::localtime(&rawtime);
	char buffer[32];
	std::strftime(buffer, 32, "%Y-%m-%d %H:%M:%S", timeinfo);

	return str(buffer);
}

#ifndef DEBUG
#define bug(m)
#else
#define bug(m) do{std::cout << m << std::endl;}while(false)
#endif
#define con(m) do{std::cout << m << std::endl;}while(false)
#define log(m) do{std::cout << get_stamp() << ": " << m << std::endl;}while(false)

template<typename T, siz SIZE>
struct array
{
	T data[SIZE];

	array()
	{
		for(siz i = 0; i < SIZE; ++i)
			this->data[i] = '0';
	}

	array(const char data[SIZE])
	{
		for(siz i = 0; i < SIZE; ++i)
			this->data[i] = data[i];
	}

	array(const array<T, SIZE>& a)
	{
		for(siz i = 0; i < SIZE; ++i)
			this->data[i] = a.data[i];
	}

	const array& operator=(const array<T, SIZE>& a)
	{
		for(siz i = 0; i < SIZE; ++i)
			this->data[i] = a.data[i];
		return *this;
	}

	bool operator==(const array<T, SIZE>& a) const
	{
		for(siz i = 0; i < SIZE; ++i)
			if(this->data[i] != a.data[i])
				return false;
		return true;
	}

	bool operator!=(const array<T, SIZE>& a) const
	{
		return !(*this == a);
	}

	bool operator<(const array<T, SIZE>& a) const
	{
		for(siz i = 0; i < SIZE; ++i)
			if(this->data[i] < a.data[i])
				return true;
		return false;
	}

	T& operator[](siz i) { return data[i]; }
	const T& operator[](siz i) const { return data[i]; }
	siz size() const { return SIZE; }
};

typedef array<char, 16> GUID;

sos& operator<<(sos& os, const GUID& guid)
{
	for(siz i = 0; i < guid.size(); ++i)
		os << guid[i];
	return os;
}

sis& operator>>(sis& is, GUID& guid)
{
	for(siz i = 0; i < guid.size(); ++i)
		is.get(guid[i]);
	return is;
}

const GUID null_guid = "0000000000000000";

GUID bot_guid(siz num)
{
	soss oss;
	oss << num;
	str id = oss.str();
	if(id.size() < 16)
		id = str(16 - id.size(), '0') + id;

	return GUID(id.c_str());
}

void delay(siz msecs)
{
	timespec requested_time;
	timespec remaining;
	requested_time.tv_sec = 0;
	requested_time.tv_nsec = msecs * 1000;
	while(true)
	{
		if(!nanosleep(&requested_time, &remaining))
			break;
		if(!remaining.tv_nsec)
			break;
		requested_time.tv_nsec = remaining.tv_nsec;
	}
}

template<typename T>
str to_string(const T& t)
{
	soss oss;
	oss << t;
	return oss.str();
}

template<typename T>
T to(const str& s)
{
	T t;
	siss iss(s);
	iss >> t;
	return t;
}

// -- RCON ----------------------------------------------

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
	, siz wait = TIMEOUT)
{
	addrinfo hints;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; // AF_INET or AF_INET6
	hints.ai_socktype = SOCK_DGRAM;

	addrinfo* res;
	if(int status = getaddrinfo(host.c_str(), to_string(port).c_str(), &hints, &res) != 0)
	{
		log(gai_strerror(status));
		return false;
	}

//	st_time_point timeout = st_clk::now() + std::chrono::milliseconds(wait);
	timespec timeout;
	clock_gettime(CLOCK_REALTIME, &timeout);
	timeout.tv_nsec = wait * 1000;
	if(timeout.tv_nsec > 1000000)
	{
		timeout.tv_nsec -= 1000000000;
		++timeout.tv_sec;
	}

	// try to connect to each
	int cs;
	addrinfo* p;
	for(p = res; p; p = p->ai_next)
	{
		if((cs = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
			continue;
		if(!connect(cs, p->ai_addr, p->ai_addrlen))
			break;
		::close(cs);
	}

	freeaddrinfo(res);

	if(!p)
	{
		log("aocom: failed to connect: " << host << ":" << port);
		return false;
	}

	// cs good

	const str msg = "\xFF\xFF\xFF\xFF" + cmd;

	int n = 0;
	if((n = send(cs, msg.c_str(), msg.size(), 0)) < 0 || n < (int)msg.size())
	{
		log("cs send: " << strerror(errno));
		return false;
	}

	packets.clear();

	char buf[2048];

	n = sizeof(buf);
	while(n == sizeof(buf))
	{
		while((n = recv(cs, buf, sizeof(buf), MSG_DONTWAIT)) ==  -1 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR))
		{
			timespec now;
			clock_gettime(CLOCK_REALTIME, &now);
			if(now.tv_sec > timeout.tv_sec || now.tv_nsec > timeout.tv_nsec)
			{
				log("socket timed out connecting to: " << host << ":" << port);
				return false;
			}
//			std::this_thread::yield();
//			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			delay(10);
		}
		if(n < 0)
			log("cs recv: " << strerror(errno));
		if(n > 0)
			packets.push_back(str(buf, n));
	}

	close(cs);

	return true;
}

bool rcon(const str& cmd, str& reply, const str& host, int port)
{
	str_vec packets;
	if(!aocom(cmd, packets, host, port, TIMEOUT))
		return false;

	const str header = "\xFF\xFF\xFF\xFFprint\x0A";

	if(packets.empty())
	{
		log("Empty response.");
		return false;
	}

	reply.clear();
	for(str_vec_iter packet = packets.begin(); packet != packets.end(); ++packet)
	{
		if(packet->find(header) != 0)
		{
			log("Unrecognised response.");
			return false;
		}

		reply.append(packet->substr(header.size()));
	}

	return true;
}

struct stats
{
	siz_map kills;
	siz_map deaths;
	siz_map flags;
	siz_map awards;

	time_t first_seen;
	time_t logged_time;

	stats(): kills(), deaths(), flags(), awards(), first_seen(0), logged_time(0) {}
};

bool usage()
{
	std::cout << "Usage: " << '\n';
	return -1;
}

void test()
{
	GUID guid("FD3FED56A7F7FB2A");
}

typedef std::map<GUID, str> guid_str_map;
typedef std::pair<const GUID, str> guid_str_pair;
typedef std::map<GUID, str>::iterator guid_str_iter;
typedef std::map<GUID, str>::const_iterator guid_str_citer;

typedef std::map<siz, GUID> siz_guid_map;
typedef std::pair<const siz, GUID> siz_guid_pair;
typedef std::map<siz, GUID>::iterator siz_guid_iter;
typedef std::map<siz, GUID>::const_iterator siz_guid_citer;

typedef std::map<GUID, siz> guid_siz_map;
typedef std::pair<const GUID, siz> guid_siz_pair;
typedef std::map<GUID, siz>::iterator guid_siz_iter;
typedef std::map<GUID, siz>::const_iterator guid_siz_citer;

typedef std::map<GUID, stats> guid_stat_map;
typedef std::pair<const GUID, stats> guid_stat_pair;
typedef std::map<GUID, stats>::iterator guid_stat_iter;
typedef std::map<GUID, stats>::const_iterator guid_stat_citer;

void testxxx()
{
	siz_guid_map m;
	GUID guid;
	m[0] = guid;
}

//typedef std::map<str, str> str_str_map;
//typedef std::pair<const str, str> str_str_pair;
//typedef std::map<siz, str> siz_str_map;
//typedef std::pair<const siz, str> siz_str_pair;

typedef std::map<GUID, guid_siz_map> onevone_map;
typedef std::pair<const GUID, guid_siz_map> onevone_pair;
typedef std::map<GUID, guid_siz_map>::iterator onevone_iter;
typedef std::map<GUID, guid_siz_map>::const_iterator onevone_citer;

typedef std::multimap<siz, str> siz_str_mmap;
typedef siz_str_mmap::reverse_iterator siz_str_mmap_ritr;
typedef siz_str_mmap::iterator siz_str_mmap_iter;
typedef siz_str_mmap::const_iterator siz_str_mmap_citer;

//typedef std::pair<const str, siz> str_siz_pair;

//siz_str_map clients; // slot -> GUID
//str_str_map players; // GUID -> name
//onevone_map onevone; // GUID -> GUID -> <count> //

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

	bool command(const str& cmd, str& reply)
	{
		return rcon("rcon " + pass + " " + cmd, reply, host, port);
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

};

// -- IRC --------------------------------------------------------------

#define ColorIndex(c)	( ( (c) - '0' ) & 7 )
#define Q_COLOR_ESCAPE	'^'
#define Q_IsColorString(p)	( p && *(p) == Q_COLOR_ESCAPE && *((p)+1) && isalnum(*((p)+1)) ) // ^[0-9a-zA-Z]
#define Q_IsSpecialChar(c) ((c) && ((c) < 32))

const int oatoirctab[8] =
{
	1 // "black"
	, 4 // "red"
	, 3 // "lime"
	, 8 // "yellow"
	, 2 // "blue"
	, 12 // "cyan"
	, 6 // "magenta"
	, 0 // "white"
};

const str IRC_BOLD = "";
const str IRC_NORMAL = "";
const str IRC_COLOR = "";

str oa_to_IRC(const char* msg)
{
	std::ostringstream oss;

	oss << IRC_BOLD;

	while(*msg)
	{
		if(Q_IsColorString(msg))
		{
			oss << IRC_NORMAL;
			siz code = (*(msg + 1)) % 8;
			oss << IRC_COLOR << (oatoirctab[code] < 10 ? "0" : "") << oatoirctab[code];
			msg += 2;
		}
		else if(Q_IsSpecialChar(*msg))
		{
			oss << "#";
			msg++;
		}
		else
		{

			oss << *msg;
			msg++;
		}
	}

	oss << IRC_NORMAL;

	return oss.str();
}

str oa_to_IRC(const str& msg)
{
	return oa_to_IRC(msg.c_str());
}

const str irc_katina = "04K00at08i00na";

class SkivvyClient
{
private:
//	std::mutex mtx;
	bool active;
	net::socketstream ss;
	str host;
	siz port;
	str_set chans;

public:
	SkivvyClient(): active(false), host("localhost"), port(7334) {}

	void on() { active = true; }
	void off() { active = false; }

	void config(const str& host, siz port, const str_set& chans)
	{
//		bug_func();
//		bug_var(host);
//		bug_var(port);
		for(str_set_iter chan = chans.begin(); chan != chans.end(); ++chan)
			bug("chan: " << *chan);
		this->host = host;
		this->port = port;
		this->chans = chans;
	}

	bool say(const str& text)
	{
		if(!ss.open(host, port))
		{
			log("error: " << std::strerror(errno));
			return false;
		}
		str res;
		bool good = true;
		for(str_set_iter chan = chans.begin(); chan != chans.end(); ++chan)
			good = good && send("/say " + *chan + " [" + irc_katina + "] " + text, res);
		return good;
	}

	bool chat(const str& text) { return say(oa_to_IRC(text)); }

	bool send(const str& cmd, str& res)
	{
//		lock_guard lock(mtx);
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

RCon server;
SkivvyClient skivvy;

void chat(const str& msg)
{
	server.chat(msg);
	skivvy.chat(msg);
}

void report_clients(const siz_guid_map& clients)
{
	for(siz_guid_citer i = clients.begin(); i != clients.end(); ++i)
		con("slot: " << i->first << ", " << i->second);
}

void report_players(const guid_str_map& players)
{
	for(guid_str_citer i = players.begin(); i != players.end(); ++i)
		con("player: " << i->first << ", " << i->second);
}

void report_onevone(const onevone_map& onevone, const guid_str_map& players)
{
	for(onevone_citer o = onevone.begin(); o != onevone.end(); ++o)
		for(guid_siz_citer p = o->second.begin(); p != o->second.end(); ++p)
		{
			str p1 = players.at(o->first);
			str p2 = players.at(p->first);
			con("player: " << p1 << " killed " << p2 << " " << p->second << " times.");
		}
}

typedef std::multimap<siz, GUID> siz_guid_mmap;
typedef std::pair<const siz, GUID> siz_guid_pair;
typedef siz_guid_mmap::reverse_iterator siz_guid_mmap_ritr;

void report_caps(const guid_siz_map& caps, const guid_str_map& players)
{
	siz_guid_mmap sorted;
	for(guid_siz_citer c = caps.begin(); c != caps.end(); ++c)
		sorted.insert(siz_guid_pair(c->second, c->first));

	siz i = 0;
	siz d = 1;
	siz max = 0;
	siz flags = 0;
	str_vec results;
	std::ostringstream oss;
	for(siz_guid_mmap_ritr ri = sorted.rbegin(); ri != sorted.rend(); ++ri)
	{
		++i;
		if(flags != ri->first)
			{ d = i; flags = ri->first; }
		oss.str("");
		oss << "^3#" << d << " ^7" << players.at(ri->second) << " ^3capped ^7" << ri->first << "^3 flags.";
		results.push_back(oss.str());
		if(oss.str().size() > max)
			max = oss.str().size();
	}

	chat("^5== ^6RESULTS ^5" + str(max - 23, '='));
	for(siz i = 0; i < results.size(); ++i)
		chat(results[i]);
	chat("^5" + str(max - 12, '-'));
}

siz map_get(const siz_map& m, siz key)
{
	return m.find(key) == m.end() ? 0 : m.at(key);
}

void report_stats(const guid_stat_map& stats, const guid_str_map& players)
{
	for(guid_stat_citer p = stats.begin(); p != stats.end(); ++p)
	{
		const str& player = players.at(p->first);
		con("player: " << player);
		con("\t caps: " << map_get(p->second.flags, FL_CAPTURED));
		con("\tkills: " << map_get(p->second.kills, MOD_RAILGUN));
		con("\t defs: " << map_get(p->second.awards, AW_DEFENCE));
		con("\tgaunt: " << map_get(p->second.awards, AW_GAUNTLET));
	}
}

void save_records(const str_map& recs)
{
	std::ofstream ofs((str(getenv("HOME")) + "/.katina/high-scores.txt").c_str());

	str sep;
	for(str_map_citer r = recs.begin(); r != recs.end(); ++r)
		{ ofs << sep << r->first << ": " << r->second; sep = "\n"; }
}

void load_records(str_map& recs)
{
	std::ifstream ifs((str(getenv("HOME")) + "/.katina/high-scores.txt").c_str());

	recs.clear();
	str key;
	str val;
	while(std::getline(std::getline(ifs, key, ':') >> std::ws, val))
		recs[key] = val;
}

int main(const int argc, const char* argv[])
{
	str_map recs; // high scores
	load_records(recs);

	log("Records loaded: " << recs.size());

	sifs ifs;
	if(argc > 1)
		ifs.open(argv[1], std::ios::ate);

	sis& is = (argc > 1) ? ifs : std::cin;

	if(!is)
	{
		log("Input error:");
		return -2;
	}

	server.config(recs["rcon.host"], to<siz>(recs["rcon.port"]), recs["rcon.pass"]);

	{
		str res; // skivvy result
		str_set chans; // skivvy channels
		res = recs["skivvy.chan"];
		siss iss(res);
		while(iss >> res)
			chans.insert(res);
		skivvy.config(recs["skivvy.host"], to<siz>(recs["skivvy.port"]), chans);
		if(recs["skivvy.active"] == "true")
			skivvy.on();
	}

	chat("^3Stats System v^70.1^3-alpha - ^1ONLINE");

	std::time_t time = 0;
	char c;
	siz m, s;
	str skip, name, cmd, stamp;
	siz secs = 0;
	std::istringstream iss;
	bool in_game = false;

	siz flags[2];

	siz_guid_map clients; // slot -> GUID
	guid_str_map players; // GUID -> name
	onevone_map onevone; // GUID -> GUID -> <count> //
	guid_siz_map caps; // GUID -> <count> // TODO: caps container duplicated in stats::caps
	guid_stat_map stats; // GUID -> <stat>
	guid_siz_map teams; // GUID -> 'R' | 'B'
	str mapname; // current map name

	typedef std::map<str, time_t> str_utime_map; // GUID -> time
	typedef std::pair<const str, time_t> str_utime_pair;
//	str_utime_map dash[2];
	timespec dash[2];// = {0, 0}; // time of dash start
	GUID dasher[2]; // who is dashing
	bool dashing[2] = {true, true}; // flag dash in progress?

	const str flag[2] = {"^1RED", "^4BLUE"};

	bool done = false;
	std::ios::streampos pos = is.tellg();
	str line;
	while(!done)
	{
		if(!std::getline(is, line) || is.eof())
			{ delay(10); is.clear(); is.seekg(pos); continue; }
		pos = is.tellg();

		bug("line: " << line);

		iss.clear();
		iss.str(line);
		iss >> m >> c >> s >> cmd;
		secs = (m * 60) + s;
		bug("cmd: " << cmd);
		if(in_game)
		{
			if(cmd == "Exit:")
			{
				chat("^3Exit:");
				in_game = false;

				try
				{
					if(!caps.empty())
						report_caps(caps, players);

					// report
					con("Report:");
					report_clients(clients);
					con("");
					report_players(players);
					con("");
					report_onevone(onevone, players);
					con("");
					report_stats(stats, players);
					con("");
				}
				catch(std::exception& e)
				{
					con(e.what());
				}
			}
			else if(cmd == "ShutdownGame:")
			{
				chat("^ShutdownGame:");
				in_game = false;
			}
			else if(cmd == "Warmup:")
			{
				in_game = false;
			}
			else if(cmd == "ClientUserinfoChanged:")
			{
				//do_rcon("^3ClientUserinfoChanged:");
				// 0:23 ClientUserinfoChanged: 2 n\^1S^2oo^3K^5ee\t\2\model\ayumi/red\hmodel\ayumi/red\g_redteam\\g_blueteam\\c1\1\c2\1\hc\100\w\0\l\0\tt\0\tl\1\id\1A7C66BACBA16F0C9068D8B82C1D55DE
				siz num;
				if(!std::getline(std::getline(iss >> num, skip, '\\'), name, '\\'))
				{
					std::cout << "Error parsing ClientUserinfoChanged: "  << line << '\n';
					continue;
				}
				siz pos = line.find("id\\");
				if(pos != str::npos)
				{
					str guid = line.substr(pos + 3, 32);
					bug("guid: " << guid);
					if(guid.size() != 32)
						clients[num] = bot_guid(num);//null_guid;
					else
						clients[num] = to<GUID>(guid);
					players[clients[num]] = name;
					if(stats[clients[num]].first_seen)
						stats[clients[num]].logged_time += std::time(0) - stats[clients[num]].first_seen;
					stats[clients[num]].first_seen = time + secs;

					teams[clients[num]] = 'U'; // unknown

					str reply;
					server.command("!listplayers", reply); // TODO:
					trim(reply);
					// !listplayers: 4 players connected:
					//  1 R 0   Unknown Player (*)   Major
					//  2 B 0   Unknown Player (*)   Tony
					//  4 B 0   Unknown Player (*)   Sorceress
					//  5 R 0   Unknown Player (*)   Sergei
					if(!reply.empty())
					{
						siz n;
						char team;
						siss iss(reply);
						str line;
						std::getline(iss, line); // skip command
						while(std::getline(iss,line))
						{
							siss iss(line);
							if(iss >> n >> team)
								teams[clients[n]] = team;
						}
					}
				}
			}
			else if(cmd == "Kill:")
			{
				//bug("Kill:");
				// 3:20 Kill: 2 1 10: ^1S^2oo^3K^5ee killed Neko by MOD_RAILGUN
				siz num1, num2, weap;
				if(!(iss >> num1 >> num2 >> weap))
				{
					std::cout << "Error parsing Kill:" << '\n';
					continue;
				}

				if(weap == MOD_SUICIDE)
				{
					// 14:22 Kill: 2 2 20: ^1S^2oo^3K^5ee killed ^1S^2oo^3K^5ee by MOD_SUICIDE
					for(siz i = 0; i < 2; ++i)
					{
						if(dasher[i] == clients[num1])
						{
							dasher[i] = null_guid; // end current dash (if exists)
							dashing[i] = true; // (re) enable dashing
						}
					}
				}
				else if(clients.find(num1) != clients.end() && clients.find(num2) != clients.end())
				{
					if(num1 == 1022) // no killer
						++stats[clients[num2]].deaths[weap];
					else
					{
						++stats[clients[num1]].kills[weap];
						++stats[clients[num2]].deaths[weap];
						++onevone[clients[num1]][clients[num2]];
					}
				}
			}
			else if(cmd == "CTF:")
			{
//				do_rcon("^3CTF:");
				// 10:26 CTF: 0 2 1: ^5A^6lien ^5S^6urf ^5G^6irl captured the BLUE flag!
				// 0 = got, 1 = cap, 2 = ret
				siz num, col, act;
				if(!(iss >> num >> col >> act) || col < 1 || col > 2)
				{
					std::cout << "Error parsing CTF:" << '\n';
					continue;
				}

				--col; // make 0-1 for array index
				siz ncol = col ? 0 : 1;

				str nums_team = "^7[^2U^7]"; // unknown team
				if(teams[clients[num]] == 'R')
					nums_team = "^7[^1R^7]";
				else if(teams[clients[num]] == 'B')
					nums_team = "^7[^4B^7]";

				bug("inc stats");
				++stats[clients[num]].flags[act];

				if(act == FL_CAPTURED) // In Game Announcer
				{
					bug("FL_CAPTURED");
					if(dashing[col] && dasher[col] != null_guid)
					{
						timespec now;
						clock_gettime(CLOCK_REALTIME, &now);

						timespec diff;
						diff.tv_sec = now.tv_sec - dash[col].tv_sec;
						diff.tv_nsec = now.tv_nsec - dash[col].tv_nsec;
						if(diff.tv_nsec < 0)
						{
							diff.tv_nsec += 1000000000;
							--diff.tv_sec;
						}

						double sec = diff.tv_sec + (diff.tv_nsec / 1000000000.0);

						std::ostringstream oss;
						oss.precision(2);
						oss << std::fixed << sec;
						chat(players[clients[num]] + "^3 took ^7" + oss.str()
							+ "^3 seconds to capture the " + flag[col] + "^3 flag.");

						double rec = to<double>(recs["dash." + mapname + ".secs"]);

						bug("rec: " << rec);

						if(rec < 0.5)
						{
							chat(players[clients[num]] + "^3 has set the record for this map.");
							recs["dash." + mapname + ".guid"] = to_string(clients[num]);
							recs["dash." + mapname + ".name"] = players[clients[num]];
							recs["dash." + mapname + ".secs"] = oss.str();
							save_records(recs);
						}
						else if(sec < rec)
						{
							chat(players[clients[num]] + "^3 beat "
								+ recs["dash." + mapname + ".name"] + "'^3s ^7"
								+ recs["dash." + mapname + ".secs"] + " ^3seconds.");
							recs["dash." + mapname + ".guid"] = to_string(clients[num]);
							recs["dash." + mapname + ".name"] = players[clients[num]];
							recs["dash." + mapname + ".secs"] = oss.str();
							save_records(recs);
						}
					}
//					do_rcon("^1DEBUG:^3 End dash & (re)enable dashing of the " + flag[col] + "^3 flag.");
					dasher[col] = null_guid;;
					dashing[col] = true; // new dash now possible
					++flags[col];
					++caps[clients[num]];

					str msg = players[clients[num]] + "^3 has ^7" + to_string(caps[clients[num]]) + "^3 flag" + (caps[clients[num]]==1?"":"s") + "!";
					server.cp(msg);
					skivvy.chat(msg);

					skivvy.chat("^1RED^3: ^7" + to_string(flags[FL_BLUE]) + " ^3v ^4BLUE^3: ^7" + to_string(flags[FL_RED]));
				}
				else if(act == FL_TAKEN)
				{
					bug("FL_TAKEN");
					if(dashing[col])
					{
						clock_gettime(CLOCK_REALTIME, &dash[col]);
					}
					dasher[col] = clients[num];
					skivvy.chat(nums_team + " ^7" + players[clients[num]] + "^3 has taken the " + flag[col] + " ^3flag!");
				}
				else if(act == FL_DROPPED)
				{
					bug("FL_DROPPED");
//					rcon.chat("^1DEBUG:^3 End dash & disable dashing for the " + flag[ncol] + "^3 flag.");
					dasher[ncol] = null_guid;; // end a dash
					dashing[ncol] = false; // no more dashes until return, capture or suicide
					skivvy.chat(nums_team + " ^7" + players[clients[num]] + "^3 has dropped the " + flag[col] + " ^3flag!");
				}
				else if(act == FL_RETURNED)
				{
					bug("FL_RETURNED");
//					rcon.chat("^1DEBUG:^3 (Re)enable dashing for the " + flag[col] + "^3 flag.");
					dasher[col] = null_guid;; // end a dash
					dashing[col] = true; // new dash now possible
					skivvy.chat(nums_team + " ^7" + players[clients[num]] + "^3 has returned the " + flag[col] + " ^3flag!");
				}
			}
			else if(cmd == "Award:")
			{
				// 0:37 Award: 1 3: (drunk)Mosey gained the DEFENCE award!
				siz num, awd;
				if(!(iss >> num >> awd))
				{
					std::cout << "Error parsing Award:" << '\n';
					continue;
				}
				++stats[clients[num]].awards[awd];
			}
		}
		else
		{
			if(cmd == "InitGame:")
			{
				chat("^3InitGame:");

				time = std::time(0);
				in_game = true;
				flags[FL_RED] = 0;
				flags[FL_BLUE] = 0;

				clients.clear();
				players.clear();
				onevone.clear();
				caps.clear();
				stats.clear();
				dasher[FL_RED] = null_guid;;
				dasher[FL_BLUE] = null_guid;;
				dashing[FL_RED] = true;
				dashing[FL_BLUE] = true;

				str msg = "^1K^7at^3i^7na ^3Stats system v^70.1^3-alpha.";
				server.cp(msg);
				skivvy.chat(msg);

				siz pos;
				if((pos = line.find("mapname\\")) != str::npos)
				{
					mapname.clear();
					std::istringstream iss(line.substr(pos + 8));
					if(!std::getline(iss, mapname, '\\'))
					{
						std::cout << "Error parsing mapname\\" << '\n';
						continue;
					}
					bug("mapname: " << mapname);
				}
				skivvy.chat("^3Map: ^7" + mapname + "^3.");
			}
		}
		if(cmd == "say:")
		{
			// 0:23 say: ^5A^6lien ^5S^6urf ^5G^6irl: yes, 3-4 players max
			bug("line: " << line);
			siz pos;
			if((pos = line.find_last_of(':')) != str::npos)
			{
				bug("pos: " << pos);
				if((pos = line.find('!', pos)) != str::npos)
				{
					bug("pos: " << pos);
					str cmd = line.substr(pos);
					bug("cmd: " << cmd);
					if(cmd == "!record")
					{
						bug("mapname: " << mapname);

						chat("^3MAP RECORD: ^7"
							+ recs["dash." + mapname + ".secs"]
							+ "^3 set by ^7" + recs["dash." + mapname + ".name"]);
					}

				}
			}
		}
	}
}
