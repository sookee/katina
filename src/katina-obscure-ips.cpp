/*
 *  Created on: 3 Jul 2014
 *      Author: SooKee oasookee@googlemail.com
 */


/*-----------------------------------------------------------------.
| Copyright (C) 2014 SooKee oasookee@googlemail.com               |
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

#include <katina/log.h>
#include <katina/types.h>
#include <katina/utils.h>

#include <pcrecpp.h>

using namespace katina;
using namespace katina::log;
using namespace katina::types;
using namespace katina::utils;

#ifndef REVISION
#define REVISION "0000000"
#endif

#ifndef VERSION
#define VERSION "0.1-dev"
#endif

char ip1 = 1;
char ip2 = 1;
char ip3 = 1;
char ip4 = 1;

void inc_ip()
{
	if(++ip1 > 0)
		return;

	ip1 = 1;

	if(++ip2 > 0)
		return;

	ip1 = ip2 = 1;

	if(++ip3 > 0)
		return;

		ip1 = ip2 = ip3 = 1;

	if(++ip4 > 0)
		return;

	log("ERROR: ip address space exhausted");
	ip1 = ip2 = ip3 = ip4 = 1;
}

str get_ip()
{
	soss oss;
	oss << (int) ip4 << '.' << (int) ip3 << '.' << (int) ip2 << '.' << (int) ip1;
	return oss.str();
}

str next_ip()
{
	str ip = get_ip();
	inc_ip();
	return ip;
}

str mapped_ip(const str& ip)
{
	static str_map ips;

	if(ips.find(ip) == ips.end())
		ips[ip] = next_ip();
	return ips[ip];
}

/**
 * katina /path/to/config [$HOME/.katina]
 */
int main(const int argc, const char* argv[])
{
	// pcrecpp::RE(r).PartialMatch(s)
	log("Katina Obscure IPs v" << VERSION);

	if(argc < 1)
	{
		log("ERROR: No input file specified.");
		return 1;
	}

	sifs ifs(argv[1]);

	if(!ifs)
	{
		log("ERROR: Unable to open input file: " << argv[1]);
		return 1;
	}

	str outfile;
	pcrecpp::RE("(.*)\\.log").FullMatch(argv[1], &outfile);

	sofs ofs(outfile + "-fix.log");

	if(!ofs)
	{
		log("ERROR: Unable to open output file: " << outfile + "-fix.log");
		return 1;
	}

	pcrecpp::RE re(R"x((.*?)(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})(.*))x");
//	pcrecpp::RE re(R"x(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})x");

	str pre, ip, post;
	str line;

	while(sgl(ifs, line))
	{
		str last_ip;
		while(re.FullMatch(line, &pre, &ip, &post) && last_ip != ip)
		{
			last_ip = mapped_ip(ip);
			line = pre + last_ip + post;
		}

//		re.GlobalReplace(mapped_ip(), &line);
		ofs << line << '\n';
	}
}
