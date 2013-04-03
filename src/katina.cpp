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

#include "codes.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <exception>
#include <stdexcept>
#include <algorithm>

#include <map>
#include <ctime>
#include <cctype>

#include <sys/time.h>
#include <unistd.h>
#include <wordexp.h>

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

#include <pthread.h>

#include "socketstream.h"

#include <mysql.h>

using namespace oastats;

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

typedef std::map<str, int> str_int_map;
typedef str_int_map::iterator str_int_map_iter;
typedef str_int_map::const_iterator str_int_map_citer;
typedef std::pair<const str, int> str_int_map_pair;

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

typedef long milliseconds;

milliseconds get_millitime()
{
	timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	return (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
}

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

inline str& lower(str& s)
{
	std::transform(s.begin(), s.end(), s.begin(), std::ptr_fun<int, int>(std::tolower));
	return s;
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

class GUID
{
	str data;

public:
	const static siz SIZE = 8;

	GUID(): data(SIZE, '0')
	{
		for(siz i = 0; i < SIZE; ++i)
			this->data[i] = '0';
	}

	GUID(const char data[SIZE]): data(SIZE, '0')
	{
		for(siz i = 0; i < SIZE; ++i)
			this->data[i] = data[i];
	}

	GUID(const str& data): data(SIZE, '0')
	{
		for(siz i = 0; i < SIZE && i < data.size(); ++i)
			this->data[i] = data[i];
	}

	GUID(const GUID& guid): data(SIZE, '0')
	{
		for(siz i = 0; i < SIZE; ++i)
			this->data[i] = guid.data[i];
	}

	const GUID& operator=(const GUID& guid)
	{
		for(siz i = 0; i < SIZE; ++i)
			this->data[i] = guid.data[i];
		return *this;
	}

	bool operator==(const GUID& guid) const
	{
		for(siz i = 0; i < SIZE; ++i)
			if(this->data[i] != guid.data[i])
				return false;
		return true;
	}

	bool operator!=(const GUID& guid) const
	{
		return !(*this == guid);
	}

	bool operator<(const GUID& guid) const
	{
		return data < guid.data;
	}

	char& operator[](siz i) { return data[i]; }
	const char& operator[](siz i) const { return data[i]; }
	siz size() const { return SIZE; }

	operator str() const { return data; }

	bool is_bot() const { return data < "00001000"; }
};

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

const GUID null_guid = "00000000";

/*
 * Create a GUID for bots based on their slot number
 */
GUID bot_guid(siz num)
{
	soss oss;
	oss << num;
	str id = oss.str();
	if(id.size() < GUID::SIZE)
		id = str(GUID::SIZE - id.size(), '0') + id;

	return GUID(id.c_str());
}

inline
void thread_sleep_millis(siz msecs)
{
	usleep(msecs * 1000);
}

template<typename T>
str to_string(const T& t, siz width = 0, siz precision = 2)
{
	soss oss;
	oss.setf(std::ios::fixed, std::ios::floatfield);
	oss.width(width);
	oss.precision(precision);
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

	milliseconds timeout = get_millitime() + wait;

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
		::close(cs);
		return false;
	}

	// cs good

	const str msg = "\xFF\xFF\xFF\xFF" + cmd;

	int n = 0;
	if((n = send(cs, msg.c_str(), msg.size(), 0)) < 0 || n < (int)msg.size())
	{
		log("cs send: " << strerror(errno));
		::close(cs);
		return false;
	}

	packets.clear();

	char buf[2048];

	n = sizeof(buf);
	while(n == sizeof(buf))
	{
		while((n = recv(cs, buf, sizeof(buf), MSG_DONTWAIT)) ==  -1 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR))
		{
			if(get_millitime() > timeout)
			{
				log("socket timed out connecting to: " << host << ":" << port);
				::close(cs);
				return false;
			}
			thread_sleep_millis(10);
		}
		if(n < 0)
			log("cs recv: " << strerror(errno));
		if(n > 0)
			packets.push_back(str(buf, n));
	}

	close(cs);
	return true;
}

