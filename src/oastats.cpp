/*
 * oastats.cpp
 *
 *  Created on: Jan 2, 2013
 *      Author: oasookee@gmail.com
 */

#include <iostream>

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

int main()
{
	std::cout << VERSION << '\n';
}
