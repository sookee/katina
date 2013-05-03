/*
 * ircbot.cpp
 *
 *  Created on: 29 Jul 2011
 *      Author: oaskivvy@gmail.com
 */

/*-----------------------------------------------------------------.
| Copyright (C) 2011 SooKee oaskivvy@gmail.com               |
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

#include <katina/message.h>

#include <katina/types.h>
#include <katina/str.h>
#include <katina/log.h>

#define CSTRING(s) s,sizeof(s)
#define STRC(c) str(1, c)

namespace oastats { namespace ircbot {

using namespace oastats::types;
using namespace oastats::string;

//std::istream& operator>>(std::istream& is, message& m)
//{
//	str o;
//	if(!getobject(is, o))
//		return is;
//	if(!parsemsg(unescaped(o), m))
//		is.setstate(std::ios::failbit); // protocol error
//	return is;
//}
//
//std::ostream& operator<<(std::ostream& os, const message& m)
//{
//	return os << '{' << escaped(m.line) << '}';
//}

bool message::from_channel() const
{
	str_vec params = get_params();
	return !params.empty() && !params[0].empty() && params[0][0] == '#';
}

str message::get_to() const
{
	str_vec params = get_params();
	if(!params.empty())
		return params[0];
	return "";
}

str message::reply_to() const
{
	if(!from_channel())
		return get_nickname();
	return get_to();
}

str message::get_user_cmd() const
{
	str cmd;
	siss iss(get_trailing());
	std::getline(iss, cmd, ' ');
	return trim(cmd);
}

str message::get_user_params() const
{
	str params;
	siss iss(get_trailing());
	std::getline(iss, params, ' ');
	if(!std::getline(iss, params))
		return "";
	return trim(params);
}

str message::get_nick() const
{
	return get_nickname(); // alias
}

str message::get_userhost() const
{
	return prefix.substr(prefix.find("!") + 1);
}

static const std::vector<str_set>& get_chan_params()
{

	static std::vector<str_set> chan_params;

	if(chan_params.empty())
	{
		chan_params.resize(3);

		chan_params[0].insert("PRIVMSG");
		chan_params[0].insert("JOIN");
		chan_params[0].insert("MODE");
		chan_params[0].insert("KICK");
		chan_params[0].insert("PART");
		chan_params[0].insert("TOPIC");

		chan_params[1].insert("332");
		chan_params[1].insert("333");
		chan_params[1].insert("366");
		chan_params[1].insert("404");
		chan_params[1].insert("474");
		chan_params[1].insert("482");
		chan_params[1].insert("INVITE");

		chan_params[2].insert("353");
		chan_params[2].insert("441");
	}
}

static const str chan_start = "#&+!";

str message::get_chan() const
{
	str_vec params = get_params();

	for(siz i = 0; i < get_chan_params().size(); ++i)
		if(get_chan_params()[i].count(command) && params.size() > i)
			if(!params[i].empty() && std::count(chan_start.begin(), chan_start.end(), params[i][0]))
				return params[i];
	return "";
}

std::ostream& printmsg(std::ostream& os, const message& m)
{
	os << "//                  line: " << m.line << '\n';
	os << "//                prefix: " << m.prefix << '\n';
	os << "//               command: " << m.command << '\n';
	os << "//                params: " << m.params << '\n';
	os << "// get_servername()     : " << m.get_servername() << '\n';
	os << "// get_nickname()       : " << m.get_nickname() << '\n';
	os << "// get_user()           : " << m.get_user() << '\n';
	os << "// get_host()           : " << m.get_host() << '\n';
	str_vec params = m.get_params();
	for(siz i = 0; i < params.size(); ++i)
		os << "// param                : " << params[i] << '\n';
	str_vec middles = m.get_middles();
	for(siz i = 0; i < middles.size(); ++i)
		os << "// middle               : " << middles[i] << '\n';
	os << "// trailing             : " << m.get_trailing() << '\n';
	os << "// get_nick()           : " << m.get_nick() << '\n';
	os << "// get_chan()           : " << m.get_chan() << '\n';
	os << "// get_user_cmd()       : " << m.get_user_cmd() << '\n';
	os << "// get_user_params()    : " << m.get_user_params() << '\n';
	return os << std::flush;
}

void bug_message(const std::string& K, const std::string& V, const message& msg)
{
	bug("//================================");
	bug("// " << K << ": " << V);
	bug("//--------------------------------");
//	bug_msg(msg);
	bug("//--------------------------------");
}

const str message::nospcrlfcl(CSTRING("\0 \r\n:")); // any octet except NUL, CR, LF, " " and ":"
const str message::nospcrlf(CSTRING("\0 \r\n")); // (":" / nospcrlfcl)
const str message::nocrlf(CSTRING("\0\r\n")); // (":" / nospcrlfcl)
const str message::nospcrlfat(CSTRING("\0 \r\n@")); // any octet except NUL, CR, LF, " " and "@"
const str message::bnf_special(CSTRING("[]\\`_^{|}")); // "[", "]", "\", "`", "_", "^", "{", "|", "}"

}} // oastats::ircbot
