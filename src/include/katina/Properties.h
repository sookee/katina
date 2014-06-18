#pragma once
#ifndef _OASTATS_PROPERTIES_H
#define	_OASTATS_PROPERTIES_H

/*
 * File:   Properties.h
 * Author: SooKee oasookee@gmail.com
 *
 * Created on May 17, 2013
 */

/*-----------------------------------------------------------------.
| Copyright (C) 2013 SooKee oasookee@gmail.com               |
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

#include "str.h"
#include "types.h"
#include "utils.h"

namespace katina {

using namespace katina::types;
using namespace katina::utils;
using namespace katina::string;

class Properties
{
private:
	typedef std::map<str, str_vec> property_map;
	typedef std::pair<const str, str_vec> property_map_pair;
	typedef property_map::iterator property_map_iter;
	typedef property_map::const_iterator property_map_citer;

	typedef std::pair<property_map_iter, property_map_iter> property_map_iter_pair;
	typedef std::pair<property_map_iter, property_map_iter> property_map_range;

	property_map props;

public:
	
	template<typename T>
	T get(const str& s, const T& dflt = T())
	{
		if(props[s].empty())
			return dflt;
		T t;
		std::istringstream(props[s][0]) >> std::boolalpha >> t;
		return t;
	}

	str get(const str& s, const str& dflt = "")
	{
		return props[s].empty() ? dflt : props[s][0];
	}

	str_vec get_vec(const str& s)
	{
		return props[s];
	}

	bool has(const str& s)
	{
		property_map_range i = props.equal_range(s);
		return i.first != i.second;
	}

	bool have(const str& s) { return has(s); }
	
	bool load(std::istream& is)
	{
		siz pos;
		str line, key, val;
		
		siz no = 0;
		while(sgl(is, line))
		{
			++no;
			//bug(no << ": line: " << line);
			
			if((pos = line.find("//")) != str::npos)
				line.erase(pos);
			
			//bug(no << ": line: " << line);
		
			trim(line);
			
			//bug(no << ": line: " << line);
			//bug("");
			
			if(line.empty() || line[0] == '#')
				continue;
			
			// remote.irc.client: file data/irc-output.txt data/irc-input.txt #test-channel(*)
			siss iss(line);
			if(sgl(sgl(iss, key, ':') >> std::ws, val))
			{
				//bug("expand_env(val): " << expand_env(val, WRDE_SHOWERR|WRDE_UNDEF));
				props[key].push_back(expand_env(val, WRDE_SHOWERR|WRDE_UNDEF));
			}
		}
		return true;
	}
};
 
} // ::oastats

#endif // _OASTATS_PROPERTIES_H
