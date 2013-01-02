
GCC_HOME = $(HOME)/app/gcc-4.7.0

DEF_FLAGS = \
	-D _GLIBCXX_USE_NANOSLEEP \
	-D _GLIBCXX_USE_SCHED_YIELD \
	-D STAMP=\"$(shell date +%Y%m%d-%H%M%S)\" \
	-D DEV=\"$(shell git diff --quiet || echo -dev)\" \
	-D COMMITS=\"$(shell printf %04d \"$(git log --after={yesterday} --pretty=oneline|wc -l)\")\" \
	-D REVISION=\"$(shell git log -n 1 --pretty=format:%h|tr [:lower:] [:upper:])\"

CXX = $(GCC_HOME)/bin/g++
CXXFLAGS = -std=c++11 $(DEF_FLAGS)

all: oastats

clean:
	rm -f *.o *.so oastats