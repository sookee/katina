#pragma once
#ifndef _OASTATS_LOG_H_
#define _OASTATS_LOG_H_
/*
 * log.h
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

namespace oastats { namespace log {

using namespace oastats::types;

// -- LOGGING ------------------------------------------------

inline
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
#define bug_var(v)
#define bug_func()
#define con(m) do{std::cout << m << std::endl;}while(false)
#define log(m) do{std::cout << oastats::log::get_stamp() << ": " << m << std::endl;}while(false)
#else
#define bug(m) do{std::cout << m << std::endl;}while(false)
#define QUOTE(s) #s
#define bug_var(v) bug(QUOTE(v:) << std::boolalpha << " " << v)
struct _
{
	const char* n;
	_(const char* n): n(n) { bug("---> " << n); }
	~_() { bug("<--- " << n); }
};
#define bug_func() oastats::log::_ __(__PRETTY_FUNCTION__)
#define con(m) do{std::cout << m << " [" << __FILE__ << "]" << " (" << __LINE__ << ")" << std::endl;}while(false)
#define log(m) do{std::cout << oastats::log::get_stamp() << ": " << m << " [" << __FILE__ << "]" << " (" << __LINE__ << ")" << std::endl;}while(false)
#endif


//#define trace(m)
#define trace(m) bug("TRACE: " << m << ": " << __LINE__)

inline
bool log_error(const str& m, bool status = false) { log(m); return status; }

}} // oastats::log

#endif /* _OASTATS_LOG_H_ */
