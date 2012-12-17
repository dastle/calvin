/// @file
/// @author Anton Petrov <anton.petrov@yale.edu>
/// @author Michael Giuffrida <michael.giuffrida@yale.edu>
/// @version 0.1
/// @since 16 Apr 2012
///
/// @section DESCRIPTION
///
/// Implementation details for the methods and structs defined in the
/// ast_update.h file.

#include "query_processor/ast/ast_update.h"

namespace calvin {

// ----------------------------------------------------------------------------
// ASTUpdate Definitions

ASTUpdate::ASTUpdate(ASTKey* tbl_name, ASTUpdateAsgnList* update_asgn_list,
    ASTExpr* where, ASTGroupBy* order_by, ASTExprOperand* limit)
  : ASTRoot(UPDATE), tbl_name_(tbl_name), update_asgn_list_(update_asgn_list),
  where_(where), order_by_(order_by), limit_(limit) {
}

// ----------------------------------------------------------------------------
// ASTUpdateAsgnList Definitions

void ASTUpdateAsgnList::add_assignments(ASTAssignment* assignment) {
  assignments_->push_back(assignment);
}

// ----------------------------------------------------------------------------
// ASTAssignment Definitions

ASTAssignment::ASTAssignment(const char* attribute, ASTExpr* expr)
  : attribute_(attribute), expr_(expr) {
}

}  // namespace calvin
