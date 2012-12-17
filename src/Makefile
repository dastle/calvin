#
# This makefile system follows the structuring conventions
# recommended by Peter Miller in his excellent paper:
#
#       Recursive Make Considered Harmful
#       http://aegis.sourceforge.net/auug97.pdf
#

# We create a listing of the root directories for access into
OBJDIR := ../obj
BINDIR := ../bin
EXTDIR := ../ext
SRCDIR := ../src
DOCDIR := ../doc
LOGDIR := ../logs

# Next, we define the external libraries we use and their directories
ZEROMQ  := $(EXTDIR)/zeromq
PROTOB  := $(EXTDIR)/protobuf
GTEST   := $(EXTDIR)/googletest
ZOOKPR  := $(EXTDIR)/zookeeper
GLOG    := $(EXTDIR)/glog
LEVELDB := $(EXTDIR)/leveldb
FLEX    := $(EXTDIR)/flex
BISON   := $(EXTDIR)/bison

# Doxygen specific information
DOXY_EXE  := doxygen
DOXY_CONF := Calvin.doxygen
DOCOUT    := ../doc/html

# Executable for protocol buffers, flex and bison
PROTOCEXE := $(PROTOB)/src/.libs/protoc
FLEXEXE   := $(FLEX)/flex
BISONEXE  := $(BISON)/src/bison

# '$(V)' controls whether the lab makefiles print verbose commands (the
# actual shell commands run by Make), as well as the "overview" commands
# (such as '+ cc lib/readline.c').
#
# For overview commands only, the line should read 'V = @'.
# For overview and verbose commands, the line should read 'V ='.
V = @

# Set the compiler and compile-time loaded libraries
CXX           := g++
LDLIBRARYPATH := BISON_PKGDATADIR=$(BISON)/data LD_LIBRARY_PATH=$(ZEROMQ)/src/.libs:$(PROTOB)/src/.libs:$(GTEST)/lib/.libs:$(ZOOKPR)/.libs:$(GLOG)/lib64:$(LEVELDB)

# For profiling (with gprof), this line should read 'PG = -pg'
# For fast execution, this line should read 'PG ='.
PG =

# Set the flags for C++ to compile with (namely where to look for external
# libraries) and the linker libraries (again to look in the ext/ library)
CXXFLAGS := -g -MD $(PG) -I$(SRCDIR) -I$(OBJDIR) \
            -I$(ZEROMQ)/include \
            -I$(PROTOB)/src \
            -I$(GTEST)/include \
            -I$(ZOOKPR)/include -I$(ZOOKPR)/generated \
            -I$(GLOG)/include \
            -I$(LEVELDB)/include
CXXFLAGS += -Wall

# TODO(kun): get zookeeper working
LDFLAGS := -lgtest -L$(GTEST)/lib/.libs -lpthread \
           -lglog -L$(GLOG)/lib64 \
           -lleveldb -L$(LEVELDB) \
           -lrt $(PG) \
           -lprotobuf -L$(PROTOB)/src/.libs \
           -lreadline
#           -lzookeeper_mt -L$(ZOOKPR)/.libs

ZMQLDFLAGS := -lzmq -L$(ZEROMQ)/src/.libs

# Lists that the */Makefile.inc makefile fragments will add to
OBJDIRS :=
TESTS :=

# Make sure that 'all' is the first target
all:
	@mkdir -p $(LOGDIR)

# Documentation
doxygen:
	$(warning + doc $(DOXY_EXE) $(DOCDIR)/$(DOXY_CONF) > $(DOCOUT))
	$(shell cd $(DOCDIR); $(DOXY_EXE) $(DOXY_CONF))
	
# Makefile template so that the makefrags are far less redundant
MAKEFILE_TEMPLATE := Makefile.template

# python obj file used by some components
PYTHON_OBJS := /usr/lib64/libpython2.7.so

# Makefile fragments for library code
include proto/Makefile.inc
include query_processor/ast/Makefile.inc
include query_processor/flex_bison/Makefile.inc
include common/Makefile.inc
include connection/Makefile.inc
include process_mgr/Makefile.inc
include backend/Makefile.inc
include query_processor/Makefile.inc
include stored_procedures/Makefile.inc
include checkpointing/Makefile.inc
#include paxos/Makefile.inc  # TODO(kun): uncomment this once zookeeper is working

# Makefile fragments for components
include sequencer/Makefile.inc
include scheduler/Makefile.inc
include deploy/Makefile.inc

# Makefile fragments for executable and deployment scripts
include include/Makefile.inc
include bin/Makefile.inc

test: $(TESTS)
	@mkdir -p $(LOGDIR)
clean:
	rm -rf $(OBJDIR) $(BINDIR) $(DOCOUT) $(LOGDIR)

# This magic automatically generates makefile dependencies
# for header files included from C source files we compile,
# and keeps those dependencies up-to-date every time we recompile.
# See 'mergedep.pl' for more information.
$(OBJDIR)/.deps: $(foreach dir, $(OBJDIRS), $(wildcard $(OBJDIR)/$(dir)/*.d))
	@mkdir -p $(@D)
	@cat $^ /dev/null > $@
#	@$(PERL) mergedep.pl $@ $^
-include $(OBJDIR)/.deps

always:

# Eliminate default suffix rules
.SUFFIXES:

# make it so that no intermediate .o files are ever deleted
.PRECIOUS: $(foreach dir, $(OBJDIRS), $(OBJDIR)/$(dir)/%.o) \
           $(foreach dir, $(OBJDIRS), $(OBJDIR)/$(dir)/%.pb.cc) \
           $(foreach dir, $(OBJDIRS), $(OBJDIR)/$(dir)/%.pb.h)

.PHONY: all always clean test
