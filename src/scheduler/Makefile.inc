OBJDIRS += scheduler

# Create template specific variables
UPPERC_DIR := SCHEDULER
LOWERC_DIR := scheduler

SCHEDULER_SRCS        := scheduler/lock_manager.cc \
                         scheduler/scheduler.cc
SCHEDULER_EXES :=

SRC_LINKED_OBJS     := $(PYTHON_OBJS)
TEST_LINKED_OBJS    := $(PROTO_OBJS) $(COMMON_OBJS) $(CONNECTION_OBJS) $(BACKEND_OBJS) \
                       $(APPLICATION_OBJS) $(SEQUENCER_OBJS)  $(PROCESS_MGR_OBJS) $(PYTHON_OBJS) \
                       $(STORED_PROCEDURES_OBJS)
EXECUTABLE_OBJS     :=

# Link the template to avoid redundancy
include $(MAKEFILE_TEMPLATE)

# Need to specify test cases explicitly because they have variables in recipe
test-scheduler: $(SCHEDULER_TESTS)
	@for a in $(SCHEDULER_TESTS); do \
		echo == $$a ==; \
		$(LDLIBRARYPATH) $$a; \
	done
