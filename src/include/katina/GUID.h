//#pragma once
#ifndef _OASTATS_GUID_H_
#define _OASTATS_GUID_H_

/*
 * GUID.h
 *
 *  Created on: Apr 7, 2013
 *      Author: oasookee@gmail.com
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

#include "types.h"
#include "log.h"

//#include <cinttypes>
#include <cstdint>
#include <list>
#include <algorithm>
#include <iomanip>

namespace katina {

using namespace katina::log;
using namespace katina::types;

class GUID
{
private:
	uint32_t data = 0;

	bool bot = false;
	bool bad = false; // created with bad string data?

	mutable str src; // input data (if relevant) for debugging
	mutable bool connected = true;

	bool is_bot_data() const
	{
		return ((data >> 28) & 0xF) == 0x0B && (data & 0x0FFFFFFF) < 64;
	}

public:

//	GUID() = delete;
	GUID(): connected(false)
	{
//		bug("CTOR NULL GUID: " << std::uppercase << (void*)this);
	}

	~GUID()
	{
//		if(!data)
//			bug("DTOR NULL GUID: " << std::uppercase << (void*)this);
	}

	explicit GUID(const str& s): src(s)
	{
		bad = s.size() != 8 || !(siss(s) >> std::hex >> data);
		bot = is_bot_data();
	}

	/**
	 * bot constructor
	 */
	explicit GUID(slot num): data(int(num)|0xB0000000), bot(true)
	{
	}

	bool operator==(const GUID& guid) const
	{
		return data == guid.data;
	}

	bool operator!=(const GUID& guid) const
	{
		return !(*this == guid);
	}

	bool operator<(const GUID& guid) const
	{
		return data < guid.data;
	}

	explicit operator str() const
	{
		using namespace std;
		if(!src.empty())
			return src;
		return src = ((sss&)(sss() << setw(8) << setfill('0') << hex << uppercase << data)).str();
	}

	explicit operator uint32_t() const
	{
		return data;
	}

	operator bool() const
	{
		return !bad;
	}

	void disconnect() const { connected = false; }
	bool is_connected() const { return connected; }

	bool is_bot() const { return bot; }

	friend sos& operator<<(sos& os, const GUID& guid)
	{
		return os << str(guid);
	}

	friend sis& operator>>(sis& is, GUID& guid)
	{
		str s;
		is >> s;

		if(s.size() == 8)
			guid = GUID(s);
		else if(s.size() == 32)
			guid = GUID(s.substr(24));
		else
		{
			is.setstate(std::ios::failbit);
			guid.bad = true;
		}
		guid.bot = guid.is_bot_data();
		if(!guid.data)
			guid.connected = false;
		return is;
	}
};

TYPEDEF_CONTAINER_1(std::set, GUID, guid_set);
TYPEDEF_CONTAINER_1(std::list, GUID, guid_lst);
TYPEDEF_CONTAINER_2(std::map, GUID, str, guid_str_map);
TYPEDEF_CONTAINER_2(std::map, GUID, siz, guid_siz_map);
TYPEDEF_CONTAINER_2(std::map, GUID, int, guid_int_map);
//TYPEDEF_CONTAINER_2(std::map, slot, GUID, slot_guid_map);

extern const GUID null_guid;
} // katina

#endif /* _OASTATS_GUID_H_ */
