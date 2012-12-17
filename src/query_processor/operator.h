/// @file
/// @author Anton Petrov <anton.petrov@yale.edu>
/// @author Michael Giuffrida <michael.giuffrida@yale.edu>
/// @version 0.1
/// @since 16 Apr 2012
///
/// @section DESCRIPTION
///
/// An operator tree represents a query execution plan. Each node in the tree
/// represents some operation. The QueryProcessor converts an AST into an
/// operator tree, which is then executed by the QueryProcessor.

#ifndef _DB_QUERY_PROCESSOR_OPERATOR_H_
#define _DB_QUERY_PROCESSOR_OPERATOR_H_

#include <string>
#include <vector>
#include "query_processor/ast/ast.h"

namespace calvin {

/// @enum OpType
/// @brief List of operator types currently supported
enum OpType {
  OP_PROJECT = 0,
  OP_SELECT,
  OP_TABLE,
};

/// @struct Operator
/// @brief  A node in an operator tree.
struct Operator {
  explicit Operator(OpType op) : operator_(op) { }
  virtual ~Operator() { }

  /// The type of operator
  OpType operator_;
};

/// @struct OpProject
/// Operator for projecting attributes from tuples
struct OpProject : public Operator {
  OpProject() : Operator(OP_PROJECT), next_(NULL) { }

  /// Free memory
  ~OpProject() {
    delete next_;
  }

  /// Attributes to project
  vector<string> attributes_;

  /// Operator to project from
  Operator* next_;
};

/// @struct OpSelect
/// Operator for selecting tuples
struct OpSelect : public Operator {
  OpSelect() : Operator(OP_SELECT), expr_(NULL), next_(NULL) { }

  /// Free memory
  ~OpSelect() {
    delete next_;
  }

  /// Expression to select on
  ASTExpr* expr_;

  /// Operator to select from
  Operator* next_;
};

/// @struct OpTable
/// Operator for tuple extraction from table
struct OpTable : public Operator {
  OpTable() : Operator(OP_TABLE) { }

  /// Name of table to extract tuples from
  string table_;
};

}  // namespace calvin

#endif  // _DB_QUERY_PROCESSOR_OPERATOR_H_
