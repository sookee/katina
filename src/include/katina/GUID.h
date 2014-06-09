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

#include <cinttypes>
#include <list>
#include <algorithm>

namespace katina {

using namespace katina::log;
using namespace katina::types;

class GUID
{
public:
	const static siz SIZE = 8;

private:
	str data;
	bool bot;
	mutable bool connected = true;

//	explicit GUID(const char data[SIZE]): data(SIZE, '0'), bot(false)
//	{
//		for(siz i = 0; i < SIZE; ++i)
//			this->data[i] = data[i];
//	}

	bool is_bot_data()
	{
		return data.size() == 8 && data[0] == 'B' && data.substr(1) < "0000064";
	}

public:

	GUID(): data(SIZE, '0'), bot(false)
	{
		connected = false;
//		for(siz i = 0; i < SIZE; ++i)
//			this->data[i] = '0';
	}

	GUID(const GUID& guid): data(guid.data), bot(guid.bot)
	{
		connected = guid.connected;
	}

	explicit GUID(const str& data): data(SIZE, '0'), bot(false)
	{
		for(siz i = 0; i < SIZE && i < data.size(); ++i)
			this->data[i] = data[i];
		bot = is_bot_data();
	}

	/**
	 * bot constructor
	 */
	explicit GUID(slot num): data(SIZE, '0'), bot(true)
	{
		soss oss;
		oss << num;
		data = oss.str();
		if(data.size() < GUID::SIZE)
			data = "B" + str(GUID::SIZE - data.size() - 1, '0') + data;
		//bug_var(data);
	}

	const GUID& operator=(const GUID& guid)
	{
		bot = guid.bot;
		connected = guid.connected;
		data = guid.data;
		return *this;
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

//	char& operator[](siz i) { return data[i]; }
//	const char& operator[](siz i) const { return data[i]; }
	siz size() const { return SIZE; }

	operator str() const { return data; }

	explicit operator uint32_t() const
	{
		uint32_t i = 0;
		siss iss(data);
		iss >> std::hex >> i;
		return i;
	}

	operator bool() const
	{
		if(data.size() != SIZE)
			return false;
		return std::count_if(data.begin(), data.end(), std::ptr_fun<int,int>(isxdigit)) == SIZE;
	}

	void disconnect() const { connected = false; }
	bool is_connected() const { return connected; }

	//bool is_bot() const { return data < "00001000"; }
	bool is_bot() const { return bot; }

	friend sos& operator<<(sos& os, const GUID& guid)
	{
		return os << guid.data;
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
			is.setstate(std::ios::failbit);
//		if(guid.data.size() == 8 && guid.data[0] == 'B' && guid.data.substr(1) < "0000064")
//			guid.bot = true;
		guid.bot = guid.is_bot_data();
		if(guid.data == "00000000")
			guid.connected = false;
		return is;
	}
};

TYPEDEF_CONTAINER_1(std::set, GUID, guid_set);
TYPEDEF_CONTAINER_1(std::list, GUID, guid_lst);
TYPEDEF_CONTAINER_2(std::map, GUID, str, guid_str_map);
TYPEDEF_CONTAINER_2(std::map, GUID, siz, guid_siz_map);
TYPEDEF_CONTAINER_2(std::map, GUID, int, guid_int_map);
TYPEDEF_CONTAINER_2(std::map, slot, GUID, slot_guid_map);

extern const GUID null_guid;
} // oastats

#endif /* _OASTATS_GUID_H_ */
