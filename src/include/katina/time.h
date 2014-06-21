//#pragma once
#ifndef _OASTATS_TIME_H_
#define _OASTATS_TIME_H_
/*
 * rcon.h
 *
 *  Created on: 07 Apr 2013
 *      Author: oaskivvy@gmail.com
 */

/*-----------------------------------------------------------------.
| Copyright (C) 2013 SooKee oaskivvy@gmail.com               |
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

#include <ctime>
#include <unistd.h>

namespace katina { namespace time {

using namespace katina::types;

inline
void thread_sleep_millis(siz msecs)
{
	usleep(msecs * 1000);
}

//inline
//hr_time_point get_millitime()
//{
//	timespec ts;
//	clock_gettime(CLOCK_REALTIME, &ts);
//	hr_time_point now;
//	now.
//	return milliseconds((ts.tv_sec * 1000) + (ts.tv_nsec / 1000000));
//}

}} // katina::time

#endif /* _OASTATS_TIME_H_ */
