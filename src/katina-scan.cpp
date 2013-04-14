/*
 * katina.cpp
 *
 *  Created on: Apr 14, 2013
 *      Author: SooKee oasookee@gmail.com
 */

#include "Katina.h"

#include <signal.h>
#include <execinfo.h>
#include <cxxabi.h>

using namespace oastats;

void stack_handler(int sig)
{
	con("Error: signal " << sig);

	void *array[2048];
	size_t size;

	// get void*'s for all entries on the stack
	size = backtrace(array, 2048);

	// print out all the frames to stderr
	char** trace = backtrace_symbols(array, size);

	int status;
	str obj, func;
	for(siz i = 0; i < size; ++i)
	{
		siss iss(trace[i]);
		std::getline(std::getline(iss, obj, '('), func, '+');

		char* func_name = abi::__cxa_demangle(func.c_str(), 0, 0, &status);
		std::cerr << "function: " << func_name << '\n';
		free(func_name);
	}
	free(trace);
	exit(1);
}

int main(const int argc, const char* argv[])
{
	signal(11, stack_handler);
	Katina katina;
	return katina.run(argc, argv);
}





