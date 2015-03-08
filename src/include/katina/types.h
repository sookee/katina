#ifndef _KATINA_TYPES_H_
#define _KATINA_TYPES_H_
/*
 *  Created on: 9 Jan 2012
 *      Author: SooKee oasookee@gmail.com
 */

/*-----------------------------------------------------------------.
| Copyright (C) 2011 SooKee oasookee@gmail.com                     |
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

#include <ctime>
#include <cmath>
#include <algorithm>

#include <map>
#include <set>
#include <deque>
#include <stack>
#include <vector>
#include <chrono>
#include <istream>
#include <ostream>
#include <sstream>
#include <fstream>
#include <iostream>

#include <mutex>
#include <thread>


// TODO: mode to "defs.h"
#define MAX_CLIENTS 64

#define TYPEDEF_CONTAINER_T(name) \
	typedef name::value_type name##_vt; \
	typedef name::iterator name##_iter; \
	typedef name::const_iterator name##_citer; \
	typedef name::reverse_iterator name##_riter; \
	typedef name::const_reverse_iterator name##_criter

/**
 * Single parameter containers
 */
#define TYPEDEF_CONTAINER_1(type, t, name) \
	typedef type<t> name; \
	TYPEDEF_CONTAINER_T(name)

/**
 * Two parameter containers
 */
#define TYPEDEF_CONTAINER_2(type, t1, t2, name) \
	typedef type<t1,t2> name; \
	TYPEDEF_CONTAINER_T(name)

#define TYPEDEF_DEQ(type, name) \
	TYPEDEF_CONTAINER_1(std::deque, type, name)

#define TYPEDEF_QUE(type, name) \
	TYPEDEF_CONTAINER_1(std::queue, type, name)

#define TYPEDEF_PQUE(type, name) \
	TYPEDEF_CONTAINER_1(std::priority_queue, type, name)

#define TYPEDEF_SET(type, name) \
	TYPEDEF_CONTAINER_1(std::set, type, name)

#define TYPEDEF_MSET(type, name) \
	TYPEDEF_CONTAINER_1(std::multiset, type, name)

#define TYPEDEF_USET(type, name) \
	TYPEDEF_CONTAINER_1(std::unordered_set, type, name)

#define TYPEDEF_LST(type, name) \
	TYPEDEF_CONTAINER_1(std::list, type, name)

#define TYPEDEF_FLST(type, name) \
	TYPEDEF_CONTAINER_1(std::foreward_list, type, name)

#define TYPEDEF_VEC(type, name) \
	TYPEDEF_CONTAINER_1(std::vector, type, name)

#define TYPEDEF_PAIR(type1, type2, name) \
	typedef std::pair<type1,type2> name

