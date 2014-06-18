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

using namespace katina;
using namespace katina::log;
using namespace katina::types;

#ifndef REVISION
#define REVISION "not set"
#endif

/**
 * katina /path/to/config [$HOME/.katina]
 */
int main(const int argc, const char* argv[])
{
	srand(std::time(0));
	log("KATINA REVISION: " << REVISION);
	Katina katina;

	// command line override log file
	if(argc > 2)
	{
		katina.props["logfile"].push_back(argv[2]);
		katina.props["run.mode"].push_back("backlog");
	}

	katina.start(str(argc > 1 ? argv[1] : "$HOME/.katina"));
}
