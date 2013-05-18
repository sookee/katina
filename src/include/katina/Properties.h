#pragma once
#ifndef _OASTATS_PROPERTIES_H
#define	_OASTATS_PROPERTIES_H

/*
 * File:   Properties.h
 * Author: SooKee oasookee@gmail.com
 *
 * Created on May 17, 2013
 */

#include "str.h"
#include "types.h"
#include "utils.h"

namespace oastats {

using namespace oastats::types;
using namespace oastats::utils;
using namespace oastats::string;

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
