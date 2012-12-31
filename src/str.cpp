/*
 * str.cpp
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

#include <skivvy/str.h>

#include <cctype>
#include <sstream>
#include <algorithm>
#include <thread>

namespace skivvy { namespace string {

using namespace skivvy::types;

/**
 * Remove surrounding whitespace from a std::string.
 * @param s The string to be modified.
 * @param t The set of characters to delete from each end
 * of the string.
 * @return The same string passed in as a parameter reference.
 */
//str& trim(str& s, const char* t)
//{
//	s.erase(0, s.find_first_not_of(t));
//	s.erase(s.find_last_not_of(t) + 1);
//	return s;
//}

str& replace(str& s, const str& from, const str& to)
{
	size_t pos;
	while((pos = s.find(from)) != str::npos)
		s.replace(pos, from.size(), to);
	return s;
}


str lowercase(str s)
{
	std::transform(s.begin(), s.end(), s.begin(), std::ptr_fun<int, int>(tolower));
	return s;
}

str uppercase(str s)
{
	std::transform(s.begin(), s.end(), s.begin(), std::ptr_fun<int, int>(toupper));
	return s;
}

size_t extract_delimited_text(const str& in, const str& d1, const str& d2, str& out, size_t pos)
{
	if(pos == str::npos)
		return pos;

	size_t end = pos;

	if((pos = in.find(d1, pos)) != str::npos)
		if((end = in.find(d2, (pos = pos + d1.size()))) != str::npos)
		{
			out = in.substr(pos, end - pos);
			return end + d2.size();
		}
	return str::npos;
}

str fit_to_width(const str& s, size_t w, char align = 'L', char fill = ' ')
{
	str fmt;

	if(s.size() > w)
		if(align == 'L') fmt = s.substr(w);
		else if(align == 'R') fmt = s.substr(0, w);
		else fmt = s.substr(w >> 1, w >> 1);
	else
		if(align == 'L') fmt = s + str(w - s.size(), fill);
		else if(align == 'R') fmt = str(w - s.size(), fill) + s;
		else fmt = str((w >> 1) - s.size(), fill) + s + str((w >> 1) - s.size(), fill);

	return fmt;
}

str_vec split(const str& s, char d)
{
	str_vec v;
	std::istringstream iss(s);
	str p;
	while(std::getline(iss, p, d))
		v.push_back(p);
	return v;
}

// TODO: Move this to <skivvy/stl.h>
template<typename Container, typename Pred>
typename Container::iterator remove_if(Container& c, Pred pred)
{
	return std::remove_if(c.begin(), c.end(), pred);
}

// TODO: Move this to <skivvy/stl.h>
template<typename Container, typename Pred>
typename Container::iterator erase_if(Container& c, Pred pred)
{
	return c.erase(std::remove_if(c.begin(), c.end(), pred), c.end());
}

// TODO: Make this take note of quoted params
str_vec split_params(const str& s, char d)
{
	str_vec v = split(s, d);
//	v.erase(remove_if(v, [](const str& s) { return s.empty(); }), v.end());
	erase_if(v, [](const str& s) { return s.empty(); });
	return v;
}

}} // sookee::string
