#ifndef _KATINA_LOG_H_
#define _KATINA_LOG_H_
/*
 *  Created on: 07 Apr 2013
 *      Author: oasookee@gmail.com
 */

/*-----------------------------------------------------------------.
| Copyright (C) 2013 SooKee oasookee@gmail.com                     |
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

#include "ansi.h"
#include "types.h"

namespace katina {

class GUID;
katina::types::sos& operator<<(katina::types::sos& os, const katina::GUID& guid);

namespace log {

using namespace katina::ansi;
using katina::ansi::ANSI;
using namespace katina::types;


// -- LOGGING ------------------------------------------------

#define va(...) __VA_ARGS__
#define ATTS(...) {__VA_ARGS__}

inline str _ansify(const str& s, std::initializer_list<int> codes)
{
	return ansi_esc(codes) + s + norm;
}

#ifndef COLOR_LOGGING
#define ansify(s,c) s
#else
#define ansify(s,c) _ansify(s, c)
#endif

inline
str get_stamp()
{
	time_t rawtime = std::time(0);
	tm* timeinfo = std::localtime(&rawtime);
	char buffer[32];
	std::strftime(buffer, 32, "%Y-%m-%d %H:%M:%S", timeinfo);

	return ansify(str(buffer), ATTS(FG_YELLOW));
}

inline void log_out(sss& s)
{
	static std::mutex mtx;
	lock_guard lock(mtx);
	std::cout << s.str() << std::flush;
}

#define QUOTE(s) #s

#ifndef TRACE
#define trace
#else
#define trace do{std::cout << __FILE__ << ":" << __LINE__ << ":0 trace:";}while(0);
#endif
//../../src/RemoteClient.cpp:238:2: error: expected ‘}’ at end of input
#ifndef DEBUG
#define bug(m)
#define bug_var(v)
#define bug_func()
#define con(m) do{std::cout << m << std::endl;}while(false)
#define log(m) do{sss __s;__s << katina::log::get_stamp() << ": " << m << std::endl;katina::log::log_out(__s);}while(false)
#define nlog(m) do{sss __s;__s << katina::log::get_stamp() << ": " << m << " {" << line_number << "}" << std::endl;katina::log::log_out(__s);}while(false)
#else
inline str _fixfile(const char* f)
{
	str fixed = f;
	siz pos = fixed.find_last_of('/');
	if(pos != str::npos && (pos + 1) < fixed.size())
		fixed = fixed.substr(pos + 1);
	return ansify(fixed, ATTS(FG_GREEN));
}
inline str _fixfunc(const char* f)
{
	// virtual bool katina::plugin::KatinaPluginStats::exit(katina::types::siz, katina::types::siz)
	str fixed = f;
//	return fixed;
	siz pos = fixed.rfind("(");
	if(pos != str::npos)
		pos = fixed.rfind("::", pos);
	if(pos != str::npos)
		pos = fixed.rfind("::", pos - 2);
	if(pos != str::npos && (pos + 2) < fixed.size())
		fixed = fixed.substr(pos + 2);
	return fixed;
}
#define bug(m) do{sss __s;__s << "BUG: " << m << " [" << _fixfile(__FILE__) << "]" << " (" << __LINE__ << ")" << std::endl;katina::log::log_out(__s);}while(false)
#define bug_var(v) bug(QUOTE(v:) << std::boolalpha << " " << v)
struct _
{
	str n;
	str f;
	_(const char* n, const char* f): n(_fixfunc(n)), f(_fixfile(f))
	{
		sss __s;
		__s << ansify("\n---> ", ATTS(BOLD_ON)) << ansify(this->n, ATTS(FG_RED)) << " [" << this->f << "]\n\n";
		katina::log::log_out(__s);
	}
	~_()
	{
		sss __s;
		__s << "\n<--- " << ansify(n, ATTS(FG_RED,FAINT_ON)) << " [" << f << "]\n\n";
		katina::log::log_out(__s);
	}
};
#define bug_func() katina::log::_ __(__PRETTY_FUNCTION__, __FILE__)
#define con(m) do{std::cout << m << " [" << _fixfile(__FILE__) << "]" << " (" << __LINE__ << ")" << std::endl;}while(false)
#define log(m) do{sss __s;__s << katina::log::get_stamp() << ": " << m << " [" << _fixfile(__FILE__) << "]" << " (" << __LINE__ << ")" << std::endl;katina::log::log_out(__s);}while(false)
#define nlog(m) do{sss __s;__s << katina::log::get_stamp() << ": " << m << " {" << line_number << "}" << " [" << _fixfile(__FILE__) << "]" << " (" << __LINE__ << ")" << std::endl;katina::log::log_out(__s);}while(false)
#endif


//#define trace(m)
//#define trace(m) bug("TRACE: " << m << ": " << __LINE__)

inline
bool log_error(const str& m, bool status = false) { log(m); return status; }

enum
{
	LOG_NONE, LOG_NORMAL, LOG_VERBOSE, LOG_DETAILED
};

}} // katina::log

#endif /* _KATINA_LOG_H_ */
