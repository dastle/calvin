OBJDIRS += query_processor/flex_bison

# Create template specific variables
UPPERC_DIR := QUERY_PROCESSOR_FLEX_BISON
LOWERC_DIR := query_processor/flex_bison

QUERY_PROCESSOR_FLEX_BISON_SRCS	  := $(LOWERC_DIR)/sqlparser.tab.cpp \
                                     $(LOWERC_DIR)/sqlscanner.yy.cpp \
				     $(LOWERC_DIR)/sqldriver.cc
QUERY_PROCESSOR_FLEX_BISON_EXES   := $(LOWERC_DIR)/test_parser.cc

SRC_LINKED_OBJS     :=
TEST_LINKED_OBJS    :=
EXECUTABLE_OBJS     := $(QUERY_PROCESSOR_AST_OBJS)

# Link the template to avoid redundancy
include $(MAKEFILE_TEMPLATE)

####################################################################
# Note: ANTON and MICHAEL
#     Anytime that a person wants to include the lex and yacc functions the
#     final binary that is produced needs to have them linked as follows:
#
#   For a test:
#     TEST_LINKED_OBJS    := $(QUERY_PROCESSOR_FLEX_BISON_OBJS)
#
#   For a binary (i.e., calvin_ctl):
#     EXECUTABLE_OBJS     := $(QUERY_PROCESSOR_FLEX_BISON_OBJS)
####################################################################

# Compile flex and bison files into objects and headers
$(OBJDIR)/$(LOWERC_DIR)/sqlparser.tab.cpp \
$(OBJDIR)/$(LOWERC_DIR)/sqlparser.tab.hpp: $(LOWERC_DIR)/sqlparser.ypp
	@echo + bison $<
	@mkdir -p $(@D)
	$(V)$(LDLIBRARYPATH) $(BISONEXE) -o $@ -d $^
$(OBJDIR)/$(LOWERC_DIR)/sqlscanner.yy.cpp: $(LOWERC_DIR)/sqlscanner.l
	@echo + flex $<
	@mkdir -p $(@D)
	$(V)$(LDLIBRARYPATH) $(FLEXEXE) -o $@ $^

