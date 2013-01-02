/*
 * oastats.cpp
 *
 *  Created on: Jan 2, 2013
 *      Author: oasookee@gmail.com
 */

#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <map>
#include <memory>

#define DEBUG 1

#include "codes.h"
#include "types.h"
#include "logrep.h"

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
	typedef std::map<siz, siz> kd_map;

private:
	const WEAPON weapon;
	kd_map k;
	kd_map d;

	str data;
	std::istringstream iss;
	siz w; // weapon code
	siz p1, p2; // player codes

public:
	KDLineProcessor(const WEAPON weapon): weapon(weapon) {}

	bool operator()(const str& line)
	{
		bug_func();
		bug_var(line);
		//  0:48 Kill: 1 4 10: (drunk)Mosey killed ^1S^3amus ^1A^3ran by MOD_RAILGUN
		iss.str(line);
//		if((iss >> data >> data) && data == "Kill:" && (iss >> p1 >> p2 >> w) && w == weapon)
		if((iss >> data >> data) && data == "Kill:")
		{
			if((iss >> p1 >> p2 >> w) && w == to_underlying(weapon))
			// p1 killed 2 by w
			++k[p1];
			++d[p2];
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

	std::vector<LineProcessorSPtr> processors;

	processors.push_back(LineProcessorSPtr(new KDLineProcessor(WEAPON::MOD_RAILGUN)));
	processors.push_back(LineProcessorSPtr(new KDLineProcessor(WEAPON::MOD_GAUNTLET)));

	str line;
	while(std::getline(is, line))
	{
		bug_var(line);
		for(LineProcessorSPtr& processor: processors)
			(*processor)(line);
	}
}
