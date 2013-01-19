/*
 * oastats.cpp
 *
 *  Created on: Jan 2, 2013
 *      Author: oasookee@gmail.com
 */

#define DEBUG 1

#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <map>
#include <memory>
#include <cassert>
#include <vector>

#include "stl.h"
#include "codes.h"
#include "types.h"
#include "logrep.h"

#include <pcrecpp.h>
#include <mysql.h>

using namespace oastats;
using namespace oastats::types;
using namespace oastats::utils;

#ifndef STAMP
#define STAMP
#endif
#ifndef REVISION
#define REVISION
#endif
#ifndef COMMITS
#define COMMITS
#endif
#ifndef DEV
#define DEV
#endif

const str VERSION = STAMP "-" REVISION "-" COMMITS DEV;

struct LineProcessor
{
	/**
	 * @return false if parsing failed.
	 */
	virtual bool operator()(const str& line) = 0;
};

struct DataBaseWriter
{
	virtual bool create() = 0;
};

typedef std::shared_ptr<LineProcessor> LineProcessorSPtr;

bool log_error(const str& m, bool flag = false)
{
	log(m);
	return flag;
}

struct kd_t
{
	siz k;
	siz d;
	kd_t(): k(0), d(0) {}
};

typedef std::map<str, kd_t> kd_map;
typedef std::pair<const str, kd_t> kd_pair;

class KDLineProcessor // kills/deaths
: public LineProcessor
{

private:
	const siz weapon;

	kd_map m;
	siz_str_map codes; // game codes -> guid

	// working variables
	str data;
	std::istringstream iss;
	siz w; // weapon code
	siz p1, p2; // player codes
	str guid;

	pcrecpp::RE re_guid;

public:
	KDLineProcessor(const siz weapon): weapon(weapon), re_guid(".*\\\\(.*)") {}

	const kd_map& get_kds() const
	{
		return m;
	}

	bool wdb()
	{
		MYSQL *conn;

		conn = mysql_init(NULL);
		mysql_real_connect(conn, "localhost", "zetcode", "passwd", "testdb", 0, NULL, 0);
		mysql_close(conn);
	}

	bool operator()(const str& line)
	{
//		bug_func();
//		bug_var(line);

		iss.clear();
		iss.str(line);

		iss >> data >> data;
		//  0:23 ClientUserinfoChanged: 5 n\^1Lord ^2Zeus\t\2\model\gargoyle/stone\hmodel\gargoyle/stone\g_redteam\\g_blueteam\\c1\1\c2\1\hc\100\w\0\l\0\tt\0\tl\0\id\901B30E426727757588FB964A07F901E
		if(iss && data == "ClientUserinfoChanged:" && (iss >> p1) && std::getline(iss, data))
		{
			if(!re_guid.FullMatch(data, &guid))
				return log_error(re_guid.error());

			codes[p1] = guid;

			return true;
		}

		//  0:48 Kill: 1 4 10: (drunk)Mosey killed ^1S^3amus ^1A^3ran by MOD_RAILGUN
		if(iss && data == "Kill:" && (iss >> p1 >> p2 >> w) && w == weapon)
		{
			// p1 killed 2 by w
			if(codes[p1].empty() || codes[p2].empty()) // avoid bots
				return true;

			++m[codes[p1]].k;
			++m[codes[p2]].d;

			return true;
		}

		return false;
	}
};

typedef std::map<str, str_siz_map> ovo_map; // guid -> {guid -> kills}
typedef std::pair<const str, str_siz_map> ovo_pair;

class OvOLineProcessor // 1v1
: public LineProcessor
{
private:
	ovo_map ovos;
	siz_str_map codes; // game codes -> guid

	// working variables
	str data;
	std::istringstream iss;
	siz w; // weapon code
	siz p1, p2; // player codes
	str guid;

	pcrecpp::RE re_guid;

public:
	OvOLineProcessor(): re_guid(".*\\\\(.*)") {}

	const ovo_map& get_ovos()
	{
		return ovos;
	}

	bool operator()(const str& line)
	{
//		bug_func();
//		bug_var(line);

		iss.clear();
		iss.str(line);

		iss >> data >> data;
		//  0:23 ClientUserinfoChanged: 5 n\^1Lord ^2Zeus\t\2\model\gargoyle/stone\hmodel\gargoyle/stone\g_redteam\\g_blueteam\\c1\1\c2\1\hc\100\w\0\l\0\tt\0\tl\0\id\901B30E426727757588FB964A07F901E
		if(iss && data == "ClientUserinfoChanged:" && (iss >> p1) && std::getline(iss, data))
		{
			if(!re_guid.FullMatch(data, &guid))
				return log_error(re_guid.error());

			codes[p1] = guid;

			return true;
		}

		//  0:48 Kill: 1 4 10: (drunk)Mosey killed ^1S^3amus ^1A^3ran by MOD_RAILGUN
		if(iss && data == "Kill:" && (iss >> p1 >> p2 >> w))// && w == to_underlying(weapon))
		{
			// p1 killed p2 by w
			if(codes[p1].empty() || codes[p2].empty()) // avoid bots
				return true;

			++ovos[codes[p1]][codes[p2]];
			ovos[codes[p2]][codes[p1]];

			return true;
		}

		return false;
	}
};

int main(int argc, char* argv[])
{
	bug_func();
	bug_var(argc);

	con("oastats: " << VERSION);
	con("");

	std::ifstream ifs;
	std::ofstream ofs;

	if(argc > 1)
	{
		ifs.open(argv[1]);
		if(!ifs)
		{
			std::cerr << "Error opening input file: " << argv[1] << '\n';
			std::exit(1);
		}
	}

	if(argc > 2)
	{
		ofs.open(argv[2]);
		if(!ofs)
		{
			std::cerr << "Error opening ouput file: " << argv[2] << '\n';
			std::exit(2);
		}
	}

	std::istream& is = argc > 1 ? ifs : std::cin;
	std::ostream& os = argc > 2 ? ofs : std::cout;

	KDLineProcessor* kdlp = new KDLineProcessor(MOD_RAILGUN);
	OvOLineProcessor* ovolp = new OvOLineProcessor();

	std::vector<LineProcessorSPtr> processors;

//	processors.push_back(LineProcessorSPtr(kdlp));
	processors.push_back(LineProcessorSPtr(ovolp));

	str line;
	while(std::getline(is, line))
	{
//		bug_var(line);
//		bug("");
		for(LineProcessorSPtr& processor: processors)
			(*processor)(line);
	}

	for(const kd_pair& p: kdlp->get_kds())
		con(p.first << ": " << p.second.k << "k " << p.second.d << "d");

	const ovo_map& ovos = ovolp->get_ovos();
	for(const ovo_pair& p: ovos)
		for(const str_siz_pair& q: p.second)
		{
			con(p.first << " killed " << q.first << " " << ovos.at(p.first).at(q.first) << " times");
			con(q.first << " killed " << p.first << " " << ovos.at(q.first).at(p.first) << " times");
			con("");
		}

}
