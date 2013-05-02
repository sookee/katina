#pragma once
#ifndef _OASTATS_UTILS_H_
#define _OASTATS_UTILS_H_
/*
 * utils.h
 *
 *  Created on: 01 May 2013
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

#include <string>
#include <wordexp.h>
#include <dirent.h>

// STACK TRACE
#include <cstdio>
#include <cstdlib>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <execinfo.h>
#include <cxxabi.h>

namespace oastats { namespace utils {

using namespace oastats::types;

inline
sis& sgl(sis& is, str& line, char delim = '\n')
{
	return std::getline(is, line, delim);
}

inline
str expand_env(const str& var)
{
	str exp;
	wordexp_t p;
	wordexp(var.c_str(), &p, 0);
	if(p.we_wordc)
		exp = p.we_wordv[0];
	wordfree(&p);
	return exp;
}

inline
void stack_handler(int sig)
{
	void *array[2048];
	size_t size;

	// get void*'s for all entries on the stack
	size = backtrace(array, 2048);

	// print out all the frames to stderr
	char** trace = backtrace_symbols(array, size);

	int status;
	str obj, func;
	for(siz i = 0; i < size; ++i)
	{
		siss iss(trace[i]);
		std::getline(std::getline(iss, obj, '('), func, '+');

		char* func_name = abi::__cxa_demangle(func.c_str(), 0, 0, &status);
		std::cerr << "function: " << func_name << '\n';
		free(func_name);
	}
	free(trace);
	exit(1);
}

/**
 * Get directory listing.
 * @return errno
 */
inline
bool ls(const str& folder, str_vec &files)
{
    DIR* dir;
	dirent* dirp;

	if(!(dir = opendir(folder.c_str())))
		return false;

	files.clear();
	while((dirp = readdir(dir)))
		files.push_back(dirp->d_name);

	if(closedir(dir))
		return false;

	return true;
}

}} // oastats::utils

#endif // _OASTATS_UTILS_H_