bool rcon(const str& cmd, str& reply, const str& host, int port, siz wait = TIMEOUT)
{
	str_vec packets;
	if(!aocom(cmd, packets, host, port, wait))
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

typedef std::map<GUID, guid_siz_map> onevone_map;
typedef std::pair<const GUID, guid_siz_map> onevone_pair;
typedef std::map<GUID, guid_siz_map>::iterator onevone_iter;
typedef std::map<GUID, guid_siz_map>::const_iterator onevone_citer;

typedef std::multimap<siz, str> siz_str_mmap;
typedef siz_str_mmap::reverse_iterator siz_str_mmap_ritr;
typedef siz_str_mmap::iterator siz_str_mmap_iter;
typedef siz_str_mmap::const_iterator siz_str_mmap_citer;

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
	//str_set chans;
	typedef std::map<str, std::set<char> > chan_map;
	typedef chan_map::iterator chan_map_iter;
	typedef chan_map::const_iterator chan_map_citer;

	chan_map chans; // #channel -> {'c','f','k'}


public:
	SkivvyClient(): active(false), host("localhost"), port(7334) {}

	void on() { active = true; }
	void off() { active = false; }

	void config(const str& host, siz port)
	{
		for(chan_map_iter chan = chans.begin(); chan != chans.end(); ++chan)
			log("sending to channel: " << chan->first);
		this->host = host;
		this->port = port;
	}

	void set_chans(const str& chans)
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

	bool say(char f, const str& text)
	{
		if(!ss.open(host, port))
		{
			log("error: " << std::strerror(errno) << " host: " << host << " port: " << port);
			return false;
		}
		str res;
		bool good = true;
		for(chan_map_iter chan = chans.begin(); chan != chans.end(); ++chan)
			if(f == '*' || chan->second.count(f))
				good = good && send("/say " + chan->first + " [" + irc_katina + "] " + text, res);
		return good;
	}

	void add_flag(const str& chan, char f) { chans[chan].insert(f); }
	void del_flag(const str& chan, char f) { chans[chan].erase(f); }
	void set_flags(const str& chan, const str& flags)
	{
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

typedef my_ulonglong game_id;
const game_id bad_id(-1);
const game_id null_id(0);

class Database
{
	bool active;

	str host;
	siz port;
	str user;
	str pass;
	str base;

	MYSQL mysql;

public:
	Database(): active(false) { mysql_init(&mysql); }
	~Database() { off(); }

	void config(const str& host, siz port, const str& user, const str& pass, const str& base)
	{
		this->host = host;
		this->port = port;
		this->user = user;
		this->pass = pass;
		this->base = base;
	}

	void on()
	{
		if(active)
			return;
		if(mysql_real_connect(&mysql, host.c_str(), user.c_str()
			, pass.c_str(), base.c_str(), port, NULL, 0) != &mysql)
		{
			log("DATABASE ERROR: Unable to connect; " << mysql_error(&mysql));
			return;
		}
		log("DATABASE: on");
		active = true;
	}

	void off()
	{
		if(!active)
			return;
		active = false;
		mysql_close(&mysql);
		log("DATABASE: off");
	}

	// == DATABASE ==
	//
	//  kills: game_id guid weap count
	// deaths: game_id guid weap count
	//   caps: game_id guid count
	//   time: game_id guid count // seconds in game (player not spec)

	str escape(const str& s)
	{
		char buff[1024];
		return str(buff, mysql_real_escape_string(&mysql, buff, s.c_str(), 1024));
	}
	//   game: game_id host port date map

	game_id add_game(const str& host, const str& port, const str& mapname)
	{
		if(!active)
			return null_id; // inactive

		log("DATABASE: add_game(" << host << ", " << port << ", " << escape(mapname) << ")");

		str sql = "insert into `game`"
			" (`host`, `port`, `map`) values ('"
			+ host + "','" + port + "','" + mapname + "')";

		if(mysql_real_query(&mysql, sql.c_str(), sql.length()))
		{
			log("DATABASE ERROR: Unable to add_mame; " << mysql_error(&mysql));
			log("              : sql = " << sql);
			return bad_id;
		}

		return mysql_insert_id(&mysql);
	}

	/**
	 *
	 * @param id
	 * @param table "kills" | "deaths"
	 * @param guid
	 * @param weap
	 * @param count
	 * @return
	 */
	bool add_weaps(game_id id, const str& table, const GUID& guid, siz weap, siz count)
	{
		if(!active)
			return true; // not error

		log("DATABASE: add_weaps(" << id << ", " << table << ", " << guid << ", " << weap << ", " << count << ")");

		soss oss;
		oss << "insert into `" << table << "` (`game_id`, `guid`, `weap`, `count`) values (";
		oss << "'" << id << "','" << guid << "','" << weap << "','" << count << "')";

		str sql = oss.str();

		if(mysql_real_query(&mysql, sql.c_str(), sql.length()))
		{
			log("DATABASE ERROR: Unable to add_weaps; " << mysql_error(&mysql));
			log("              : sql = " << sql);
			return false;
		}

		return true;
	}

	bool add_caps(game_id id, const GUID& guid, siz count)
	{
		if(!active)
			return true; // not error

		log("DATABASE: add_caps(" << id << ", " << guid << ", " << count << ")");

		soss oss;
		oss << "insert into `caps` (`game_id`, `guid`, `count`) values (";
		oss << "'" << id << "','" << guid << "','" << count << "')";

		str sql = oss.str();

		if(mysql_real_query(&mysql, sql.c_str(), sql.length()))
		{
			log("DATABASE ERROR: Unable to add_caps; " << mysql_error(&mysql));
			log("              : sql = " << sql);
			return false;
		}

		return true;
	}

	bool add_player(const GUID& guid, const str& name)
	{
		if(!active)
			return true; // not error

		log("DATABASE: add_player(" << guid << ", " << name << ")");

		soss oss;
		oss << "insert into `player` (`guid`,`name`) values ('" << guid << "','" << escape(name)
			<< "') ON DUPLICATE KEY UPDATE count = count + 1";

		str sql = oss.str();

		if(mysql_real_query(&mysql, sql.c_str(), sql.length()))
		{
			log("DATABASE ERROR: Unable to add_player; " << mysql_error(&mysql));
			log("              : sql = " << sql);
			return false;
		}

		return true;
	}

	bool add_vote(const str& type, const str& item, const GUID& guid, int count)
	{
		if(!active)
			return true; // not error

		log("DATABASE: add_vote(" << type << ", " << item << ", " << guid << ", " << count << ")");

		soss oss;
		oss << "insert into `votes` (`type`,`item`,`guid`,`count`) values ('"
			<< type << "','" << item << "','" << guid << "','" << count << "')";

		str sql = oss.str();

		if(mysql_real_query(&mysql, sql.c_str(), sql.length()))
		{
			log("DATABASE ERROR: Unable to add_player; " << mysql_error(&mysql));
			log("              : sql = " << sql);
			return false;
		}

		return true;
	}

	bool read_recs(str_map& recs)
	{
//		recs["vote." + mapname + "." + str(i->first)] = to_string(i->second);
		if(!active)
			return true; // not error

		log("DATABASE: read_recs()");

		soss oss;
		oss << "select `guid`,`map`,`count` from `votes` where `type` = 'map'";

		str sql = oss.str();

		if(mysql_real_query(&mysql, sql.c_str(), sql.length()))
		{
			log("DATABASE ERROR: Unable to read_recs; " << mysql_error(&mysql));
			return false;
		}

		MYSQL_RES* result = mysql_store_result(&mysql);

		MYSQL_ROW row;
		while((row = mysql_fetch_row(result)))
		{
			log("DATABASE: restoring vote: " << row[1] << ", " << row[2] << ", " << row[3]);
			recs["vote." + str(row[1]) + "." + str(row[2])] = str(row[3]);
		}
		mysql_free_result(result);
		return true;
	}
};

RCon server;
SkivvyClient skivvy;
Database db;

/**
 * Set a variable from a cvar using rcon.
 * @param cvar The name of the cvar whose value is wanted
 * @param val The variable to set to the cvar's value.
 */
bool rconset(const str& cvar, str& val)
{
	str response;
	if(!server.command(cvar, response))
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
 */
template<typename T>
bool rconset(const str& cvar, T& val)
{
	str sval;
	if(!rconset(cvar, sval))
		return false;
	siss iss(sval);
	return iss >> val;
}

str_vec weapons;

// teams thread

pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
struct thread_data
{
	milliseconds delay;
	siz_guid_map* clients_p;
	guid_siz_map* teams_p;
};

bool done = false;

//std::set<siz> db_weaps;
//db_weaps.insert(MOD_RAILGUN);
//db_weaps.insert(MOD_GAUNTLET);

struct katina_conf
{
	bool active;
	bool do_flags;
	bool do_dashes;
	bool do_db; // do database writes
	std::set<siz> db_weaps; // which weapons to record

	katina_conf()
	: active(false)
	, do_flags(false)
	, do_dashes(false)
	, do_db(false)
	{
	}
};

struct skivvy_conf
{
	bool active;
	bool do_flags;
	bool do_chats;
	bool do_kills;
	bool do_infos;
	bool do_stats;
	bool spamkill;
	str chans;

	skivvy_conf()
	: active(false)
	, do_flags(false)
	, do_chats(false)
	, do_kills(false)
	, do_infos(false)
	, do_stats(false)
	, spamkill(false)
	{
	}
};


//bool katina_active = false;
katina_conf ka_cfg;
skivvy_conf sk_cfg;

str_map recs; // high scores/ config etc

siz_guid_map clients; // slot -> GUID
guid_str_map players; // GUID -> name
onevone_map onevone; // GUID -> GUID -> <count> //
guid_siz_map caps; // GUID -> <count> // TODO: caps container duplicated in stats::caps
guid_stat_map stats; // GUID -> <stat>
guid_siz_map teams; // GUID -> 'R' | 'B'
str mapname, old_mapname; // current/previous map name

typedef std::map<GUID, int> guid_int_map;
typedef std::pair<const GUID, int> guid_int_map_pair;
typedef guid_int_map::iterator guid_int_map_iter;
typedef guid_int_map::const_iterator guid_int_map_citer;

guid_int_map map_votes; // GUID -> 3

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

	server.chat("^5== ^6RESULTS ^5" + str(max - 23, '='));
	for(siz i = 0; i < results.size(); ++i)
		server.chat(results[i]);
	server.chat("^5" + str(max - 12, '-'));

	if(sk_cfg.do_infos)
	{
		skivvy.chat('i', "^5== ^6RESULTS ^5" + str(max - 23, '='));
		for(siz i = 0; i < results.size(); ++i)
			skivvy.chat('f', results[i]);
		skivvy.chat('i', "^5" + str(max - 12, '-'));
	}
}

siz map_get(const siz_map& m, siz key)
{
	return m.find(key) == m.end() ? 0 : m.at(key);
}

void report_stats(const guid_stat_map& stats, const guid_str_map& players)
{
	std::multimap<double, str> skivvy_scores;
	//std::multimap<double, str>::iterator i;
	for(guid_stat_citer p = stats.begin(); p != stats.end(); ++p)
	{
		const str& player = players.at(p->first);
		con("player: " << player);
		con("\t  caps: " << map_get(p->second.flags, FL_CAPTURED));
		con("\t kills: " << map_get(p->second.kills, MOD_RAILGUN));
		con("\tdeaths: " << map_get(p->second.deaths, MOD_RAILGUN));
		con("\t  defs: " << map_get(p->second.awards, AW_DEFENCE));
		con("\t gaunt: " << map_get(p->second.awards, AW_GAUNTLET));
		// TODO: modify this to add AW options as well as insta
		if(sk_cfg.do_stats)
		{
			siz c = map_get(p->second.flags, FL_CAPTURED);
			siz k = map_get(p->second.kills, MOD_RAILGUN);
			k += map_get(p->second.kills, MOD_GAUNTLET);
			siz d = map_get(p->second.deaths, MOD_RAILGUN);
			d += map_get(p->second.deaths, MOD_GAUNTLET);

			con("c: " << c);
			con("k: " << k);
			con("d: " << d);

			double rkd = 0.0;
			double rcd = 0.0;
			str kd, cd;
			if(!d)
			{
				if(k)
					kd = "perf  ";
				if(c)
					cd = "perf  ";
			}
			else
			{
				rkd = double(k) / d;
				rcd = double(c * 100) / d;
				kd = to_string(rkd, 6);
				cd = to_string(rcd, 6);
			}
			if(k || c || d)
				skivvy_scores.insert(std::make_pair(rkd, "^3kills^7/^3d ^5(^7" + kd + "^5) ^3caps^7/^3d ^5(^7" + cd + "^5)^7: " + player));
		}
	}
	if(sk_cfg.do_stats)
		for(std::multimap<double, str>::reverse_iterator r = skivvy_scores.rbegin(); r != skivvy_scores.rend(); ++r)
			skivvy.chat('s', r->second);
}

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

void* set_teams(void* td_vp)
{
	thread_data& td = *reinterpret_cast<thread_data*>(td_vp);
	siz_guid_map& clients = *td.clients_p;
	guid_siz_map& teams = *td.teams_p;

	if(td.delay < 3000)
		td.delay = 3000;

	while(!done)
	{
		thread_sleep_millis(td.delay);

		// cvar controls

		static siz c = 0;

		katina_conf old_ka_cfg = ka_cfg;
		skivvy_conf old_sk_cfg = sk_cfg;
		str cvar;
		siss iss;
		siz weap;

		switch(c++)
		{
			case 0:
				if(!rconset("katina_active", ka_cfg.active))
					rconset("katina_active", ka_cfg.active); // one retry
				if(ka_cfg.active != old_ka_cfg.active)
				{
					log("katina: " + str(ka_cfg.active?"":"de-") + "activated");
					server.chat("^3going ^1" + str(ka_cfg.active?"on":"off") + "-line^3.");
					skivvy.chat('*', "^3going ^1" + str(ka_cfg.active?"on":"off") + "-line^3.");
				}
			break;
			case 1:
				if(!rconset("katina_flags", ka_cfg.do_flags))
					rconset("katina_flags", ka_cfg.do_flags); // one retry
				if(ka_cfg.do_flags != old_ka_cfg.do_flags)
				{
					log("katina: flag counting is now: " << (ka_cfg.do_flags ? "on":"off"));
					server.chat( "^3Flag countng ^1" + str(ka_cfg.do_flags ? "on":"off") + "^3.");
					skivvy.chat('f', "^3Flag countng ^1" + str(ka_cfg.do_flags ? "on":"off") + "^3.");
				}
			break;
			case 2:
				if(!rconset("katina_dashes", ka_cfg.do_dashes))
					rconset("katina_dashes", ka_cfg.do_dashes); // one retry
				if(ka_cfg.do_dashes != old_ka_cfg.do_dashes)
				{
					log("katina: flag timing is now: " << (ka_cfg.do_dashes ? "on":"off"));
					server.chat("^3Flag timing ^1" + str(ka_cfg.do_dashes ? "on":"off") + "^3.");
					skivvy.chat('f', "^3Flag timing ^1" + str(ka_cfg.do_dashes ? "on":"off") + "^3.");
				}
			break;
			case 3:
				if(!rconset("katina_db_active", ka_cfg.do_db))
					rconset("katina_db_active", ka_cfg.do_db); // one retry
				if(ka_cfg.do_db != old_ka_cfg.do_db)
				{
					log("katina: database writing is now: " << (ka_cfg.do_db ? "on":"off"));
					skivvy.chat('*', "^3Flag timing ^1" + str(ka_cfg.do_db ? "on":"off") + "^3.");
					if(!ka_cfg.do_db)
						db.off();
					else
					{
						db.on();
//						recs["vote." + mapname + "." + str(i->first)] = to_string(i->second);
						db.read_recs(recs);
					}
				}
			break;
			case 4:
				if(!rconset("katina_db_weaps", cvar))
					if(!rconset("katina_db", cvar))
						break;

				iss.clear();
				iss.str(cvar);
				while(iss >> weap)
					ka_cfg.db_weaps.insert(weap);

				if(ka_cfg.db_weaps != old_ka_cfg.db_weaps)
				{
					log("katina: database weaps set to: " << cvar);
					skivvy.chat('*', "^3Database weapons set to: ^1" + cvar + "^3.");
				}
			break;
			case 5:
				if(!rconset("katina_skivvy_active", sk_cfg.active))
					rconset("katina_skivvy_active", sk_cfg.active); // one retry
				if(sk_cfg.active != old_sk_cfg.active)
				{
					if(sk_cfg.active)
					{
						log("skivvy: reporting activated");
						skivvy.chat('*', "^3reporting turned on.");
						skivvy.on();
					}
					else
					{
						log("skivvy: reporting deactivated");
						skivvy.chat('*', "^3reporting turned off.");
						skivvy.off();
					}
				}
			break;
			case 6:
				if(!rconset("katina_skivvy_chats", sk_cfg.do_chats))
					rconset("katina_skivvy_chats", sk_cfg.do_chats); // one retry
				if(sk_cfg.do_chats != old_sk_cfg.do_chats)
				{
					log("skivvy: chat reporting is now: " << (sk_cfg.do_chats ? "on":"off"));
					skivvy.chat('*', "^3Chat reports ^1" + str(sk_cfg.do_chats ? "on":"off") + "^3.");
				}
			break;
			case 7:
				if(!rconset("katina_skivvy_flags", sk_cfg.do_flags))
					rconset("katina_skivvy_flags", sk_cfg.do_flags); // one retry
				if(sk_cfg.do_flags != old_sk_cfg.do_flags)
				{
					log("skivvy: flag reporting is now: " << (sk_cfg.do_flags ? "on":"off"));
					skivvy.chat('*', "^3Flag reports ^1" + str(sk_cfg.do_flags ? "on":"off") + "^3.");
				}
			break;
			case 8:
				if(!rconset("katina_skivvy_kills", sk_cfg.do_kills))
					rconset("katina_skivvy_kills",sk_cfg. do_kills); // one retry
				if(sk_cfg.do_kills != old_sk_cfg.do_kills)
				{
					log("skivvy: kill reporting is now: " << (sk_cfg.do_kills ? "on":"off"));
					skivvy.chat('*', "^3Kill reports ^1" + str(sk_cfg.do_kills ? "on":"off") + "^3.");
				}
			break;
			case 9:
				if(!rconset("katina_skivvy_infos", sk_cfg.do_infos))
					rconset("katina_skivvy_infos", sk_cfg.do_infos); // one retry
				if(sk_cfg.do_kills != old_sk_cfg.do_kills)
				{
					log("skivvy: info reporting is now: " << (sk_cfg.do_infos ? "on":"off"));
					skivvy.chat('*', "^3info reports ^1" + str(sk_cfg.do_infos ? "on":"off") + "^3.");
				}
			break;
			case 10:
				if(!rconset("katina_skivvy_stats", sk_cfg.do_stats))
					rconset("katina_skivvy_stats", sk_cfg.do_stats); // one retry
				if(sk_cfg.do_stats != old_sk_cfg.do_stats)
				{
					log("skivvy: stats reporting is now: " << (sk_cfg.do_stats ? "on":"off"));
					skivvy.chat('*', "^3stats reports ^1" + str(sk_cfg.do_stats ? "on":"off") + "^3.");
				}
			break;
			case 11:
				if(!rconset("katina_skivvy_stats", sk_cfg.do_stats))
					rconset("katina_skivvy_stats", sk_cfg.do_stats); // one retry
				if(sk_cfg.do_stats != old_sk_cfg.do_stats)
				{
					log("skivvy: stats reporting is now: " << (sk_cfg.do_stats ? "on":"off"));
					skivvy.chat('*', "^3stats reports ^1" + str(sk_cfg.do_stats ? "on":"off") + "^3.");
				}
			break;
			case 12:
				if(!rconset("katina_skivvy_spamkill", sk_cfg.spamkill))
					rconset("katina_skivvy_spamkill", sk_cfg.spamkill); // one retry
				if(old_sk_cfg.spamkill != sk_cfg.spamkill)
				{
					log("skivvy: spamkill is now: " << (sk_cfg.spamkill ? "on":"off"));
					skivvy.chat('*', "^3spamkill ^1" + str(sk_cfg.spamkill ? "on":"off") + "^3.");
				}
			break;
			default:
				c = 0;
			break;
		}

		if(!sk_cfg.active || !sk_cfg.active)
			continue;

		str reply;
		if(server.command("!listplayers", reply))
		{
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
				while(std::getline(iss, line))
				{
					//bug("\t\tline: " << line);
					siss iss(line);
					if(iss >> n >> team)
					{
						pthread_mutex_lock(&mtx);
						teams[clients[n]] = team;
						pthread_mutex_unlock(&mtx);
					}
				}
			}
		}
	}
	pthread_exit(0);
}

GUID guid_from_name(const str& name)
{
	for(guid_str_iter i = players.begin(); i != players.end(); ++i)
		if(i->second == name)
			return i->first;
	return null_guid;
}

bool extract_name_from_text(const str& line, GUID& guid, str& text)
{
//	bug("extract_name_from_text: " << line);
	// longest ": " to ": " substring that exixts in names database
	GUID candidate;
	siz pos = 0;
	siz beg = 0;
	if((beg = line.find(": ")) == str::npos) // "say: "
		return false;

	beg += 2;
//	bug("beg: " << beg);

	bool found = false;
	for(pos = beg; (pos = line.find(": ", pos)) != str::npos; pos += 2)
	{
//		bug("beg: " << beg);
//		bug("pos: " << pos);
		if((candidate = guid_from_name(line.substr(beg, pos - beg))) != null_guid)
		{
//			bug("candidate: " << candidate);
			guid = candidate;
			text = line.substr(pos + 2);
			found = true;
		}
	}
	return found;
}

str expand_env(const str& var)
{
	str exp;
	wordexp_t p;
	wordexp(var.c_str(), &p, 0);
	if(p.we_wordc)
		exp = p.we_wordv[0];
	wordfree(&p);
	return exp;
}

int main(const int argc, const char* argv[])
{
	load_records(recs);

	log("Records loaded: " << recs.size());

	sifs ifs;
	if(argc > 1)
		ifs.open(argv[1], std::ios::ate);
	else if(!recs["logfile"].empty())
		ifs.open(expand_env(recs["logfile"]).c_str(), std::ios::ate);

	sis& is = ifs.is_open() ? ifs : std::cin;

	if(!is)
	{
		log("Input error:");
		return -2;
	}

	server.config(recs["rcon.host"], to<siz>(recs["rcon.port"]), recs["rcon.pass"]);
	skivvy.config(recs["skivvy.host"], to<siz>(recs["skivvy.port"]));
	db.config(recs["db.host"], to<siz>(recs["db.port"]), recs["db.user"], recs["db.pass"], recs["db.base"]);

	server.chat("^3Stats System v^70.1^3-alpha - ^1ONLINE");
	skivvy.chat('*', "^3Stats System v^70.1^3-alpha - ^1ONLINE");

	// weapons
	weapons.push_back("unknown weapon");
	weapons.push_back("shotgun");
	weapons.push_back("gauntlet");
	weapons.push_back("machinegun");
	weapons.push_back("grenade");
	weapons.push_back("grenade schrapnel");
	weapons.push_back("rocket");
	weapons.push_back("rocket blast");
	weapons.push_back("plasma");
	weapons.push_back("plasma splash");
	weapons.push_back("railgun");
	weapons.push_back("lightening");
	weapons.push_back("BFG");
	weapons.push_back("BFG fallout");
	weapons.push_back("dround");
	weapons.push_back("slimed");
	weapons.push_back("burnt up in lava");
	weapons.push_back("crushed");
	weapons.push_back("telefraged");
	weapons.push_back("falling to far");
	weapons.push_back("suicide");
	weapons.push_back("target lazer");
	weapons.push_back("inflicted pain");
	weapons.push_back("nailgun");
	weapons.push_back("chaingun");
	weapons.push_back("proximity mine");
	weapons.push_back("kamikazi");
	weapons.push_back("juiced");
	weapons.push_back("grappled");

	std::time_t time = 0;
	char c;
	siz m, s;
	str skip, name, cmd, stamp;
	siz secs = 0;
	std::istringstream iss;
	bool in_game = false;
	bug("in_game: " << in_game);

	siz flags[2];

	milliseconds dash[2];// = {0, 0}; // time of dash start
	GUID dasher[2]; // who is dashing
	bool dashing[2] = {true, true}; // flag dash in progress?

	const str flag[2] = {"^1RED", "^4BLUE"};

	pthread_t teams_thread;
	milliseconds thread_delay = 6000; // default
	if(recs.count("rcon.delay"))
		thread_delay = to<milliseconds>(recs["rcon.delay"]);
	thread_data td = {thread_delay, &clients, &teams};
	pthread_create(&teams_thread, NULL, &set_teams, (void*) &td);

	milliseconds sleep_time = 100; // milliseconds
	bool done = false;
	std::ios::streampos pos = is.tellg();
	str line;
	while(!done)
	{
		if(ka_cfg.do_dashes)
			sleep_time = 10;
		else
			sleep_time = 100;

		if(!std::getline(is, line) || is.eof())
			{ thread_sleep_millis(sleep_time); is.clear(); is.seekg(pos); continue; }

		pos = is.tellg();

		if(!sk_cfg.active)
			continue;

//		bug("line: " << line);

		iss.clear();
		iss.str(line);
		iss >> m >> c >> s >> cmd;
		secs = (m * 60) + s;
//		bug("cmd: " << cmd);
		if(in_game)
		{
			if(cmd == "Exit:")
			{
				// shutdown voting until next map
				log("exit: writing stats to database and collecting votes");
				str reply;
				if(!server.command("set g_allowVote 0", reply))
					server.command("set g_allowVote 0", reply); // one retry

				skivvy.chat('*', "^3Game Over");
				in_game = false;
				bug("in_game: " << in_game);

				try
				{
					if(ka_cfg.do_flags && !caps.empty())
						report_caps(caps, players);

					game_id id = db.add_game(recs["rcon.host"], recs["rcon.port"], mapname);
					bug("id; " << id);
					if(id != null_id && id != bad_id)
					{
						// TODO: insert game stats here
						for(guid_stat_citer p = stats.begin(); p != stats.end(); ++p)
						{
							const str& player = players.at(p->first);

							siz count;
							for(std::set<siz>::iterator weap = ka_cfg.db_weaps.begin(); weap != ka_cfg.db_weaps.end(); ++weap)
							{
								if((count = map_get(p->second.kills, *weap)))
									db.add_weaps(id, "kills", p->first, *weap, count);
								if((count = map_get(p->second.deaths, *weap)))
									db.add_weaps(id, "deaths", p->first, *weap, count);
							}

							if((count = map_get(p->second.flags, FL_CAPTURED)))
								db.add_caps(id, p->first, count);
						}
					}

					// report
					con("-- Report: -------------------------------");
					report_clients(clients);
					con("");
					report_players(players);
					con("");
					report_onevone(onevone, players);
					con("");
					report_stats(stats, players);
					con("------------------------------------------");
//
//					// TODO: make votes db only
//					for(guid_int_map_iter i = map_votes.begin(); i != map_votes.end(); ++i)
//					{
//						db.add_vote("map", mapname, i->first, i->second);
//						recs["vote." + mapname + "." + str(i->first)] = to_string(i->second);
//					}
//
//					save_records(recs); // TODO: remoe this when vots only in db
//					map_votes.clear();
				}
				catch(std::exception& e)
				{
					con(e.what());
				}

				for(guid_str_map::iterator player = players.begin(); player != players.end(); ++player)
					if(!player->first.is_bot())
						db.add_player(player->first, player->second);

				log("exit: done");
			}
			else if(cmd == "ShutdownGame:")
			{
				bug("ShutdownGame:");
				in_game = false;
				bug("in_game: " << in_game);

			}
			else if(cmd == "Warmup:")
			{
				bug("Warmup:");
				in_game = false;
				bug("in_game: " << in_game);
			}
			else if(cmd == "ClientUserinfoChanged:")
			{
				//bug("ClientUserinfoChanged:");
				//do_rcon("^3ClientUserinfoChanged:");
				// 0:23 ClientUserinfoChanged: 2 n\^1S^2oo^3K^5ee\t\2\model\ayumi/red\hmodel\ayumi/red\g_redteam\\g_blueteam\\c1\1\c2\1\hc\100\w\0\l\0\tt\0\tl\1\id\1A7C66BACBA16F0C9068D8B82C1D55DE
				siz num;
				if(!std::getline(std::getline(iss >> num, skip, '\\'), name, '\\'))
				{
					std::cout << "Error parsing ClientUserinfoChanged: "  << line << '\n';
					continue;
				}

				//bug("num: " << num);
				//bug("skip: " << skip);
				//bug("name: " << name);

				siz pos = line.find("id\\");
				if(pos != str::npos)
				{
					str guid = line.substr(pos + 3, 32);

					if(guid.size() != 32)
						clients[num] = bot_guid(num);//null_guid;
					else
						clients[num] = to<GUID>(guid.substr(24));

					players[clients[num]] = name;

					if(stats[clients[num]].first_seen)
						stats[clients[num]].logged_time += std::time(0) - stats[clients[num]].first_seen;
					stats[clients[num]].first_seen = time + secs;
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
					if(num1 == 1022 && !clients[num2].is_bot()) // no killer
						++stats[clients[num2]].deaths[weap];
					else if(!clients[num1].is_bot() && !clients[num2].is_bot())
					{
							++stats[clients[num1]].kills[weap];
							++stats[clients[num2]].deaths[weap];
							++onevone[clients[num1]][clients[num2]];

						if(sk_cfg.do_kills)
							skivvy.chat('k', "^7" + players[clients[num1]] + " ^4killed ^7" + players[clients[num2]]
								+ " ^4with a ^7" + weapons[weap]);
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
				str nums_nteam = "^7[^2U^7]"; // unknown team

				pthread_mutex_lock(&mtx);
				if(teams[clients[num]] == 'R')
					nums_team = "^7[^1R^7]";
				else if(teams[clients[num]] == 'B')
					nums_team = "^7[^4B^7]";
				if(teams[clients[num]] == 'B')
					nums_nteam = "^7[^1R^7]";
				else if(teams[clients[num]] == 'R')
					nums_nteam = "^7[^4B^7]";
				pthread_mutex_unlock(&mtx);

				//bug("inc stats");
				if(!clients[num].is_bot())
					++stats[clients[num]].flags[act];

				if(act == FL_CAPTURED) // In Game Announcer
				{
					bug("FL_CAPTURED");
					if(ka_cfg.do_dashes && dashing[col] && dasher[col] != null_guid)
					{
						double sec = (get_millitime() - dash[col]) / 1000.0;

						std::ostringstream oss;
						oss.precision(2);
						oss << std::fixed << sec;
						server.chat(players[clients[num]] + "^3 took ^7" + oss.str()
							+ "^3 seconds to capture the " + flag[col] + "^3 flag.");
						if(sk_cfg.do_flags)
							skivvy.chat('f', players[clients[num]] + "^3 took ^7" + oss.str()
								+ "^3 seconds to capture the " + flag[col] + "^3 flag.");

						double rec = to<double>(recs["dash." + mapname + ".secs"]);

						bug("rec: " << rec);

						if(rec < 0.5)
						{
							server.chat(players[clients[num]] + "^3 has set the record for this map.");
							skivvy.chat('f', players[clients[num]] + "^3 has set the record for this map.");
							recs["dash." + mapname + ".guid"] = to_string(clients[num]);
							recs["dash." + mapname + ".name"] = players[clients[num]];
							recs["dash." + mapname + ".secs"] = oss.str();
							save_records(recs);
						}
						else if(sec < rec)
						{
							server.chat(players[clients[num]] + "^3 beat ^7"
								+ recs["dash." + mapname + ".name"] + "'^3s ^7"
								+ recs["dash." + mapname + ".secs"] + " ^3seconds.");
							skivvy.chat('f', players[clients[num]] + "^3 beat ^7"
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

					if(ka_cfg.do_flags)
					{
						str msg = players[clients[num]] + "^3 has ^7" + to_string(caps[clients[num]]) + "^3 flag" + (caps[clients[num]]==1?"":"s") + "!";
						server.cp(msg);
						if(sk_cfg.do_flags)
						{
							skivvy.chat('f', msg);
							skivvy.chat('f', "^1RED^3: ^7" + to_string(flags[FL_BLUE]) + " ^3v ^4BLUE^3: ^7" + to_string(flags[FL_RED]));
						}
					}
				}
				else if(act == FL_TAKEN)
				{
					if(dashing[col])
						dash[col] = get_millitime();

					dasher[col] = clients[num];

					if(sk_cfg.do_flags)
						skivvy.chat('f', nums_team + " ^7" + players[clients[num]] + "^3 has taken the " + flag[col] + " ^3flag!");
				}
				else if(act == FL_DROPPED)
				{
					if(sk_cfg.do_flags)
					{
						skivvy.chat('f', nums_team + " ^7" + players[clients[num]] + "^3 has killed the " + flag[col] + " ^3flag carrier!");
						skivvy.chat('f', nums_nteam + " ^7" + players[dasher[ncol]] + "^3 has dropped the " + flag[ncol] + " ^3flag!");
					}
					dasher[ncol] = null_guid;; // end a dash
					dashing[ncol] = false; // no more dashes until return, capture or suicide
				}
				else if(act == FL_RETURNED)
				{
					if(sk_cfg.do_flags)
						skivvy.chat('f', nums_team + " ^7" + players[clients[num]] + "^3 has returned the " + flag[col] + " ^3flag!");

					dasher[col] = null_guid;; // end a dash
					dashing[col] = true; // new dash now possible
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
				bug("InitGame:");


				// SAVE mapvotes from the previous game (if any)
				// We do this here because if the previous map was voted off
				// end of map processing will have been avoided.

				// TODO: make votes db only
				for(guid_int_map_iter i = map_votes.begin(); i != map_votes.end(); ++i)
				{
					db.add_vote("map", mapname, i->first, i->second);
					recs["vote." + mapname + "." + str(i->first)] = to_string(i->second);
				}

				save_records(recs); // TODO: remoe this when vots only in db
				map_votes.clear();

				// -----------------

				time = std::time(0);
				in_game = true;
				bug("in_game: " << in_game);

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

				str msg = "^1K^7at^3i^7na ^3Stats System v^70.1^3-alpha.";
				server.cp(msg);
				// startup voting
				str reply;
				if(!server.command("set g_allowVote 1", reply))
					server.command("set g_allowVote 1", reply); // do 1 retry


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

					// load map votes
					// TODO: FIX map voting bypases normal end of map
					// so votes get lost
					map_votes.clear();
					for(str_map_iter i = recs.begin(); i != recs.end(); ++i)
						if(!(i->first.find("vote." + mapname + ".")))
						{
							str guid = i->first.substr(5 + mapname.size() + 1);
							bug("restoring vote: " << guid << " to " << i->second);
							map_votes[GUID(guid)] = to<siz>(i->second);
						}
					// recs["vote." + mapname + "." + str(i->first)] = to_string(i->second);
				}
				if(sk_cfg.do_infos && mapname != old_mapname)
				{
					skivvy.chat('i', "^3== Playing Map: ^7" + mapname + "^3 ==");
					old_mapname = mapname;
				}
			}
		}
		if(cmd == "say:")
		{
			// 0:23 say: ^5A^6lien ^5S^6urf ^5G^6irl: yes, 3-4 players max
			bug("line: " << line);

			static str_set spam;

			if(sk_cfg.do_chats)
			{
				str text;
				if(std::getline(iss >> std::ws, text))
					if(!sk_cfg.spamkill || !spam.count(text))
						skivvy.chat('c', "^7say: " + *spam.insert(spam.end(), text));
			}

			siz pos;
			if((pos = line.find_last_of(':')) == str::npos)
				continue;
			if((pos = line.find('!', pos)) == str::npos)
				continue;

			siss iss(line.substr(pos));
			str cmd;
			iss >> cmd;
			bug("cmd: " << cmd);
			lower(cmd);

			if(cmd == "!record")
			{
				con("!record");
				server.chat("^3MAP RECORD: ^7"
					+ recs["dash." + mapname + ".secs"]
					+ "^3 set by ^7" + recs["dash." + mapname + ".name"]);
			}
			else if(cmd == "!love") // TODO:
			{
				con("!love");
				str love;
				GUID guid;
				if(!extract_name_from_text(line, guid, love))
					continue;
				iss >> love;
				con("guid: " << guid);
				con("love: " << love);

				if(lower(trim(love)) == "map")
				{
					if(map_votes.count(guid))
						server.chat("^3You can only vote once per week.");
					else
					{
						++map_votes[guid];
						server.chat("^7" + players[guid] + "^7: ^3Your vote has been counted.");
					}
				}
			}
			else if(cmd == "!hate") // TODO:
			{
				con("!hate");
				str hate;
				GUID guid;
				if(!extract_name_from_text(line, guid, hate))
					continue;

				iss >> hate;
				con("guid: " << guid);
				con("hate: " << hate);

				if(lower(trim(hate)) == "map")
				{
					if(map_votes.count(guid))
						server.chat("^3You can only vote once per week.");
					else
					{
						--map_votes[guid];
						server.chat("^7" + players[guid] + "^7: ^3Your vote has been counted.");
					}
				}
			}
		}
	}
	done = true;
	pthread_join(teams_thread, 0);
}
