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

namespace katina { namespace types {

#define TYPEDEF_CONTAINER_T(def) \
typedef def::value_type def##_vt; \
typedef def::iterator def##_iter; \
typedef def::const_iterator def##_citer; \
typedef def::reverse_iterator def##_riter; \
typedef def::const_reverse_iterator def##_criter

/**
 * Single parameter containers
 */
#define TYPEDEF_CONTAINER_1(type, p, def) \
typedef type<p> def; \
TYPEDEF_CONTAINER_T(def)

/**
 * Two parameter containers
 */
#define TYPEDEF_CONTAINER_2(type, p1, p2, def) \
typedef type<p1,p2> def; \
TYPEDEF_CONTAINER_T(def)

//-- TYPES ---------------------------------------------

typedef unsigned char byte;

typedef std::size_t siz;

typedef std::string str;
typedef str::iterator str_iter;
typedef str::const_iterator str_citer;

TYPEDEF_CONTAINER_1(std::vector, int, int_vec);
TYPEDEF_CONTAINER_1(std::vector, siz, siz_vec);
TYPEDEF_CONTAINER_1(std::vector, str, str_vec);

TYPEDEF_CONTAINER_1(std::set, int, int_set);
TYPEDEF_CONTAINER_1(std::set, siz, siz_set);
TYPEDEF_CONTAINER_1(std::set, str, str_set);

TYPEDEF_CONTAINER_1(std::multiset, int, int_mset);
TYPEDEF_CONTAINER_1(std::multiset, siz, siz_mset);
TYPEDEF_CONTAINER_1(std::multiset, str, str_mset);

TYPEDEF_CONTAINER_2(std::map, int, int, int_map);
TYPEDEF_CONTAINER_2(std::map, siz, siz, siz_map);
TYPEDEF_CONTAINER_2(std::map, str, str, str_map);

TYPEDEF_CONTAINER_2(std::multimap, int, int, int_mmap);
TYPEDEF_CONTAINER_2(std::multimap, siz, siz, siz_mmap);
TYPEDEF_CONTAINER_2(std::multimap, str, str, str_mmap);

TYPEDEF_CONTAINER_2(std::map, str, int, str_int_map);
TYPEDEF_CONTAINER_2(std::map, str, siz, str_siz_map);
TYPEDEF_CONTAINER_2(std::map, int, str, int_str_map);
TYPEDEF_CONTAINER_2(std::map, siz, str, siz_str_map);
TYPEDEF_CONTAINER_2(std::multimap, int, str, int_str_mmap);
TYPEDEF_CONTAINER_2(std::multimap, siz, str, siz_str_mmap);
TYPEDEF_CONTAINER_2(std::map, str, std::time_t, siz_time_map);
TYPEDEF_CONTAINER_2(std::map, str, str_set, str_set_map);
TYPEDEF_CONTAINER_2(std::map, str, str_vec, str_vec_map);

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
};

TYPEDEF_CONTAINER_2(std::map, slot, siz, slot_siz_map);

}} // katina::types

#endif /* _OASTATS_TYPES_H_ */
