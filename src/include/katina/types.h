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

namespace oastats { namespace types {

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

typedef std::set<siz> siz_set;
typedef siz_set::iterator siz_set_iter;
typedef siz_set::const_iterator siz_set_citer;

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

}} // oastats::types

#endif /* _OASTATS_TYPES_H_ */
