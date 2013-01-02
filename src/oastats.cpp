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
#include <initializer_list>

#include "types.h"

using namespace oastats::types;

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

class KillsLineProcessor
: public LineProcessor
{
private:
	str_map k;
	str_map d;

public:
	bool operator()(const str& line)
	{

	}
};

int main(int argc, char* argv[])
{
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

	str line;
	while(std::getline(is, line))
		;//process(line);
}
