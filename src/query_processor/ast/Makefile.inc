OBJDIRS += query_processor/ast

# Create template specific variables
UPPERC_DIR := QUERY_PROCESSOR_AST
LOWERC_DIR := query_processor/ast

QUERY_PROCESSOR_AST_SRCS	  := $(LOWERC_DIR)/ast.cc \
                               $(LOWERC_DIR)/ast_select.cc \
                               $(LOWERC_DIR)/ast_delete.cc \
                               $(LOWERC_DIR)/ast_insert.cc \
                               $(LOWERC_DIR)/ast_update.cc

QUERY_PROCESSOR_AST_EXES   :=

SRC_LINKED_OBJS     :=
TEST_LINKED_OBJS    :=
EXECUTABLE_OBJS     :=

# Link the template to avoid redundancy
include $(MAKEFILE_TEMPLATE)
