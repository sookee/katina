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

#include <inttypes.h>
#include <list>

namespace oastats {

using namespace oastats::types;

class GUID
{
	str data;
	bool bot;
	mutable bool connected = true;

public:
	const static siz SIZE = 8;

	GUID(): data(SIZE, '0'), bot(false)
	{
		for(siz i = 0; i < SIZE; ++i)
			this->data[i] = '0';
	}

	GUID(const char data[SIZE]): data(SIZE, '0'), bot(false)
	{
		for(siz i = 0; i < SIZE; ++i)
			this->data[i] = data[i];
	}

	GUID(const str& data): data(SIZE, '0'), bot(false)
	{
		for(siz i = 0; i < SIZE && i < data.size(); ++i)
			this->data[i] = data[i];
	}

	GUID(const GUID& guid): data(SIZE, '0'), bot(guid.bot)
	{
		bot = guid.bot;
		for(siz i = 0; i < SIZE; ++i)
			this->data[i] = guid.data[i];
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
			data = str(GUID::SIZE - data.size(), '0') + data;
	}

	const GUID& operator=(const GUID& guid)
	{
		bot = guid.bot;
		for(siz i = 0; i < SIZE; ++i)
			this->data[i] = guid.data[i];
		return *this;
	}

	bool operator==(const GUID& guid) const
	{
		for(siz i = 0; i < SIZE; ++i)
			if(this->data[i] != guid.data[i])
				return false;
		return true;
	}

	bool operator!=(const GUID& guid) const
	{
		return !(*this == guid);
	}

	bool operator<(const GUID& guid) const
	{
		return data < guid.data;
	}

	char& operator[](siz i) { return data[i]; }
	const char& operator[](siz i) const { return data[i]; }
	siz size() const { return SIZE; }

	operator str() const { return data; }

	operator uint32_t() const
	{
		uint32_t i = 0;
		siss iss(data);
		iss >> std::hex >> i;
		return i;
	}

	void disconnect() const { connected = false; }
	bool is_connected() { return connected; }

	//bool is_bot() const { return data < "00001000"; }
	bool is_bot() const { return bot; }
};

inline
sos& operator<<(sos& os, const GUID& guid)
{
	for(siz i = 0; i < guid.size(); ++i)
		os << guid[i];
	return os;
}

inline
sis& operator>>(sis& is, GUID& guid)
{
	for(siz i = 0; i < guid.size(); ++i)
		is.get(guid[i]);
	return is;
}

typedef std::list<GUID> guid_lst;
typedef guid_lst::iterator guid_lst_iter;
typedef guid_lst::const_iterator guid_lst_citer;

typedef std::map<GUID, str> guid_str_map;
typedef guid_str_map::value_type guid_str_map_pair;
typedef guid_str_map::iterator guid_str_map_iter;
typedef guid_str_map::const_iterator guid_str_map_citer;

typedef std::map<siz, GUID> siz_guid_map;
typedef siz_guid_map::value_type siz_guid_map_pair;
typedef siz_guid_map::iterator siz_guid_map_iter;
typedef siz_guid_map::const_iterator siz_guid_map_citer;

typedef std::map<GUID, siz> guid_siz_map;
typedef guid_siz_map::value_type guid_siz_map_pair;
typedef guid_siz_map::iterator guid_siz_map_iter;
typedef guid_siz_map::const_iterator guid_siz_map_citer;

typedef std::multimap<siz, str> siz_str_mmap;
typedef siz_str_mmap::reverse_iterator siz_str_mmap_ritr;
typedef siz_str_mmap::iterator siz_str_mmap_iter;
typedef siz_str_mmap::const_iterator siz_str_mmap_citer;

typedef std::map<GUID, int> guid_int_map;
typedef std::pair<const GUID, int> guid_int_map_pair;
typedef guid_int_map::iterator guid_int_map_iter;
typedef guid_int_map::const_iterator guid_int_map_citer;

typedef std::set<GUID> guid_set;
typedef guid_set::iterator guid_set_iter;
typedef guid_set::const_iterator guid_set_citer;

extern const GUID null_guid;

/*
 * Create a GUID for bots based on their slot number
 */
//inline GUID bot_guid(siz num)
//{
//	soss oss;
//	oss << num;
//	str id = oss.str();
//	if(id.size() < GUID::SIZE)
//		id = str(GUID::SIZE - id.size(), '0') + id;
//
//	GUID guid(id.c_str());
//	guid.bot = true;
//	return guid;
//}

} // oastats

#endif /* _OASTATS_GUID_H_ */
