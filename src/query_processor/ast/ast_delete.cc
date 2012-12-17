/// @file
/// @author Anton Petrov <anton.petrov@yale.edu>
/// @author Michael Giuffrida <michael.giuffrida@yale.edu>
/// @version 0.1
/// @since 16 Apr 2012
///
/// @section DESCRIPTION
///
/// Implementation details of the ASTDelete node and all its relevant
/// sub-nodes.

#include "query_processor/ast/ast_delete.h"

namespace calvin {

// ----------------------------------------------------------------------------
// ASTDelete Definitions

ASTDelete::ASTDelete(ASTKey* table_name, ASTExpr* opt_where,
    ASTGroupBy* opt_order_by, ASTExprOperand* limit)
  : ASTRoot(DELETE), where_(opt_where), table_name_(table_name),
  opt_order_by_(opt_order_by), limit_(limit) {
}

}  // namespace calvin
