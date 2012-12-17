/// @file
/// @author Anton Petrov <anton.petrov@yale.edu>
/// @author Michael Giuffrida <michael.giuffrida@yale.edu>
/// @version 0.1
/// @since 16 Apr 2012
///
/// @section DESCRIPTION
///
/// Interface to the ASTSelect node and all relevant sub-nodes. Please see the
/// ast.h file for more information on the structure of the AST.

#ifndef _DB_QUERY_PROCESSOR_AST_AST_SELECT_H_
#define _DB_QUERY_PROCESSOR_AST_AST_SELECT_H_

#include "query_processor/ast/ast.h"
#include <string>
#include <vector>

using std::string;
using std::vector;

namespace calvin {

// Some forward declarations so the developer does not need to worry about the
// order in which classes are declared. These are also needed because a
// circular dependency is introduced with the ast.h #include above.
struct ASTSelectExpr;
struct ASTTableReference;
struct ASTExprOperand;
struct ASTTableFactor;
struct ASTJoinTable;
struct ASTKey;
struct ASTAtom;
struct ASTGroupBy;
struct ASTExpr;

/// @struct ASTSelect
/// @brief Representation of a SQL SELECT statement. The struct contains
/// various fields that reference other nodes in the AST that are part of this
/// SELECT statement.
struct ASTSelect : ASTRoot {
  ASTSelect() : ASTRoot(SELECT) {
    opts_ = NULL;
    select_expr_list_ = NULL;
    table_references_ = NULL;
    where_ = NULL;
    group_by_ = NULL;
    having_ = NULL;
    order_by_ = NULL;
    limit_ = NULL;
  }

  ~ASTSelect();

  /// Pointer to the option specified (represented as a string); NULL if none.
  const char* opts_;

  /// TRUE if we select all attributes -- '*'; FALSE otherwise.
  bool select_all_;

  /// Pointer to vector of ASTSelectExpr pointers. An ASTSelectExpr object
  /// represents an attribute value we need to return when performing the
  /// projection during the SELECT statement. If selectAll == TRUE, this vector
  /// is irrelevant, since we will be fetching all the value in the table.
  ///
  /// The reason why we are using select_expr_list_ as a pointer is so that the
  /// parser does not have to worry about memory management. If we didn't use
  /// pointers here, we would have to copy data structures over and then free
  /// the old copy in the parser. By using pointer, we can free the memory
  /// after we execute the AST.
  vector<ASTSelectExpr*>* select_expr_list_;

  /// Pointer to Vvctor of ASTTableReference pointers. An ASTTableReference
  /// object represents the list of tables which we need to query in order to
  /// execute the SELECT statement.
  ///
  /// Note that the ASTTableReference object contaon ASTTableFactor objects.
  /// An ASTTableFactor can just be a table reference as specified by the
  /// key_format.h file or an arbitrary sequence of table references chained
  /// by the JOIN keyword.
  vector<ASTTableReference*>* table_references_;

  /// This is an optional field specifying whether the SELECT statement
  /// contains a WHERE clause. The WHERE clause is an ASTExpr. This field
  /// is NULL when no such clause has been specified.
  ASTExpr* where_;

  /// This is an optional field specifying whether the SELECT statement
  /// contains a GROUP BY clause. If it does, then this field points to an
  /// ASTGroupBy object which in turn contains a vector aggregating all the
  /// attribute names in the list. Field is NULL if no GROUP BY was
  /// specified.
  ASTGroupBy* group_by_;

  /// Optional field specifying a HAVING clause. This clause restricts the
  /// result only to those aggregate values that meet the specified
  /// condition. HAVING is like a WHERE clause on aggregates.
  ASTExpr* having_;

