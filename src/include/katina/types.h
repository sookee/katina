#pragma once
#ifndef _OASTATS_TYPES_H_
#define _OASTATS_TYPES_H_
/*
 * tyoes.h
 *
 *  Created on: 9 Jan 2012
 *      Author: oasookee@gmail.com
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
//#include <random>
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

typedef std::size_t siz;

typedef std::string str;
typedef str::iterator str_iter;
typedef str::const_iterator str_citer;

TYPEDEF_VEC(int, int_vec);
TYPEDEF_VEC(siz, siz_vec);
TYPEDEF_VEC(str, str_vec);

TYPEDEF_SET(int, int_set);
TYPEDEF_SET(siz, siz_set);
TYPEDEF_SET(str, str_set);

TYPEDEF_MSET(int, int_mset);
TYPEDEF_MSET(siz, siz_mset);
TYPEDEF_MSET(str, str_mset);

TYPEDEF_MAP(int, int, int_map);
TYPEDEF_MAP(siz, siz, siz_map);
TYPEDEF_MAP(str, str, str_map);

TYPEDEF_MMAP(int, int, int_mmap);
TYPEDEF_MMAP(siz, siz, siz_mmap);
TYPEDEF_MMAP(str, str, str_mmap);

TYPEDEF_MAP(str, int, str_int_map);
TYPEDEF_MAP(str, siz, str_siz_map);
TYPEDEF_MAP(int, str, int_str_map);
TYPEDEF_MAP(siz, str, siz_str_map);
TYPEDEF_MAP(str, std::time_t, siz_time_map);
TYPEDEF_MAP(str, str_set, str_set_map);
TYPEDEF_MAP(str, str_vec, str_vec_map);

TYPEDEF_MMAP(int, str, int_str_mmap);
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

typedef long milliseconds;

class slot
{
	siz num;
public:
	slot(): num(0) {}
	explicit slot(siz num): num(num) {}

	bool operator<(const slot& s) const { return num < s.num; }
	bool operator>(const slot& s) const { return num > s.num; }
	bool operator==(const slot& s) const { return num == s.num; }
	bool operator!=(const slot& s) const { return num != s.num; }

	friend sos& operator<<(sos& o, const slot& s) { return o << s.num; }
	friend sis& operator>>(sis& i, slot& s) { return i >> s.num; }

	explicit operator str() const { return std::to_string(num); }
	explicit operator siz() const { return num; }
};

TYPEDEF_MAP(slot, siz, slot_siz_map);

typedef std::lock_guard<std::mutex> lock_guard;
typedef std::unique_lock<std::mutex> unique_lock;

}} // katina::types

#endif /* _OASTATS_TYPES_H_ */
