OBJDIRS += checkpointing

# Create template specific variables
UPPERC_DIR := CHECKPOINTING
LOWERC_DIR := checkpointing

CHECKPOINTING_SRCS          := checkpointing/hobbes.cc \
                               checkpointing/ping_pong.cc \
                               checkpointing/ping_pong_utilities.cc \
                               checkpointing/naive.cc
CHECKPOINTING_EXES   :=

SRC_LINKED_OBJS     :=
TEST_LINKED_OBJS    := $(BACKEND_OBJS) $(PROTO_OBJS) $(COMMON_OBJS) $(CONNECTION_OBJS) \
                       $(STORED_PROCEDURES_OBJS)
EXECUTABLE_OBJS     :=

# Link the template to avoid redundancy
include $(MAKEFILE_TEMPLATE)
	
# Need to specify test cases explicitly because they have variables in recipe
test-checkpointing: $(CHECKPOINTING_TESTS)
	@for a in $(CHECKPOINTING_TESTS); do \
		echo == $$a ==; \
		$(LDLIBRARYPATH) $$a; \
	done
