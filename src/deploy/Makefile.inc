OBJDIRS += deploy

# Create template specific variables
UPPERC_DIR := DEPLOY
LOWERC_DIR := deploy

DEPLOY_SRCS         := deploy/calvin_instance.cc
DEPLOY_EXES  :=

SRC_LINKED_OBJS     := $(STORED_PROCEDURES_OBJS) $(SEQUENCER_OBJS) $(BACKEND_OBJS)
TEST_LINKED_OBJS    := $(STORED_PROCEDURES_OBJS) $(SEQUENCER_OBJS) $(BACKEND_OBJS)
EXECUTABLE_OBJS     := $(STORED_PROCEDURES_OBJS) $(SEQUENCER_OBJS) $(BACKEND_OBJS)

# Link the template to avoid redundancy
include $(MAKEFILE_TEMPLATE)
	
# Need to specify test cases explicitly because they have variables in recipe
test-deploy: $(DEPLOY_TESTS)
	@for a in $(DEPLOY_TESTS); do \
		echo == $$a ==; \
		$(LDLIBRARYPATH) $$a; \
	done
