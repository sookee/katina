# Makefile

src_topdir = ../../../src

CXXFLAGS = -std=c++11 -fPIC -D DEBUG -O0 -g3 -I$(src_topdir)/include

LIBOACOM_FLAGS = -D DEBUG -O0 -g3 -fPIC -pthread
LIBOACOM_LIBS =  -lrt -pthread -L.. -lkatina
LIBOACOM_OBJS = net_ip.o net_chan.o msg.o

LIBRARIES = liboacom.so

PREFIX ?= $(HOME)

all: $(addsuffix .0.0.0, $(LIBRARIES))

liboacom.so.0.0.0: $(LIBOACOM_OBJS)
	@echo
	@echo Compiling $@
	g++ -shared -Wl,-soname,liboacom.so.0 -o liboacom.so.0.0.0 $(LIBOACOM_OBJS) $(LIBOACOM_LIBS)
	@ln -f -s liboacom.so.0.0.0 liboacom.so.0.0
	@ln -f -s liboacom.so.0.0 liboacom.so.0
	@ln -f -s liboacom.so.0 liboacom.so
	@echo

%.o: $(src_topdir)/oacom/%.cpp $(src_topdir)/oacom/%.h
	@echo -n "AUTO: "
	$(CXX) $(CXXFLAGS) -c -o $@ $<

install: $(LIBRARIES)
	mkdir -p $(PREFIX)/lib
	@for library in $(LIBRARIES); \
	do \
		cp -f $$library.0.0.0 $(PREFIX)/lib; \
		cd $(PREFIX)/lib; \
		ln -f -s $$library.0.0.0 $$library.0.0; \
		ln -f -s $$library.0.0 $$library.0; \
		ln -f -s $$library.0 $$library; \
	done

clean:
	rm -f *.o *.so* $(LIBRARIES)

