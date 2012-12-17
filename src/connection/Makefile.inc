OBJDIRS += connection

# Create template specific variables
UPPERC_DIR := CONNECTION
LOWERC_DIR := connection

CONNECTION_SRCS   := connection/connection.cc connection/caravan.cc
CONNECTION_EXES    :=

SRC_LINKED_OBJS     :=
TEST_LINKED_OBJS    := $(COMMON_OBJS) $(PROTO_OBJS)
EXECUTABLE_OBJS     :=

# Link the template to avoid redundancy
include $(MAKEFILE_TEMPLATE)

# Need to specify test cases explicitly because they have variables in recipe
test-connection: $(CONNECTION_TESTS)
	@for a in $(CONNECTION_TESTS); do \
		echo == $$a ==; \
		$(LDLIBRARYPATH) $$a; \
	done
