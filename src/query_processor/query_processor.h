/// @file
/// @author Anton Petrov <anton.petrov@yale.edu>
/// @author Michael Giuffrida <michael.giuffrida@yale.edu>
/// @version 0.1
/// @since 04 Apr 2012
///
/// @section DESCRIPTION
///
/// The QueryProcessor class is, among other things, responsible for processing
/// query plans and converting them from SQL into a set of CRUD actions.  This
/// is a non-instantiable class.
///
/// Thad's advice on how to do query processing, as conveyed to Alex:
///  1) Does the table exist in my catalog manager?
///       If yes, go on
///  2) Do all the attributes in the SQL query for the table(s) specified exist?
///       If yes, go on
///  3) Do we have an index on any?
///       If yes, perform cost analysis
///       If no, do range scan over entire table and compare predicates
///

#ifndef _DB_QUERY_PROCESSOR_QUERY_PROCESSOR_H_
#define _DB_QUERY_PROCESSOR_QUERY_PROCESSOR_H_

#include <map>
#include <string>
#include "query_processor/flex_bison/sqldriver.h"
#include "query_processor/catalog_manager.h"
#include "query_processor/operator.h"
#include "proto/txn.pb.h"
#include "common/types.h"
#include "backend/storage.h"

namespace calvin {

using std::string;
/// @class  QueryProcessor
/// @brief  The QueryProcessor is responsible for executing SQL queries
class QueryProcessor {
 public:
  /// Parse ande execute a SQL query, outputting the results to the log.
  ///
  /// @param      sql_txn    A transaction with the SQL query to execute
  ///
  /// @returns    True if the query was sucessufully parsed and executed
  static bool ProcessQuery(TxnProto* sql_txn);

 private:
  /// @struct Table
  /// @brief  Internal representation of a collection of tuples used
  /// during execution of operator tree
  struct Table {
    /// The attributes in this projection
    map<string, CatalogManager::Attribute> attributes_;

    /// Map of packed tuple values by key
    map<Key, const char*> values_;

    /// The name of the table
    string table_;
  };

  /// Constructor is private and undefined to prevent instantiation
  QueryProcessor();

  /// Execute an operator tree
  ///
  /// @param      root    Root of an operator tree
  ///
  /// @returns    Table of tuples found
  static Table* ExecuteQuery(Operator* root);

  /// Select tuples from a table
  ///
  /// @param      expr    Expression to select on
  /// @param      table   Table to select from
  ///
  /// @returns    Table of tuples that match the selection predicate
  static Table* ProcessSelect(ASTExpr* expr, Table* table);

  /// Flatten an expression into a single expression operand
  ///
  /// @param      expr         ASTExpr object to process
  /// @param      tuple        Tuple expression is operating against
  /// @param      attributes   List of relevant attributes
  ///
  /// @returns    An operand, or NULL if not possible
  static ASTExprOperand* ProcessExpr(ASTExpr* expr, const char* tuple,
    const map<string, CatalogManager::Attribute>& attributes);

  /// Instance of the catalog manager
  static CatalogManager* catalog_;

  /// Instance of the storage manager
  static Storage* storage_;
};

/// @class Tuple
/// @brief A value array and methods to get and set the attributes within
class Tuple {
 public:
  /// Create a tuple, allocating an array of the appropriate size.
  ///
  /// @param      size    Size of value array to create
  explicit Tuple(size_t size);

  /// Create a tuple with the given value
  ///
  /// @param      value   Value array representing tuple
  /// @param      size    Size of value array
  Tuple(char* value, size_t size);

  /// Return an integer value from the tuple
  ///
  /// @param      offset  Location of value within array
  ///
  /// @returns    Integer stored in tuple
  int GetInt(size_t offset) const;

  /// Return an string value from the tuple
  ///
  /// @param      offset  Location of value within array
  /// @param      length  Maximum length of string value
  ///
  /// @returns    String stored in tuple
  const char* GetString(size_t offset) const;

  /// Static method to return an integer value from a tuple value
  ///
  /// @param      value   Tuple value to retrieve attribute from
  /// @param      offset  Location of value within array
  ///
  /// @returns    Integer stored in tuple
  static int GetInt(const char* value, size_t offset);

  /// Static method to return an string value from a tuple value
  ///
  /// @param      value   Tuple value to retrieve attribute from
  /// @param      offset  Location of value within array
  /// @param      length  Maximum length of string value
  ///
  /// @returns    String stored in tuple
  static const char* GetString(const char* value, size_t offset);

  /// Append the integer value to the tuple
  ///
  /// @param      value   Integer value to store
  void add_value(uint32 value);

  /// Append the string value to the tuple and pad or truncates to length
  ///
  /// @param      value   String value to store
  /// @param      length  Maximum size of string value
  void add_value(string value, size_t length);

  /// Get the current value array underlying the tuple
  ///
  /// @returns    A const pointer to the underlying char array
  const char* value() const;

 private:
  /// Underlying value array
  char* value_;

  /// Size of value array
  size_t size_;

  /// Offset for next entry in tuple, when adding values
  size_t offset_;
};

}  // namespace calvin

#endif  // _DB_QUERY_PROCESSOR_QUERY_PROCESSOR_H_
