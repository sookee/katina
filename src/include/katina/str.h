#pragma once
#ifndef _OASTATS_STR_H_
#define _OASTATS_STR_H_
/*
 * str.h
 *
 *  Created on: 28 Jan 2012
 *      Author: oaskivvy@gmail.com
 */

/*-----------------------------------------------------------------.
| Copyright (C) 20312 SooKee oaskivvy@gmail.com               |
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

namespace oastats { namespace string {

using namespace oastats::types;

// -- STRING -------------------------------------------------

static const char* ws = " \t\n\r\f\v";

/**
 * Remove leading characters from a std::string.
 * @param s The std::string to be modified.
 * @param t The set of characters to delete from the beginning
 * of the string.
 * @return The same string passed in as a parameter reference.
 */
inline std::string& ltrim(std::string& s, const char* t = ws)
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
inline std::string& rtrim(std::string& s, const char* t = ws)
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
inline str& trim(str& s, const char* t = ws)
{
	return ltrim(rtrim(s, t), t);
}

inline std::string ltrim_copy(std::string s, const char* t = ws)
{
	return ltrim(s, t);
}

inline std::string rtrim_copy(std::string s, const char* t = ws)
{
	return rtrim(s, t);
}

inline std::string trim_copy(std::string s, const char* t = ws)
{
	return trim(s, t);
}

/**
 * Remove all leading characters of a given value
 * from a std::string.
 * @param s The string to be modified.
 * @param c The character value to delete.
 * @return The same string passed in as a parameter reference.
 */
inline std::string& ltrim(std::string& s, char c)
{
	s.erase(0, s.find_first_not_of(c));
	return s;
}

/**
 * Remove all trailing characters of a given value
 * from a std::string.
 * @param s The string to be modified.
 * @param c The character value to delete.
 * @return The same string passed in as a parameter reference.
 */
inline std::string& rtrim(std::string& s, char c)
{
	s.erase(s.find_last_not_of(c) + 1);
	return s;
}

/**
 * Remove all surrounding characters of a given value
 * from a std::string.
 * @param s The string to be modified.
 * @param c The character value to delete.
 * @return The same string passed in as a parameter reference.
 */
inline std::string& trim(std::string& s, char c)
{
	return ltrim(rtrim(s, c), c);
}

inline std::string rtrim_copy(std::string s, char c)
{
	return rtrim(s, c);
}

inline std::string ltrim_copy(std::string s, char c)
{
	return ltrim(s, c);
}

inline std::string trim_copy(std::string s, char c)
{
	return trim(s, c);
}

inline str& lower(str& s)
{
	std::transform(s.begin(), s.end(), s.begin(), std::ptr_fun<int, int>(std::tolower));
	return s;
}

inline str& upper(str& s)
{
	std::transform(s.begin(), s.end(), s.begin(), std::ptr_fun<int, int>(std::toupper));
	return s;
}

inline
std::string lower_copy(std::string s)
{
	return lower(s);
}

inline
std::string upper_copy(std::string s)
{
	return upper(s);
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

inline
str& replace(str& s, const str& from, const str& to)
{
	if(from.empty())
		return s;
	siz pos = 0;
	if((pos = s.find(from)) != str::npos)
		s.replace(pos, from.size(), to);
	while((pos = s.find(from, pos + to.size())) != str::npos)
		s.replace(pos, from.size(), to);
	return s;
}


}} // oastats::string

#endif /* _OASTATS_STR_H_ */
