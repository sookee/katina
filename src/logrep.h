#pragma once
#ifndef _OASTATS_LOGREP_H_
#define _OASTATS_LOGREP_H_
/*
 * logrep.h
 *
 *  Created on: 1 Aug 2011
 *      Author: oasookee@gmail.com
 */

/*-----------------------------------------------------------------.
| Copyright (C) 2011 SooKee oasookee@gmail.com               |
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

#include <iostream>
#include <ctime>
#include <cmath>
#include <algorithm>

#include <map>
#include <set>
#include <deque>
#include <stack>
#include <vector>
#include <random>

namespace oastats { namespace utils {

using namespace oastats::types;

/*
 * Basic bug reporting.
 */
inline
std::ostream& botbug(std::ostream* os = 0)
{
	static std::ostream* osp = 0;

	if(!osp) // initialize
		if(!os)
			osp = &std::cout;
	if(os) // change
		osp = os;
	return *osp;
}

str thread_name();
str obj_name(void* id);

#define THREAD oastats::utils::thread_name()
#define OBJECT oastats::utils::obj_name(this)

#ifndef DEBUG
#define bug(m)
#define bug_msg(m)
#define bug_var(v)
#define bug_func()
#define BUG_COMMAND(m)
#else
#define bug(m) do{oastats::utils::botbug() << m << std::endl;}while(false)

struct __scope__bomb__
{
	const char* name;
	__scope__bomb__(const char* name): name(name) { bug("---> " << name << ' ' << THREAD << OBJECT); }
	~__scope__bomb__() { bug("<--- " << name << ' ' << THREAD << OBJECT); }
};

#define QUOTE(s) #s
#define bug_msg(m) do{printmsg(botbug(), (m));}while(false)
#define bug_var(v) bug(QUOTE(v:) << std::boolalpha << " " << v)

#define bug_func() __scope__bomb__ __scoper__(__PRETTY_FUNCTION__)

#define BUG_COMMAND(m) \
bug_func(); \
bug("-----------------------------------------------------"); \
bug_msg(msg); \
bug("-----------------------------------------------------")

#endif

#ifndef DEV
#define DEV "-unset"
#endif

#ifndef STAMP
#define STAMP "00000000"
#endif

#ifndef COMMITS
#define COMMITS "0000"
#endif

#ifndef REVISION
#define REVISION "0000000"
#endif

#define PLUGIN_INFO(N, V) \
	static const char* NAME = N; \
	static const char* VERSION = V "-" STAMP "-" COMMITS "-" REVISION DEV

/*
 * Basic logging.
 */

inline
std::string get_stamp()
{
	time_t rawtime = std::time(0);
	tm* timeinfo = std::localtime(&rawtime);
	char buffer[32];
	std::strftime(buffer, 32, "%Y-%m-%d %H:%M:%S", timeinfo);

	return std::string(buffer);
}

inline
std::ostream& botlog(std::ostream* os = 0)
{
	static std::ostream* osp = 0;

	// initialize
	if(!osp) if(!os) osp = &std::cout;

	// change
	if(os) osp = os;

	return *osp;
}

#define log(m) do{oastats::utils::botlog() << oastats::utils::get_stamp() << ": " << m << std::endl;}while(false)

// Console output
#define con(m) do{std::cout << m << std::endl;}while(false)


}} // oastats::types

#endif // _OASTATS_LOGREP_H_
