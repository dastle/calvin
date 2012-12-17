/// @file
/// @author Anton Petrov <anton.petrov@yale.edu>
/// @author Michael Giuffrida <michael.giuffrida@yale.edu>
/// @version 0.1
/// @since 04 Apr 2012
///
/// @section DESCRIPTION
///
/// Interface to the CatalogManager, which maintains a list of table schemas

#ifndef _DB_QUERY_PROCESSOR_CATALOG_MANAGER_H_
#define _DB_QUERY_PROCESSOR_CATALOG_MANAGER_H_

#include <map>
#include <string>
#include "query_processor/key_format.h"

using std::map;
using std::string;

namespace calvin {

/// @class CatalogManager
/// @brief The CatalogManager maintains the list of tables and their attributes
///
/// @attention  The CatalogManager class follows the Singleton DP
class CatalogManager {
 public:
  /// @enum  AttrType
  /// @brief Type of attribute
  enum AttrType {
    INT = 0,
    STRING,
    INVALID,
  };

  /// @struct Attribute
  /// @brief  An attribute in a table
  struct Attribute {
    /// The size of the attribute's value, in bytes
    size_t size_;

    /// The position of the attribute's value in the storage value
    size_t offset_;

    /// The type of the attribute
    AttrType type_;

    /// Whether an index exists on the attribute
    bool index_available_;
  };

  /// Return the instance, instantiating it if necessary
  ///
  /// @returns    The sole instance of the CatalogManager
  static CatalogManager* GetCatalogManagerInstance();

  /// Checks whether the table exists in the catalog
  ///
  /// @param      table    The name of the table
  ///
  /// @returns    True if the table was found, false otherwise
  bool TableExists(const string& table) const;


  /// Checks whether the attribute exists in the given table in the catalog
  ///
  /// @param      table       The name of a table in the catalog
  /// @param      attribute   The name of the attribute in the table
  ///
  /// @returns    True if the attribute was found in the table, false otherwise
  bool AttributeExists(const string& table, const string& attribute) const;

  /// Gets the size of an attribute in the given table
  ///
  /// @param      table       The name of a table in the catalog
  /// @param      attribute   The name of the attribute in the table
  ///
  /// @returns    The size of the attribute's value, in bytes
  size_t AttributeSize(const string& table, const string& attribute) const;

  /// Gets the offset of an attribute in the given table
  ///
  /// @param      table       The name of a table in the catalog
  /// @param      attribute   The name of the attribute in the table
  ///
  /// @returns    The offset of the attribute's value, in bytes
  size_t AttributeOffset(const string& table, const string& attribute) const;

  /// Gets the type of an attribute in the given table
  ///
  /// @param      table       The name of a table in the catalog
  /// @param      attribute   The name of the attribute in the table
  ///
  /// @returns    The type of the attribute's value
  AttrType AttributeType(const string& table, const string& attribute) const;

  /// Checks whether an index exists over the table on the given attribute
  ///
  /// @param      table       The name of a table in the catalog
  /// @param      attribute   The name of the attribute in the table
  ///
  /// @returns    True if an index was found on that attribute, false otherwise
  bool IndexExists(const string& table, const string& attribute) const;

  /// Gets a list of the table's attributes
  ///
  /// @returns    A map from attribute names to Attributes
  const map<string, Attribute>* Attributes(const string& table) const;

  /// Gets the size of a tuple value in the given table
  ///
  /// @param      table       The name of a table in the catalog
  ///
  /// @returns    The tuple size for the table
  size_t TupleSize(const string& table) const;

  /// Adds a table to the catalog
  ///
  /// @param      name        The name of the table to add
  /// @param      schema      A string defining the schema, given by
  ///                         semicolon-separated attribute:type:size pairs.
  ///                         An index is specified with a comma and "index",
  ///                         e.g., "id,index:int:10;name:string:64;".
  ///
  /// @returns    True if the table was added successfully, false otherwise
  bool AddTable(const string& name, const string& schema);

  /// Removes a table from the catalog
  ///
  /// @param      name        The name of the table to delete
  ///
  /// @returns    True if the table was deleted, false otherwise
  bool DeleteTable(const string& name);

  /// Access the global AtomTable instance
  ///
  /// @returns    A reference to the global AtomTable instance
  AtomTable& at();

 private:
  /// To ensure the Singleton DP is enforced we make the constructor private
  CatalogManager() {}

  /// The CatalogManager destructor must free all aggregated memory
  virtual ~CatalogManager();

  /// The sole application instance is maintained as a private static
  static CatalogManager* catalog_instance_;

  /// Global AtomTable instance
  AtomTable at_;

  /// @class  Table
  /// @brief  A table with its attributes
  class Table {
   public:
    Table() {}

    /// Initializes the attribute list with the given schema.
    ///
    /// @param      schema    A string giving the table structure
    ///
    /// @returns    True, unless the schema could not be parsed
    bool SetSchema(const string& schema);

    /// Checks whether the attribute exists in the table
    ///
    /// @param      attribute   The name of the attribute in the table
    ///
    /// @returns    True if the attribute was found, false otherwise
    bool AttributeExists(const string& attribute) const;

    /// Gets the size of the attribute
    ///
    /// @returns    The size of the attribute's value, in bytes
    size_t AttributeSize(const string& attribute) const;

    /// Gets the offset of the attribute
    ///
    /// @returns    The offset of the attribute's value, in bytes
    size_t AttributeOffset(const string& attribute) const;

    /// Gets the type of the attribute
    ///
    /// @returns    The type of the attribute's value
    AttrType AttributeType(const string& attribute) const;

    /// Checks whether an index exists over the table on the given attribute
    ///
    /// @param      attribute   The name of the attribute in the table
    ///
    /// @returns    True if an index was found on the attribute, false o/w
    bool IndexExists(const string& attribute) const;

    /// Get a list of the table's attributes
    ///
    /// @returns    A map from attribute names to Attributes
    const map<string, Attribute>* Attributes() const;

    /// Returns the total size of a tuple in the table
    ///
    /// @returns    Size of a stored tuple
    size_t Size() const;

   private:
    /// A map of the table's attributes
    map<string, Attribute> attributes_;

    /// The size of a stored tuple
    size_t size_;
  };

  /// A map of the tables in the catalog
  map<string, Table*> tables_;
};

}  // namespace calvin

#endif  // _DB_QUERY_PROCESSOR_CATALOG_MANAGER_H_
