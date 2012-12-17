/// @file
/// @author Anton Petrov <anton.petrov@yale.edu>
/// @author Michael Giuffrida <michael.giuffrida@yale.edu>
/// @version 0.1
/// @since 16 Apr 2012
///
/// @section DESCRIPTION
///
/// Interface to the ASTDelete node and all relevant sub-nodes. Please see the
/// ast.h file for more information on the structure of the AST.

#ifndef _DB_QUERY_PROCESSOR_AST_AST_DELETE_H_
#define _DB_QUERY_PROCESSOR_AST_AST_DELETE_H_

#include <vector>
#include "query_processor/ast/ast.h"

using std::vector;

namespace calvin {

struct ASTGroupBy;
struct ASTKey;
struct ASTExprOperand;

/// @struct ASTDelete
/// @brief Representation of a SQL DELETE statement. The struct contains
/// various fields that reference sub-nodes that are part of the DELETE
/// statement.
/// @TODO The current design of the ASTDelete node only supports deletes
/// from a single table. Towards the end of the project, if there is time
/// this specification can be expanded so that deletes from multiple tables
/// are supported.
/// @TODO This also means the grammar needs to be adapted so that only
/// delete statements referencing a single table are matched.
struct ASTDelete : ASTRoot {
  /// Construct an ASTDelete object and initialize all the relevant fields
  /// and sub-node pointers.
  ///
  /// @param table_name   Name of table from which we will delete tuples.
  /// @param opt_where    A WHERE clause; only tuples that pass this clause
  ///                     will be delete.
  /// @param opt_order_by An ORDER BY clause specifying the order in which
  //                      tuples are to be deleted.
  /// @param limit        Limit keyword specifying how many tuples to delete.
  ASTDelete(ASTKey* table_name, ASTExpr* opt_where, ASTGroupBy* opt_order_by,
            ASTExprOperand* limit);

  /// Optional WHERE clause. If this pointer is NULL, then no WHERE clause was
  /// specified and we delete all tuples from the table. If the pointer is
  /// non NULL then we valuate each tuple with respect to the expressions and
  /// we delete only those tuples that pass the predicate.
  ASTExpr* where_;

  /// Table name from which to delte tuples.
  ASTKey* table_name_;

  /// Optional ORDER BY clause specifying a number of attributes by which to
  /// order the tuples before deleting them.
  /// Note that the ORDER BY clause has the same syntac as the GROUP BY clause
  /// and so we used the same struct to represent both.
  ASTGroupBy* opt_order_by_;

  /// Optional LIMIT keyword specifying how many tuples to delete before ending
  /// the DELETE statement. For instance, a LIMIT 130 will delete the 130
  /// tuples and then stop.
  ASTExprOperand* limit_;
};

}  // namespace calvin

#endif  // _DB_QUERY_PROCESSOR_AST_AST_DELETE_H_
