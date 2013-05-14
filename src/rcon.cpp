/*
 * rcon.cpp
 *
 *  Created on: 07 Apr 2013
 *      Author: oasookee@gmail.com
 */

#include <katina/rcon.h>
#include <katina/str.h>
#include <katina/log.h>
#include <katina/time.h>

// rcon
#include <cerrno>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netdb.h>

namespace oastats { namespace net {

using namespace oastats::log;
using namespace oastats::time;
using namespace oastats::types;
using namespace oastats::string;

// -- RCON ----------------------------------------------

/**
 * IPv4 IPv6 agnostic OOB (out Of Band) comms
 * @param cmd
 * @param packets Returned packets
 * @param host Host to connect to
 * @param port Port to connect on
 * @param wait Timeout duration in milliseconds
 * @return false if failed to connect/send or receive else true
 */
bool aocom(const str& cmd, str_vec& packets, const str& host, int port
	, siz wait)
{
	addrinfo hints;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; // AF_INET or AF_INET6
	hints.ai_socktype = SOCK_DGRAM;
	
	static milliseconds last = 0;
	
	milliseconds now = 0;
	bug_var(now);
	bug_var(last);
	bug_var(now - last);
	while((now = get_millitime()) - last < 1500)
		thread_sleep_millis(1500 + last - now);
	
	last = now;
	
	bug_var(cmd);
	
	addrinfo* res;
	if(int status = getaddrinfo(host.c_str(), to_string(port).c_str(), &hints, &res) != 0)
	{
		log("aocom: getaddrinfo(): " << gai_strerror(status));
		return false;
	}

	milliseconds timeout = now + wait;

	// try to connect to each
	int cs;
	addrinfo* p;
	for(p = res; p; p = p->ai_next)
	{
		if((cs = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
			continue;
		if(!connect(cs, p->ai_addr, p->ai_addrlen))
			break;
		::close(cs);
	}

	freeaddrinfo(res);

	if(!p)
	{
		log("aocom: failed to connect: " << host << ":" << port);
		::close(cs);
		return false;
	}

	// cs good

	const str msg = "\xFF\xFF\xFF\xFF" + cmd;

	int n = 0;
	if((n = send(cs, msg.c_str(), msg.size(), 0)) < 0 || n < (int)msg.size())
	{
		log("cs send: " << strerror(errno));
		::close(cs);
		return false;
	}

	packets.clear();

	char buf[2048];

	n = sizeof(buf);
	while(n == sizeof(buf))
	{
		while((n = recv(cs, buf, sizeof(buf), MSG_DONTWAIT)) ==  -1 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR))
		{
			if(get_millitime() > timeout)
			{
				log("socket timed out connecting to: " << host << ":" << port);
				::close(cs);
				return false;
			}
			thread_sleep_millis(10);
		}
		if(n < 0)
			log("cs recv: " << strerror(errno));
		if(n > 0)
			packets.push_back(str(buf, n));
	}

	close(cs);
	return true;
}

bool rcon(const str& cmd, str& reply, const str& host, int port, siz wait)
{
	str_vec packets;
	if(!aocom(cmd, packets, host, port, wait))
		return false;

	const str header = "\xFF\xFF\xFF\xFFprint\x0A";

	if(packets.empty())
	{
		log("rcon: Empty response.");
		return false;
	}

	reply.clear();
	for(str_vec_iter packet = packets.begin(); packet != packets.end(); ++packet)
	{
		if(packet->find(header) != 0)
		{
			log("rcon: Unrecognised response.");
			return false;
		}

		reply.append(packet->substr(header.size()));
	}

	return true;
}

}} // oastats::net
