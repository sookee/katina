# oastats Makefile

DEBUG = 1
RELEASE = 0
PREFIX = /use/local

include Makefile.local # override defaults

src = src

REVISION = $(shell git log -n 1 --pretty=format:%h|tr [:lower:] [:upper:])

REVISION_FLAGS = -D REVISION=\"$(REVISION)\"

MYSQL_INCL = $(shell mysql_config --include)
MYSQL_LIBS = $(shell mysql_config --libs)

LIBGCRYPT_FLAGS = $(shell libgcrypt-config --cflags)
LIBGCRYPT_LIBS = $(shell libgcrypt-config --libs)

SOOKEE_INCL = $(PREFIX)/include
SOOKEE_LIBS = $(PREFIX)/lib/sookee

INCLUDES = $(wildcard $(src)/include/katina/*.h)

all: libkatina.so katina plugins

build: 
	mkdir -p $(BUILD_DIR)/bin
	mkdir -p $(BUILD_DIR)/lib

$(BUILD_DIR)/bin/katina: katina.o
	$(CXX) -o $(BUILD_DIR)/bin/$< -L$(BUILD_DIR)/lib -lkatina -L$(SOOKEE_LIBS) -lsookee $@

$(BUILD_DIR)/lib/libkatina.so.0.0.0: katina.o

.PHONY all build plugins

