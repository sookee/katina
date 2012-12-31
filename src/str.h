#pragma once
#ifndef _SKIVVY_STR_H_
#define _SKIVVY_STR_H_
/*
 * str.h
 *
 *  Created on: 28 Jan 2012
 *      Author: oaskivvy@gmail.com
 */

/*-----------------------------------------------------------------.
| Copyright (C) 2012 SooKee oaskivvy@gmail.com               |
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

#include <skivvy/types.h>

namespace skivvy { namespace string {

using namespace skivvy::types;

inline str& ltrim(str& s, const char* t = " \t\n\r\f\v")
{
	s.erase(0, s.find_first_not_of(t));
	return s;
}

inline str& rtrim(str& s, const char* t = " \t\n\r\f\v")
{
	s.erase(s.find_last_not_of(t) + 1);
	return s;
}

/**
 * Remove surrounding whitespace from a std::string.
 * @param s The string to be modified.
 * @param t The set of characters to delete from each end
 * of the string.
 * @return The same string passed in as a parameter reference.
 */
inline str& trim(str& s, const char* t = " \t\n\r\f\v")
{
//	s.erase(0, s.find_first_not_of(t));
//	s.erase(s.find_last_not_of(t) + 1);
	return ltrim(rtrim(s, t), t);
}

inline str& trim(str& s, char c)
{
	s.erase(0, s.find_first_not_of(c));
	s.erase(s.find_last_not_of(c) + 1);
	return s;
}

str& replace(str& s, const str& from, const str& to);

str lowercase(str s);
str uppercase(str s);

siz extract_delimited_text(const str& in, const str& d1, const str& d2, str& out, siz pos = 0);

str_vec split(const str& s, char d = ' ');
str_vec split_params(const str& s, char d);

}} // skivvy::string

#endif /* _SKIVVY_STR_H_ */
