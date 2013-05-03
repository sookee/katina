/*
 * irc.cpp
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

#include <katina/irc.h>
#include <katina/types.h>

namespace oastats { namespace irc {

using namespace oastats::types;

// -- IRC --------------------------------------------------------------

#define ColorIndex(c)	( ( (c) - '0' ) & 7 )
#define Q_COLOR_ESCAPE	'^'
#define Q_IsColorString(p)	( p && *(p) == Q_COLOR_ESCAPE && *((p)+1) && isalnum(*((p)+1)) ) // ^[0-9a-zA-Z]
#define Q_IsSpecialChar(c) ((c) && ((c) < 32))

const int oatoirctab[8] =
{
	1 // "black"
	, 4 // "red"
	, 3 // "lime"
	, 8 // "yellow"
	, 2 // "blue"
	, 12 // "cyan"
	, 6 // "magenta"
	, 0 // "white"
};

const str IRC_BOLD = "";
const str IRC_NORMAL = "";
const str IRC_COLOR = "";

str oa_to_IRC(const char* msg)
{
	std::ostringstream oss;

	oss << IRC_BOLD;

	while(*msg)
	{
		if(Q_IsColorString(msg))
		{
			oss << IRC_NORMAL;
			siz code = (*(msg + 1)) % 8;
			oss << IRC_COLOR << (oatoirctab[code] < 10 ? "0" : "") << oatoirctab[code];
			msg += 2;
		}
		else if(Q_IsSpecialChar(*msg))
		{
			oss << "#";
			msg++;
		}
		else
		{

			oss << *msg;
			msg++;
		}
	}

	oss << IRC_NORMAL;

	return oss.str();
}

}} // oastats::irc
