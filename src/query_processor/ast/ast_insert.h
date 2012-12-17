/// @file
/// @author Anton Petrov <anton.petrov@yale.edu>
/// @author Michael Giuffrida <michael.giuffrida@yale.edu>
/// @version 0.1
/// @since 04 Apr 2012
///
/// @section DESCRIPTION
///
/// Interface to the ASTInsert node and all relevant sub-nodes. Please see the
/// ast.h file for more information on the structure of the AST.

#ifndef _DB_QUERY_PROCESSOR_AST_AST_INSERT_H_
#define _DB_QUERY_PROCESSOR_AST_AST_INSERT_H_

#include <vector>
#include "query_processor/ast/ast.h"

using std::vector;

namespace calvin {

struct ASTKey;
struct ASTColList;
struct ASTValList;
struct ASTTupleVals;

/// @struct ASTInsert
/// @brief Representation of a SQL INSERT statement. The struct contains a
/// number of fields that reference sub-nodes and that are part of the UPDATE
/// statement syntax as specified in the BNF grammar.
/// @TODO The current ASTInsert node only supports inserts into a single
/// table. The grammar provides two more variations of the INSERT statement:
/// one using the SET keyword instead of the VALUES keyword and the other
/// enabling the insertion of tuples that were fetched using a SELECT
/// statement. The grammar needs to be adapted accordingly so that only a
/// the first type of INSERT statement is matched.
struct ASTInsert : ASTRoot {
  /// Constructor that creates a new instance of the ASTInsert object. This
  /// constructor also calls the contructor of the base class ASTRoot so that
  /// the type of ASTRoot is set appropriately. After this the constructor
  /// initializes various fields in the struct.
  ///
  /// @param tbl_name     The name of the table into which we will insert
  ///                     tuples.
  /// @param col_names    The names of the columns into which new values
  ///                     will be inserted.
  /// @param insert_vals  A list of the tuples which will be inserted.
  ASTInsert(ASTKey* tbl_name, ASTColList* col_names, ASTValList* insert_vals);

  /// The name of the table into which we will insert the new tuples. The table
  /// name is represented using an ASTKey object.
  ASTKey* tbl_name_;

  /// A list of column names for which the tuples to be inserted will have to
  /// explicitly provide values.
  ASTColList* col_names_;

  /// A list of the tuples to be inserted.
  ASTValList* insert_vals_list_;
};

/// @struct ASTColList
/// @brief Maintain a list of column names into which we will be inserting
/// data using the SQL INSERT statement. The list of columns is kept as a
/// vector of pointers to char arrays.
struct ASTColList {
  /// Constructor that creates an ASTColList object and adds a column to the
  /// column list vector.
  ASTColList() {
    col_list_ = new vector<const char*>();
}

  /// Given an ASTColList object, add another column name to the list.
  ///
  /// @param col Column name to be added (as a string pointer).
  void add_col_list(const char* col);

  /// Vector keeping track of the column names that are part of this list.
  vector<const char*>* col_list_;
};

/// @struct ASTValList
/// @brief Collect all the tuples that will be inserted into the table. The
/// tuples are kept in a vector and each tuple is represented using an
/// ASTTupleVals object.
struct ASTValList {
  /// Constructor of an ASTValList object.
  ASTValList() {
    tuples_ = new vector<ASTTupleVals*>();
  }

  /// Add another tuple to the tuple_ vector.
  ///
  /// @param tuple The ASTTupleVals object that will be appended to the tuple_
  ///              vector.
  void add_tuples(ASTTupleVals* tuple);

  /// A collection of tuples that will be inserted in the table.
  vector<ASTTupleVals*>* tuples_;
};

/// @struct ASTTupleVals
/// @brief The ASTTupleVals object represents the values of a single tuple
/// that will be inserted during the SQL INSERT statement.
struct ASTTupleVals {
  /// Constructor used to create a new instance of the ASTTupleVals object.
  /// The constructor requires an ASTExpr object which will be added to the
  /// attribute_val_ vector.
  ASTTupleVals() {
    attributes_ = new vector<ASTExpr*>;
  }

  /// Add the next attribute value that will comprise the tuple which will be
  /// inserted.
  ///
  /// @param value An ASTExpr object that holds the value to be inserted in
  ///              the vector that represents the tuple.
  void add_attributes(ASTExpr* value);

  /// Vector used to store the values that make up the tuple to be inserted.
  /// Each value is represented as an ASTExpr node. If an entry contains a
  /// NULL pointer, then that represents the DEFAULT value of the attribute.
  /// When declaring a table and its attributes, for each attribute we will
  /// indicate the default value that it takes.
  vector<ASTExpr*>* attributes_;
};

}  // namespace calvin

#endif  // _DB_QUERY_PROCESSOR_AST_AST_INSERT_H_