  /// Optional field specifying a list of columns by which the order the
  /// results. This option has the same structure as a GROUP BY clause, namely
  /// it accepts a list of column names and is terminated by an optional
  /// ASC or DESC keyword, indicating the sort order.
  /// Nore that GROUP BY and ORDER BY have the same syntax, which means both
  /// will match the same rule in the BNF grammar and so we use the same struct
  /// to represent both.
  ASTGroupBy* order_by_;

  /// The LIMIT keyword specifies if we should limit the reported results to
  /// the integer specified. LIMIT 150 only reports the first 150 tuples of
  /// the results to the client.
  ASTExprOperand* limit_;
};

/// @struct ASTSelectExpr
/// @brief An ASTSelectExpr specifies an attribute within the table that
/// will be projected during the execution of the SELECT statement.
struct ASTSelectExpr {
  ASTSelectExpr() : opt_as_alias_(NULL) { }

  /// Free associated memory
  ~ASTSelectExpr();

  /// Create a new ASTSelectExpr node and return a pointer to it.
  ///
  /// @param opt          The AS alias used to refer to this select expr.
  /// @param expression   The select expression: NAME | NAME '.' NAME.
  /// @returns            Returns a pointer to the newly created ASTSelectExpr
  ///                     node.
  static ASTSelectExpr* create(const char* opt, ASTExprOperand* expression);

  /// The attribute to be projected can be renamed using the AS keyword.
  /// If this option is not used, the pointer is set to NULL.
  const char* opt_as_alias_;

  /// The actual select expression. Note that this can either be a NAME
  /// or a NAME '.' NAME as specified at the beginning of the grammar.
  ASTExprOperand* expr_;
};

/// @struct ASTTableReference
/// @brief An ASTTableReference is a reference to a table in our database. The
/// table can be specified either using an ASTTableFactor object or using
/// an ASTJoinTable object. These fields are mutually exclusive, one must
/// be NULL.
struct ASTTableReference {
  ASTTableReference() {
    table_factor_ = NULL;
    opt_as_alias_ = NULL;
    join_table_ = NULL;
  }

  /// Free associated memory
  ~ASTTableReference();

  /// Create an ASTTableReference object by setting the appropriate pointers
  /// and returning a pointer to the newly created object.
  ///
  /// @param tableFactor  Pointer to an ASTTableFactor object.
  /// @param joinTable    Pointer to an ASTJoinTable object.
  /// @returns A pointer to the newly created ASTTableReference node.
  static ASTTableReference* create(ASTTableFactor* table_factor,
                                   ASTJoinTable* join_table);
  /// Pointer to an ASTTableFactor object if this is what the table reference
  /// is; NULL otherwise.
  ASTTableFactor* table_factor_;

  /// Optional AS reference. optAsAlias points to the string and is NULL
  /// if no AS reference has been specified.
  const char* opt_as_alias_;

  /// Pointer to an ASTJoinTable object if this is what the table reference
  /// is; NULL otherwise.
  ASTJoinTable* join_table_;
};

/// @struct ASTTableFactor
/// @brief The ASTTableFactor is a key in the storage layer with an optional
/// AS alias specified.
struct ASTTableFactor {
  /// Create an ASTTableFactor object.
  ///
  /// @param asAlias  String pointing to the alias name.
  /// @param keyPtr   Pointer to the ASTKey object.
  ASTTableFactor(ASTKey* key_ptr, const char* as_alias = NULL);

  /// Free associated memory
  ~ASTTableFactor();

  /// Pointer to the ASTKey node comprising this ASTTableFactor
  ASTKey* key_;

  /// Pointer to string containing alias; NULL if none was specified.
  const char* opt_as_alias_;
};

/// @struct ASTJoinTable
/// @brief ASTJoinTable is an arbitrary sequence of table names that are
/// chained using the JOIN keyword. This struct has yet to be defined.
/// @TODO Need to define the structure using the BNF grammar.
struct ASTJoinTable {
};

}  // namespace calvin

#endif  // _DB_QUERY_PROCESSOR_AST_AST_SELECT_H_

