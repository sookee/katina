/*
 * katina.cpp
 *
 *  Created on: 18 Jun 2012
 *      Author: oasookee@googlemail.com
 */


/*-----------------------------------------------------------------.
| Copyright (C) 2012 SooKee oasookee@googlemail.com               |
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

// STACK TRACE
#include <cstdio>
#include <cstdlib>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <execinfo.h>
#include <cxxabi.h>

#include <katina/Katina.h>
#include <katina/log.h>
#include <katina/types.h>

using namespace oastats;
using namespace oastats::log;
using namespace oastats::types;

/**
 * katina-1 /path/to/config [$HOME/.katina]
 */
int main(const int argc, const char* argv[])
{
	bug("argc: " << argc);
	Katina katina;
	katina.start(str(argc == 2 ? argv[1] : "$HOME/.katina"));
}