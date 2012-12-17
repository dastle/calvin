OBJDIRS += stored_procedures

# Create template specific variables
UPPERC_DIR := STORED_PROCEDURES
LOWERC_DIR := stored_procedures

STORED_PROCEDURES_SRCS          := stored_procedures/application.cc \
                              stored_procedures/tpcc/new_order.cc \
                              stored_procedures/tpcc/payment.cc \
                              stored_procedures/tpcc/delivery.cc \
                              stored_procedures/tpcc/order_status.cc \
                              stored_procedures/tpcc/stock_level.cc \
                              stored_procedures/tpcc/initialization.cc \
                              stored_procedures/tpcc/tpcc.cc \
                              stored_procedures/microbenchmark/initialization.cc \
                              stored_procedures/microbenchmark/microbenchmark.cc
STORED_PROCEDURES_EXES   :=

SRC_LINKED_OBJS     := $(PROTO_OBJS)
TEST_LINKED_OBJS    := $(PROTO_OBJS) $(COMMON_OBJS) $(CONNECTION_OBJS) $(BACKEND_OBJS)
EXECUTABLE_OBJS     :=

# Link the template to avoid redundancy
include $(MAKEFILE_TEMPLATE)

# Need to specify test cases explicitly because they have variables in recipe
test-stored_procedures: $(STORED_PROCEDURES_TESTS)
	@for a in $(STORED_PROCEDURES_TESTS); do \
		echo == $$a ==; \
		$(LDLIBRARYPATH) $$a; \
	done
