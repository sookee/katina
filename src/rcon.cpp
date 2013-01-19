/*
 * rcon.h
 *
 *  Created on: 01 June 2012
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

#include <cerrno>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <thread>
#include <chrono>

#include <sstream>

#include "logrep.h"
#include "rcon.h"
#include "types.h"

namespace oastats { namespace net {

using namespace oastats;
using namespace oastats::utils;
using namespace oastats::types;

bool rcon(const str& cmd, str& res, const str& host, int port)
{
	// One mutex per server:port to ensure that all threads
	// accessing the same server:port pause for a minimum time
	// between calls to avoid flood protection.
	static std::map<str, std::unique_ptr<std::mutex>> mtxs;

	int cs = socket(PF_INET, SOCK_DGRAM, 0);

	if(cs <= 0)
	{
		log(strerror(errno));
		return false;
	}

	sockaddr_in sin;
	hostent* he = gethostbyname(host.c_str());
	std::copy(he->h_addr, he->h_addr + he->h_length
		, reinterpret_cast<char*>(&sin.sin_addr.s_addr));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);

	if(connect(cs, reinterpret_cast<sockaddr*>(&sin), sizeof(sin)) < 0)
	{
		log(strerror(errno));
		return false;
	}

	fcntl(cs, F_SETFL, O_NONBLOCK);

	const str key = host + ":" + std::to_string(port);

	if(!mtxs[key].get())
		mtxs[key].reset(new std::mutex);

	// keep out all threads for the same server:port until the minimum time
	// has elapsed
	lock_guard lock(*mtxs[key]);
	st_time_point pause = st_clk::now() + std::chrono::milliseconds(1000);

	const str msg = "\xFF\xFF\xFF\xFF" + cmd;
	if(send(cs, msg.c_str(), msg.size(), 0) < 1)
	{
		log(strerror(errno));
		return false;
	}

	res.clear();

	int len;
	char buf[1024];
	for(siz i = 0; i < 10; ++i)
	{
		for(; (len = read(cs, buf, 1024)) > 9; i = 0)
			res.append(buf + 10, len - 10);
		if(len > 0)
			res.append(buf, len);
		else if(len == 0)
			break;
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
	}

	close(cs);

	std::this_thread::sleep_until(pause);

	return true;
}

}} // oastats::net

