#ifndef __KATINA_ANSI_H
#define __KATINA_ANSI_H
/*
 *  Created on: 25 June 2014
 *      Author: SooKee oasookee@gmail.com
 */

/*-----------------------------------------------------------------.
| Copyright (C) 2014 SooKee oasookee@gmail.com                     |
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

#include <sstream>
#include <iostream>
#include <vector>
#include <algorithm>
#include <iostream>

namespace katina { namespace ansi {

typedef std::numeric_limits<std::streamsize> streamlimits;

extern const std::string norm;
extern const std::string col;

enum ANSI
{
	NORM = 0
	, BOLD_ON
	, FAINT_ON
	, ITALIC_ON
	, UNDERLINE_ON
	, BLINK_SLOW
	, BLINK_FAST
	, NEGATIVE
	, CONCEAL = 8
	, CONCEAL_ON = 8
	, STRIKE_ON // crossed out

	, FONT_0 = 10
	, FONT_1
	, FONT_2
	, FONT_3
	, FONT_4
	, FONT_5
	, FONT_6
	, FONT_7
	, FONT_8
	, FONT_9

	, X1 = 21
	, BOLD_OFF = 22
	, FAINT_OFF = 22
	, ITALIC_OFF
	, UNDERLINE_OFF
	, BLINK_OFF

	, POSITIVE = 27
	, REVEAL = 28
	, CONCEAL_OFF = 28
	, STRIKE_OFF // not crossed out

	, FG_BLACK = 30
	, FG_RED
	, FG_GREEN
	, FG_YELLOW
	, FG_BLUE
	, FG_MAGENTA
	, FG_CYAN
	, FG_WHITE

	, BG_BLACK = 40
	, BG_RED
	, BG_GREEN
	, BG_YELLOW
	, BG_BLUE
	, BG_MAGENTA
	, BG_CYAN
	, BG_WHITE
};

inline
std::string ansi_esc(std::initializer_list<int> codes)
{
	std::string esc = "\033[";

	std::string sep;
	for(int c: codes)
	{
		esc += sep + std::to_string(c);
		sep = ";";
	}

	return esc + "m";
}

inline
int code_to_int(char c)
{
	if(c >= '0' && c <= '9') return c - '0';
	if(c >= 'a' && c <= 'z') return (c - 'a') + 10;
	if(c >= 'A' && c <= 'Z') return (c - 'A') + 10;
	return 0;
}

static const std::string keys = "_*/~#";
static const int map[] =
{
	30, 31, 32, 33, 34, 35, 36, 37 // 0-7
	, 40, 41, 42, 43, 44, 45, 46, 47 // 8-F
	, 00, 00, ITALIC_ON, 00, 00, 00, 00, NEGATIVE
	, 00, POSITIVE, 00, 00, 00, 00, UNDERLINE_ON, 00
	, 00, 00, 00, 00
};

class ansi_fsm
{
private:
//	static const std::string keys;
//	static const int map[];

	bool esc, u, i, b, f;

public:

	ansi_fsm(): esc(0), u(0), i(0), b(0), f(0) {}

	std::ostream& write(std::ostream& os, const char* buf, size_t len)
	{
		std::string str(buf, len);
		return write(os, str);
	}

	std::ostream& write(std::ostream& os, const std::string& str)
	{
		for(char c: str)
		{
			if(c == '^')
			{
				if(esc)
				{
					esc = false;
					os << '^';
				}
				else
				{
					esc = true;
				}
			}
			else if(esc)
			{
				// 0-F
				if(keys.find(c) != std::string::npos)
				{
					os << c;
				}
				else
				{
					os << ansi_esc({map[code_to_int(c) % sizeof(map)]});
				}
				esc = false;
			}
			else if(c == '_')
			{
				os << (((u = !u)) ? ansi_esc({UNDERLINE_ON}) : ansi_esc({UNDERLINE_OFF}));
			}
			else if(c == '/')
			{
				os << (((i = !i)) ? ansi_esc({ITALIC_ON}) : ansi_esc({ITALIC_OFF}));
			}
			else if(c == '*')
			{
				os << (((b = !b)) ? ansi_esc({BOLD_ON}) : ansi_esc({BOLD_OFF}));
			}
			else if(c == '~')
			{
				os << (((f = !f)) ? ansi_esc({FAINT_ON}) : ansi_esc({FAINT_OFF}));
			}
			else if(c == '#')
			{
				os << norm;
			}
			else
			{
				os << c;
			}
		}

		return os;
	}
};

