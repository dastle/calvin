OBJDIRS += backend

# Create template specific variables
UPPERC_DIR := BACKEND
LOWERC_DIR := backend

BACKEND_SRCS          := backend/storage.cc \
                         backend/storage_iterator.cc \
                         backend/leveldb_storage.cc \
                         backend/transactional_storage_manager.cc
BACKEND_EXES   :=

SRC_LINKED_OBJS     := $(PROTO_OBJS)
TEST_LINKED_OBJS    := $(PROTO_OBJS) $(COMMON_OBJS) $(CONNECTION_OBJS)
EXECUTABLE_OBJS     :=

# Link the template to avoid redundancy
include $(MAKEFILE_TEMPLATE)
	
# Need to specify test cases explicitly because they have variables in recipe
test-backend: $(BACKEND_TESTS)
	@for a in $(BACKEND_TESTS); do \
		echo == $$a ==; \
		$(LDLIBRARYPATH) $$a; \
	done
