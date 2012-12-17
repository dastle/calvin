OBJDIRS += paxos

# Create template specific variables
UPPERC_DIR := PAXOS
LOWERC_DIR := paxos

PAXOS_SRCS          := paxos/paxos.cc
PAXOS_EXES   :=

SRC_LINKED_OBJS     :=
TEST_LINKED_OBJS    := $(PROTO_OBJS) $(COMMON_OBJS) $(CONNECTION_OBJS)
EXECUTABLE_OBJS     :=

# Link the template to avoid redundancy
include $(MAKEFILE_TEMPLATE)
	
# Need to specify test cases explicitly because they have variables in recipe
test-paxos: $(PAXOS_TESTS)
	@for a in $(PAXOS_TESTS); do \
		echo == $$a ==; \
		$(LDLIBRARYPATH) $$a; \
	done
