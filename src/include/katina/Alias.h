/*
 * Alias.h
 *
 *  Created on: 14 May 2014
 *      Author: oasookee@gmail.com
 */

#ifndef _KATINA_ALIAS_H_
#define _KATINA_ALIAS_H_

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
