/// @file
/// @author Anton Petrov <anton.petrov@yale.edu>
/// @author Michael Giuffrida <michael.giuffrida@yale.edu>
/// @version 0.1
/// @since 04 Apr 2012
///
/// @section DESCRIPTION
///
/// Implementation details of the interface to the ASTInsert node and all
/// relevant sub-nodes.

#include "query_processor/ast/ast_insert.h"

namespace calvin {

// ---------------------------------------------------------------------------
// ASTInsert Definitions

// Constructor with initializer list.
ASTInsert::ASTInsert(ASTKey* tbl_name, ASTColList* col_names,
    ASTValList* insert_vals) :
  ASTRoot(INSERT), tbl_name_(tbl_name), col_names_(col_names),
  insert_vals_list_(insert_vals) {
}

// ----------------------------------------------------------------------------
// ASTColList Definitions

void ASTColList::add_col_list(const char* col) {
  col_list_->push_back(col);
}

// ----------------------------------------------------------------------------
// ASTValList Definitions

void ASTValList::add_tuples(ASTTupleVals* tuple) {
  tuples_->push_back(tuple);
}

// ----------------------------------------------------------------------------
// ASTTupleVals Definitions

void ASTTupleVals::add_attributes(ASTExpr* value) {
  attributes_->push_back(value);
}

}  // namespace calvin
