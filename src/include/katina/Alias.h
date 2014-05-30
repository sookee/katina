/*
 * Alias.h
 *
 *  Created on: 14 May 2014
 *      Author: oasookee@gmail.com
 */

#ifndef _KATINA_ALIAS_H_
#define _KATINA_ALIAS_H_
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


#include "types.h"

namespace oastats { namespace utils {

using namespace oastats::types;
using namespace oastats::string;

class Alias
{
private:
	str_map aliases;

public:

	void load(const str& file_name)
	{
		aliases.clear();
		sifs ifs(file_name.c_str());

		if(!ifs)
			return;

		siz num = 0;
		siz pos;
		str line, key, val;
		while(sgl(ifs, line))
		{
			++num;
			if((pos = line.find("//")) != str::npos)
				line.erase(pos);
			if(trim(line).empty() || line[0] == '#')
				continue;

			siss iss(line);
			if(sgl(sgl(iss, key, ':') >> std::ws, val))
				if(!aliases.insert(str_map_pair(key, val).second))
					log("WARN: Duplicate alias in: " << file_name << " at line: " << num);
		}
	}

	/**
	 * Translate an alias to its value if present.
	 * @return true means the alias is set and the value returned in val.
	 */
	bool trans(const str& key, str& val)
	{
		str_map_citer f = aliases.find(key);
		if(f == aliases.end())
			return false;
		val = f->second;
		return true;
	}

	/**
	 * Translate an alias into its value.
	 * If not present the key is returned untranslated.
	 */
	str trans(const str& key)
	{
		str_map_citer f = aliases.find(key);
		return f == aliases.end() ? key : f->second;
	}
};

}} // oastats::utils

#endif // _KATINA_ALIAS_H_