#define TYPEDEF_MAP(type1, type2, name) \
	TYPEDEF_CONTAINER_2(std::map, type1, type2, name); \
	TYPEDEF_PAIR(name##_iter, name##_iter, name##_range)

#define TYPEDEF_MMAP(type1, type2, name) \
	TYPEDEF_CONTAINER_2(std::multimap, type1, type2, name); \
	TYPEDEF_PAIR(name##_iter, name##_iter, name##_range)

namespace katina { namespace types {

//-- TYPES ---------------------------------------------

typedef unsigned char byte;

typedef unsigned uns;
typedef std::size_t siz;
typedef std::string str;

//class str
//{
//	std::string rep;
//public:
//	const static siz npos;
//
//	typedef str* pointer;
//	typedef const str* const_pointer;
//    typedef __gnu_cxx::__normal_iterator<pointer, str>  iterator;
//    typedef __gnu_cxx::__normal_iterator<const_pointer, str> const_iterator;
//public:
//	str() {}
//	str(const std::string& s): rep(s) {}
//	str(const str& s):rep(s.rep) {}
//	str(const str& s, size_t pos, size_t len = str::npos): rep(s.rep, pos, len) {}
//	str(const char* s): rep(s?s:"") {}
//	str(const char* s, siz n): rep(s?s:"", s?n:0) {}
//	str(siz n, char c): rep(n, c) {}
//
//	template <class InputIterator>
//	str(InputIterator first, InputIterator last): rep(first, last) {}
//	str(std::initializer_list<char> il): rep(il) {}
//	str(str&& s) rep(std::move(s.rep)) {} noexcept;
//
//	bool operator<(const str& s) const { return rep < s.rep; }
//	bool operator==(const str& s) const { return rep == s.rep; }
//	siz size() const { return rep.size(); }
//
//	friend std::istream& operator>>(std::istream& i, class str& s) { return i >> s.rep; }
//	friend std::ostream& operator<<(std::ostream& o, const class str& s) { return o << s.rep; }
//};
//
//const siz str::npos = std::string::npos;

typedef str::iterator str_iter;
typedef str::const_iterator str_citer;

TYPEDEF_VEC(int, int_vec);
TYPEDEF_VEC(uns, uns_vec);
TYPEDEF_VEC(siz, siz_vec);
TYPEDEF_VEC(str, str_vec);

TYPEDEF_SET(int, int_set);
TYPEDEF_SET(uns, uns_set);
TYPEDEF_SET(siz, siz_set);
TYPEDEF_SET(str, str_set);

TYPEDEF_MSET(int, int_mset);
TYPEDEF_MSET(uns, uns_mset);
TYPEDEF_MSET(siz, siz_mset);
TYPEDEF_MSET(str, str_mset);

TYPEDEF_MAP(int, int, int_map);
TYPEDEF_MAP(uns, uns, uns_map);
TYPEDEF_MAP(siz, siz, siz_map);
TYPEDEF_MAP(str, str, str_map);

TYPEDEF_MMAP(int, int, int_mmap);
TYPEDEF_MMAP(siz, siz, siz_mmap);
TYPEDEF_MMAP(str, str, str_mmap);

TYPEDEF_MAP(str, int, str_int_map);
TYPEDEF_MAP(str, uns, str_uns_map);
TYPEDEF_MAP(str, siz, str_siz_map);
TYPEDEF_MAP(int, str, int_str_map);
TYPEDEF_MAP(uns, str, uns_str_map);
TYPEDEF_MAP(siz, str, siz_str_map);
TYPEDEF_MAP(str, std::time_t, siz_time_map);
TYPEDEF_MAP(str, str_set, str_set_map);
TYPEDEF_MAP(str, str_vec, str_vec_map);

TYPEDEF_MMAP(int, str, int_str_mmap);
TYPEDEF_MMAP(uns, str, uns_str_mmap);
TYPEDEF_MMAP(siz, str, siz_str_mmap);

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

//typedef long milliseconds;

class slot
{
	int num;
public:
	static const slot bad;
	static const slot all;
	static const slot world;
	static const slot max;

	slot(): num(-1) {}
	explicit slot(int num): num(num) {}
	slot(const slot& num): num(num.num) {}

	slot& operator++() { ++num; return *this; }
	slot operator++(int) { slot s(num); ++(*this); return s; }

	bool operator<(const slot& s) const { return num < s.num; }
	bool operator>(const slot& s) const { return num > s.num; }
	bool operator==(const slot& s) const { return num == s.num; }
	bool operator!=(const slot& s) const { return num != s.num; }
	bool operator<=(const slot& s) const { return num <= s.num; }
	bool operator>=(const slot& s) const { return num >= s.num; }

	friend sos& operator<<(sos& o, const slot& s);
	friend sis& operator>>(sis& i, slot& s);

	explicit operator str() const { return std::to_string(num); }
	operator siz() const { return num; }
	explicit operator int() const { return num; }
};

inline sos& operator<<(sos& o, const slot& s) { return o << s.num; }
inline sis& operator>>(sis& i, slot& s)
{
	if(!(i >> s.num))
		return i;

	if(s.num == 1022)
		s = slot::world;
	else if(s.num >= MAX_CLIENTS)
	{
		s = slot::bad;
		i.setstate(std::ios::failbit);
	}

	return i;
}

TYPEDEF_MAP(slot, siz, slot_siz_map);

typedef std::lock_guard<std::mutex> lock_guard;
typedef std::unique_lock<std::mutex> unique_lock;

typedef std::lock_guard<std::recursive_mutex> r_lock_guard;
typedef std::unique_lock<std::recursive_mutex> r_unique_lock;

typedef std::chrono::system_clock sys_clk;
typedef sys_clk::period sys_period;
typedef sys_clk::time_point sys_time_point;

typedef std::chrono::high_resolution_clock hr_clk;
typedef hr_clk::period hr_period;
typedef hr_clk::time_point hr_time_point;

using std::chrono::milliseconds;
using std::chrono::seconds;
using std::chrono::minutes;
using std::chrono::hours;

// Precedence for two pre-release versions with the
// same major, minor, and patch version MUST be
// determined by comparing each dot separated
// identifier from left to right until a difference
// is found as follows: identifiers consisting of
// only digits are compared numerically and
// identifiers with letters or hyphens are compared
// lexically in ASCII sort order. Numeric identifiers
// always have lower precedence than non-numeric
// identifiers. A larger set of pre-release fields
// has a higher precedence than a smaller set, if all
// of the preceding identifiers are equal.
//
// Example: 1.0.0-alpha < 1.0.0-alpha.1
// < 1.0.0-alpha.beta < 1.0.0-beta < 1.0.0-beta.2
// < 1.0.0-beta.11 < 1.0.0-rc.1 < 1.0.0.

struct version_t
{
	siz maj = 0;
	siz min = 0;
	siz fix = 0;
	str_vec pre;

	bool operator<(const version_t& v) const
	{
		if(maj < v.maj)
			return true;
		if(maj > v.maj)
			return false;
		if(min < v.min)
			return true;
		if(min > v.min)
			return false;
		if(fix < v.fix)
			return true;
		if(fix > v.fix)
			return false;

		siz p = 0;
		for(; p < pre.size(); ++p)
		{
			if(p == v.pre.size())
				return false;

			if(pre[p].find_first_not_of("0123456789") == str::npos)
			{
				if(v.pre[p].find_first_not_of("0123456789") == str::npos)
				{
					if(std::stoi(pre[p]) < std::stoi(v.pre[p]))
						return true;
					if(std::stoi(pre[p]) > std::stoi(v.pre[p]))
						return false;
				}
				else
				{
					return true;
				}
			}
			else
			{
				if(v.pre[p].find_first_not_of("0123456789") == str::npos)
				{
					return false;
				}
				else
				{
					if(pre[p] < v.pre[p])
						return true;
					if(pre[p] > v.pre[p])
						return false;
				}
			}
		}

		if(p == v.pre.size()) // equal
			return false;

		return true; // less
	}
};

// mod_katina
enum
{
	// ONLY output machine readable portion of log messages
	// (not fully implemented yet)
	KATINA_MACHINE_ONLY = (0 << 1)


	// log messages contain Push: messages
	, KATINA_PUSH = (1 << 1)
	// log "say:" messages contain client number, name length
	// 00:00 say: 5 11 Player Name: message
	// unless KATINA_MACHINE_ONLY is set giving only client number
	// 00:00 say: 5: message
	, KATINA_SAY = (2 << 1)
};

}} // katina::types

#endif /* _KATINA_TYPES_H_ */
