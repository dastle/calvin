OBJDIRS += process_mgr

# Create template specific variables
UPPERC_DIR := PROCESS_MGR
LOWERC_DIR := process_mgr

PROCESS_MGR_SRCS   :=  process_mgr/process_mgr.cc
PROCESS_MGR_EXES    :=

SRC_LINKED_OBJS     :=
TEST_LINKED_OBJS    := $(CONNECTION_OBJS) $(COMMON_OBJS) $(PROTO_OBJS)
EXECUTABLE_OBJS     :=

# Link the template to avoid redundancy
include $(MAKEFILE_TEMPLATE)
	
# Need to specify test cases explicitly because they have variables in recipe
test-process_mgr: $(PROCESS_MGR_TESTS)
	@for a in $(PROCESS_MGR_TESTS); do \
		echo == $$a ==; \
		$(LDLIBRARYPATH) $$a; \
	done
