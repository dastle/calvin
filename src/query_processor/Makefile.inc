OBJDIRS += query_processor

# Create template specific variables
UPPERC_DIR := QUERY_PROCESSOR
LOWERC_DIR := query_processor

QUERY_PROCESSOR_SRCS   :=  query_processor/query_processor.cc query_processor/catalog_manager.cc query_processor/key_format.cc
QUERY_PROCESSOR_EXES    :=

SRC_LINKED_OBJS     := $(QUERY_PROCESSOR_AST_OBJS)
TEST_LINKED_OBJS    := $(COMMON_OBJS) $(PROTO_OBJS) \
		       $(QUERY_PROCESSOR_FLEX_BISON_OBJS) \
		       $(QUERY_PROCESSOR_AST_OBJS) $(BACKEND_OBJS) \
		       $(CONNECTION_OBJS)
EXECUTABLE_OBJS     :=

# Link the template to avoid redundancy
include $(MAKEFILE_TEMPLATE)
	
# Need to specify test cases explicitly because they have variables in recipe
test-query_processor: $(QUERY_PROCESSOR_TESTS)
	@for a in $(QUERY_PROCESSOR_TESTS); do \
		echo == $$a ==; \
		$(LDLIBRARYPATH) $$a; \
	done
