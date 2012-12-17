# Strip out this makefile since it's an API
OBJDIRS += include

# Create template specific variables
UPPERC_DIR := INCLUDE
LOWERC_DIR := include

INCLUDE_SRCS   :=  include/calvin.cc
SRC_LINKED_OBJS     :=

# Link the template to avoid redundancy
include $(MAKEFILE_TEMPLATE)