//const std::string ansi_fsm::keys = "_*/~#";
//const int ansi_fsm::map[] =
//{
//	30, 31, 32, 33, 34, 35, 36, 37 // 0-7
//	, 40, 41, 42, 43, 44, 45, 46, 47 // 8-F
//	, 00, 00, ITALIC_ON, 00, 00, 00, 00, NEGATIVE
//	, 00, POSITIVE, 00, 00, 00, 00, UNDERLINE_ON, 00
//	, 00, 00, 00, 00
//};

inline
void test()
{

//	std::string str = "0123456789ABCDefgh";
//
//	for(char c: str)
//		std::cout << c << ": " << code_to_int(c) << '\n';

	std::cout << ansi_esc({UNDERLINE_ON}) << '\n';

	ansi_fsm fsm;

	fsm.write(std::cout, "Hello ^2co^Ulor^0 and ^^ dull\n");


	fsm.write(std::cout, "Hello _^4under# ^2line# text_ and /italics/ with *bold* and ~faint~ ^^ dull\n");

//	int i;
//
//	std::cout << "Input: ";
//	while(!(std::cin >> i))
//	{
//		std::cin.clear();
//		std::cin.ignore(streamlimits::max(), '\n');
//		std::cout << "Input: ";
//	}
//
//	std::cout << "i: " << i << '\n';
}

template<typename Char>
class basic_ansibuf
: public std::basic_streambuf<Char>
{
private:

public:
	typedef Char char_type;
	typedef std::basic_streambuf<char_type> buf_type;
	typedef std::basic_ostream<char_type> stream_type;
	typedef typename buf_type::int_type int_type;
	typedef typename std::basic_streambuf<Char>::traits_type traits_type;

protected:

	static const int char_size = sizeof(char_type);
	static const int SIZE = 128;
	char_type obuf[SIZE] ;
	char_type ibuf[SIZE] ;

	ansi_fsm ifsm;
	ansi_fsm ofsm;
	std::ostream* os;

public:
	basic_ansibuf(): os(0)
	{
		buf_type::setp(obuf, obuf + (SIZE - 1));
		buf_type::setg(ibuf + 4, ibuf + 4, ibuf + 4);
	}

	basic_ansibuf(std::ostream& os): os(&os)
	{
		buf_type::setp(obuf, obuf + (SIZE - 1));
		buf_type::setg(ibuf + 4, ibuf + 4, ibuf + 4);
	}

	virtual ~basic_ansibuf() { sync(); }

	void set_os(std::ostream& os) { this->os = &os; }
	std::ostream& get_os() { return *this->os; }

protected:

	int output_buffer()
	{
		int num = buf_type::pptr() - buf_type::pbase();
		//if(send(sock, reinterpret_cast<char*>(obuf), num * char_size, 0) != num)
		if(!ofsm.write(*os, reinterpret_cast<char*>(obuf), num * char_size))
		{
			return traits_type::eof();
		}
		buf_type::pbump(-num);
		return num;
	}

	virtual int_type overflow(int_type c)
	{
		if(c != traits_type::eof())
		{
			*buf_type::pptr() = c;
			buf_type::pbump(1);
		}
		if(output_buffer() == traits_type::eof())
		{
			return traits_type::eof();
		}
		return c;
	}

	virtual int sync()
	{
		if(output_buffer() == traits_type::eof())
		{
			return -1;
		}
		return 0;
	}

//	virtual int_type underflow()
//	{
//		if(buf_type::gptr() < buf_type::egptr())
//		{
//			return *buf_type::gptr();
//		}
//
//		int numPutback;
//		numPutback = buf_type::gptr() - buf_type::eback();
//		if(numPutback > 4)
//		{
//			numPutback = 4;
//		}
//
//		std::copy(ibuf + (4 - numPutback)
//			, ibuf + (4 - numPutback) + numPutback
//			, buf_type::gptr() - numPutback);
//		int num;
//		if((num = recv(sock
//			, reinterpret_cast<char*>(ibuf + 4)
//			, (SIZE - 4) * char_size, 0)) <= 0)
//		{
//			return traits_type::eof();
//		}
//		buf_type::setg(ibuf + (4 - numPutback), ibuf + 4, ibuf + 4 + num);
//		return *buf_type::gptr();
//	}
};

typedef basic_ansibuf<char> ansibuf;
typedef basic_ansibuf<wchar_t> wansibuf;

template<typename Char>
class basic_ansistream
: public std::basic_iostream<Char>
{
public:
	typedef Char char_type;
	typedef std::basic_iostream<char_type> stream_type;
	typedef basic_ansibuf<char_type> buf_type;

protected:
	buf_type buf;

public:
	basic_ansistream(): stream_type(&buf) {}
	basic_ansistream(std::ostream& os): stream_type(&buf) { buf.set_os(os); }
};

typedef basic_ansistream<char> ansistream;
typedef basic_ansistream<wchar_t> wansistream;

}} // katina::ansi

#endif // __KATINA_ANSI_H
