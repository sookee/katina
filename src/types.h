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

#include <iostream>
#include <ctime>
#include <cmath>
#include <algorithm>

#include <map>
#include <set>
#include <deque>
#include <mutex>
#include <stack>
#include <vector>
#include <random>
#include <chrono>
#include <istream>

namespace oastats { namespace types {

typedef std::size_t siz;

typedef std::string str;
typedef str::iterator str_iter;
typedef str::const_iterator str_citer;

typedef std::vector<int> int_vec;
typedef std::vector<siz> siz_vec;

typedef std::vector<str> str_vec;
typedef str_vec::iterator str_vec_itr;
typedef str_vec::const_iterator str_vec_citr;

typedef std::set<str> str_set;
typedef str_set::const_iterator str_set_citer;

typedef std::map<str, time_t> str_time_map;
typedef std::pair<const str, time_t> str_time_pair;


typedef std::multiset<str> str_mset;
typedef std::map<str, str> str_map;
typedef std::multimap<str, str> str_mmap;
typedef std::pair<const str, str> str_pair;
typedef std::deque<str> str_deq;

typedef std::map<siz, siz> siz_map;
typedef std::pair<const siz, siz> siz_pair;

typedef std::map<str, siz> str_siz_map;
typedef std::pair<const str, siz> str_siz_pair;

typedef std::map<siz, str> siz_str_map;
typedef std::pair<const siz, str> siz_str_pair;

typedef std::lock_guard<std::mutex> lock_guard;
typedef std::chrono::steady_clock steady_clock;
typedef steady_clock::period period;
typedef steady_clock::time_point time_point;

}} // oastats::types

#endif /* _OASTATS_TYPES_H_ */
