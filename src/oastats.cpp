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

#include "stl.h"
#include "codes.h"
#include "types.h"
#include "logrep.h"

#include <pcrecpp.h>

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
	virtual bool operator()(const str& line) = 0;
};

typedef std::shared_ptr<LineProcessor> LineProcessorSPtr;

class KDLineProcessor // kills/deaths
: public LineProcessor
{
private:
	siz iid = 0;
	const WEAPON weapon;
	siz_map k; // internal codes -> score
	siz_map d; // internal codes -> score

	str_siz_map names; // GUID -> internal codes
	siz_map codes; // game codes -> internal codes

	str data;
	std::istringstream iss;
	siz w; // weapon code
	siz p1, p2; // player codes
	str guid;

	pcrecpp::RE re_guid;

public:
	KDLineProcessor(const WEAPON weapon): weapon(weapon), re_guid(".*\\\\(.*)") {}

	str_siz_map get_kills()
	{
		str_siz_map m;
		for(const str_siz_pair& p: names)
			m[p.first] = k[p.second];
		return m;
	}

	str_siz_map get_deaths()
	{
		str_siz_map m;
		for(const str_siz_pair& p: names)
			m[p.first] = d[p.second];
		return m;
	}

	bool operator()(const str& line)
	{
//		bug_func();
//		bug_var(line);

		iss.str(line);

		iss >> data >> data;
		//  0:23 ClientUserinfoChanged: 5 n\^1Lord ^2Zeus\t\2\model\gargoyle/stone\hmodel\gargoyle/stone\g_redteam\\g_blueteam\\c1\1\c2\1\hc\100\w\0\l\0\tt\0\tl\0\id\901B30E426727757588FB964A07F901E
		if(iss && data == "ClientUserinfoChanged:" && (iss >> p1) && std::getline(iss, data))
		{
			if(!re_guid.FullMatch(data, &guid))
			{
				log(re_guid.error());
				return false;
			}

//			bug_var(p1);
//			bug_var(guid);

			if(guid.empty()) // bot
				codes.erase(p1);
			else
			{
				if(!names.count(guid)) // new guid needs internal code
				{
					names[guid] = iid++;
					//codes[p1] = names[guid];
					bug_var(names[guid]);
				}

				codes[p1] = names[guid]; // map game to new internal
//				{
//					log("error: Unknown player: " << p1);
//					log("line : " << line);
//				}
			}

			return true;
		}

		//  0:48 Kill: 1 4 10: (drunk)Mosey killed ^1S^3amus ^1A^3ran by MOD_RAILGUN
		if(iss && data == "Kill:" && (iss >> p1 >> p2 >> w) && w == to_underlying(weapon))
		{
			// p1 killed 2 by w
//			bug_var(p1);
//			bug_var(p2);
//			bug_var(w);

			if(codes.count(p1) && codes.count(p2)) // avoid bots
			{
				++k[codes[p1]];
				++d[codes[p2]];
			}
			return true;
		}

		iss.clear();

		return false;
	}
};

int main(int argc, char* argv[])
{
	bug_func();
	bug_var(argc);
	std::cout << "oastats: " << VERSION << '\n';

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

	KDLineProcessor* kdlp = new KDLineProcessor(WEAPON::MOD_RAILGUN);

	std::vector<LineProcessorSPtr> processors;

	processors.push_back(LineProcessorSPtr(kdlp));
	//processors.push_back(LineProcessorSPtr(new KDLineProcessor(WEAPON::MOD_GAUNTLET)));

	str line;
	while(std::getline(is, line))
	{
		//bug_var(line);
		for(LineProcessorSPtr& processor: processors)
			(*processor)(line);
	}

	str_siz_map m;
	for(LineProcessorSPtr& processor: processors)
	{
		for(const str_siz_pair p: kdlp->get_kills())
			con(p.first << ": " << p.second << " kills");
		for(const str_siz_pair p: kdlp->get_deaths())
			con(p.first << ": " << p.second << " deaths");
	}


}
