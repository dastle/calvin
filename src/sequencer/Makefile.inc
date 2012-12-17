OBJDIRS += sequencer

# Create template specific variables
UPPERC_DIR := SEQUENCER
LOWERC_DIR := sequencer

SEQUENCER_SRCS        := sequencer/sequencer.cc
SEQUENCER_EXES :=

SRC_LINKED_OBJS     := $(STORED_PROCEDURES_OBJS) $(PROTO_OBJS) $(COMMON_OBJS) $(CONNECTION_OBJS) $(PROCESS_MGR_OBJS) $(BACKEND_OBJS)
TEST_LINKED_OBJS    := $(STORED_PROCEDURES_OBJS) $(PROTO_OBJS) $(COMMON_OBJS) $(CONNECTION_OBJS) $(PROCESS_MGR_OBJS) $(BACKEND_OBJS)
EXECUTABLE_OBJS     := $(STORED_PROCEDURES_OBJS) $(PROTO_OBJS) $(COMMON_OBJS) $(CONNECTION_OBJS) $(PROCESS_MGR_OBJS) $(BACKEND_OBJS)

# Link the template to avoid redundancy
include $(MAKEFILE_TEMPLATE)

# Need to specify test cases explicitly because they have variables in recipe
test-sequencer: $(SEQUENCER_TESTS)
	@for a in $(SEQUENCER_TESTS); do \
		echo == $$a ==; \
		$(LDLIBRARYPATH) $$a; \
	done
