OBJDIRS += common

# Create template specific variables
UPPERC_DIR := COMMON
LOWERC_DIR := common

COMMON_SRCS           := common/configuration.cc \
                         common/utils.cc
COMMON_EXES    :=

SRC_LINKED_OBJS     :=
TEST_LINKED_OBJS    := $(PROTO_OBJS)
EXECUTABLE_OBJS     :=

# Link the template to avoid redundancy
include $(MAKEFILE_TEMPLATE)
	
# Need to specify test cases explicitly because they have variables in recipe
test-common: $(COMMON_TESTS)
	@for a in $(COMMON_TESTS); do \
		echo == $$a ==; \
		$(LDLIBRARYPATH) $$a; \
	done
