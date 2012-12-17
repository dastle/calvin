/// @file
/// @author Anton Petrov <anton.petrov@yale.edu>
/// @author Michael Giuffrida <michael.giuffrida@yale.edu>
/// @version 0.1
/// @since 16 Apr 2012
///
/// @section DESCRIPTION
///
/// Interface to the ASTUpdate node and all relevant sub-nodes. Please see the
/// ast.h file for more information on the structure of the AST.

#ifndef _DB_QUERY_PROCESSOR_AST_AST_UPDATE_H_
#define _DB_QUERY_PROCESSOR_AST_AST_UPDATE_H_

#include <vector>
#include "query_processor/ast/ast.h"

using std::vector;

namespace calvin {

struct ASTGroupBy;
struct ASTUpdateAsgnList;
struct ASTAssignment;

/// @struct ASTUpdate
/// @brief Representation of a SQL UPDATE statement. The struct contains
/// various fields that reference sub-nodes that are part of the UPDATE
/// statement.
///
/// @TODO When building up the AST in the sqlparser.ypp file note that the
/// current AST specification only supports single table UPDATE statements.
/// I think we should keep it that way and if we have time (which we most
/// definitely won't) the AST specification can easily be expanded to include
/// multi table UPDATE statement. Currently, the grammar for the UPDATE
/// statement accepts table_references; please change that to a KEY so that
/// only a single table matched the UPDATE rule.
struct ASTUpdate : ASTRoot {
  /// Good old constructor that creates a new instance of the ASTUpdate node
  /// and initializes various fields and sub-nodes that will be needed during
  /// the execution process.
  ///
  /// @param tbl_name         Name of table to be updated.
  /// @param update_asgn_list List of assignment statements for various
  ///                         columns we wish to update.
  /// @param where            Optional WHERE clause.
  /// @param order_by         Optional ORDER BY clause.
  /// @param                  Optional LIMIT clause.
  ASTUpdate(ASTKey* tbl_name, ASTUpdateAsgnList* update_asgn_list,
            ASTExpr* where, ASTGroupBy* order_by, ASTExprOperand* limit);

  /// The name of the table whose tuples will be updated.
  ASTKey* tbl_name_;

  /// The ASTUpdateAsgnList object specifies which columns in the table to
  /// update and to what each column should be updated.
  ASTUpdateAsgnList* update_asgn_list_;

  /// Optional WHERE clause. If this pointer is NULL, then all the tuples in
  /// the table will be updated. Otherwise, the WHERE clause is evaluated and
  /// only tuples that pass the predicate will be updated.
  ASTExpr* where_;

  /// Optional ORDER BY clause. If this pointer is NULL, then tuples will be
  /// updated in some arbitrary order (as they appear in the underlying
  /// storage). Otherwise, if the pointer is not NULL then the tuples in the
  /// table are sorted in the specified ordering and updated in that order.
  ASTGroupBy* order_by_;

  /// Optional LIMIT keyword. If this option is specified, this is the
  /// maximum number of tuples that will be updated.
  ASTExprOperand* limit_;
};

/// @struct ASTUpdateAsgnList
/// @brief A list of statements that specify how a given column should be
/// updated. Each such statement is represented by an ASTAssignment object
/// and all assignment statements are aggregated in the assignments_ vector.
struct ASTUpdateAsgnList {
  /// Constructor that creates an instance of the ASTUpdateAsgnList and adds
  /// the first ASTAssignment to the assignments_ vector.
  ASTUpdateAsgnList() {}

  /// Method to add another assignment statement to the existing list.
  ///
  /// @param assignment Single assignment statement to be added to the
  ///                   assignments_ list;
  void add_assignments(ASTAssignment* assignment);

  /// Vector that keeps track of all the assignment statements that will be
  /// executed during the update operation.
  vector<ASTAssignment*>* assignments_;
};

/// @struct ASTAssignment
/// @brief The ASTAssignment represents a single assignment statement that
/// will be executed during the update operation.
struct ASTAssignment {
  /// Create an instance of the ASTAssignment object initialize the two
  /// fields of the object.
  ///
  /// @param attribute  Name of the column whose value will be updated.
  /// @expr             Expression that will be evaluated and whose value
  ///                   will be assigned to the attribute named above.
  ASTAssignment(const char* attribute, ASTExpr* expr);

  /// The name of the attribute whose value will be updated.
  const char* attribute_;

  /// This is the expression that will be evaluated and then assigned to the
  /// attribute named above.
  ASTExpr* expr_;
};

}  // namespace calvin

#endif  // _DB_QUERY_PROCESSOR_AST_AST_UPDATE_H_

