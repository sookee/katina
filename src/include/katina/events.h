#ifndef EVENTS_H_
#define EVENTS_H_
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

/*
 *
 *  Created on: 10 Jul 2014
 *      Author: SooKee oasookee@gmail.com
 */

#include "types.h"

namespace katina {

using namespace katina::types;

struct event
{
	siz min;
	siz sec;
};

struct client_event
: event
{
	int num;
};

struct evt_Kill
: client_event
{
	int num2;
	siz weap;
};

struct evt_KatinaFlags
: event
{
	siz state;
};

struct evt_Award
: client_event
{
	siz awd;
};

struct evt_CTF
: client_event
{
	siz col;
	siz act;
};

struct evt_Callvote
: client_event
{
	str type;
	str info;
};

struct evt_ClientBegin
: client_event
{
};

struct evt_ClientConnect
: client_event
{
};

struct evt_ClientDisconnect
: client_event
{
};

struct evt_ClientConnectInfo
: client_event
{
	str guid;
	str ip;
};

struct evt_ClientUserinfoChanged
: client_event
{
	str guid;
	str name;
	siz team;
	siz hc;
};

struct evt_Speed
: client_event
{
	siz dist;
	siz time;
	bool flag;
};

struct evt_ShutdownGame
: event
{
};

} // ::katina

#endif /* EVENTS_H_ */
