#program_NAME := find-open-rows
program_NAME := row-clone-exe
program_CXX_SRCS := test.cpp ../util.cpp $(wildcard ../../../api/*.c) $(wildcard ../../../api/*.cpp)
#program_CXX_SRCS := test_find_open_rows.cpp $(wildcard ../../api/*.c) $(wildcard ../../api/*.cpp)
program_CXX_OBJS := ${program_CXX_SRCS:.cpp=.o}
program_CXX_OBJS := ${program_CXX_OBJS:.c=.o}
program_OBJS := $(program_CXX_OBJS)
program_INCLUDE_DIRS := ../../../api ../../../../boost-lib
program_LIBRARY_DIRS :=
program_LIBRARIES := 
CPPFLAGS += -g -std=c++11 -pthread -O3

CPPFLAGS += $(foreach includedir,$(program_INCLUDE_DIRS),-I$(includedir))
LDFLAGS += $(foreach librarydir,$(program_LIBRARY_DIRS),-L$(librarydir))
LDFLAGS += $(foreach library,$(program_LIBRARIES),-l$(library))

CC=g++

.PHONY: all clean distclean

all: $(program_NAME)

$(program_NAME): $(program_OBJS)
	$(CC) $(CPPFLAGS) $(program_OBJS) -o $(program_NAME) $(LDFLAGS)

clean:
	@- $(RM) $(program_NAME)
	@- $(RM) $(program_OBJS)

parser: 
	$(MAKE) -C ../../../api/lexyacc
	cp ../../../api/lexyacc/smc_parser .

distclean: clean
