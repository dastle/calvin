OBJDIRS += bin

# Create template specific variables
UPPERC_DIR := BIN
LOWERC_DIR := bin

BIN_SRCS         :=
BIN_EXES         := bin/csql/csql.cc \
                    bin/calvin_ctl/calvinmaster.cc \
                    bin/calvin_ctl/calvin_ctl.cc
BIN_SCRIPTS      := bin/calvin_ctl/calvin_cluster.py

SRC_LINKED_OBJS     := $(STORED_PROCEDURES_OBJS) $(SEQUENCER_OBJS) $(BACKEND_OBJS) $(CHECKPOINTING_OBJS)
TEST_LINKED_OBJS    :=
EXECUTABLE_OBJS     := $(COMMON_OBJS) $(DEPLOY_OBJS) $(PROCESS_MGR_OBJS) \
                       $(CONNECTION_OBJS) $(PROTO_OBJS) $(SCHEDULER_OBJS) \
                       $(STORED_PROCEDURES_OBJS) $(SEQUENCER_OBJS) $(BACKEND_OBJS) \
                       $(INCLUDE_OBJS) $(PYTHON_OBJS) $(CHECKPOINTING_OBJS)

# Link the template to avoid redundancy
include $(MAKEFILE_TEMPLATE)
	
# Need to specify test cases explicitly because they have variables in recipe
test-bin: $(BIN_TESTS)
	@for a in $(BIN_TESTS); do \
		echo == $$a ==; \
		$(LDLIBRARYPATH) $$a; \
	done
