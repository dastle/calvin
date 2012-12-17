/// @file
/// @author Alex Thomson <thomson@cs.yale.edu>
/// @version 0.1
/// @since 5 April 2012
///
/// @section DESCRIPTION
///
///
/// Introduction to the data model
/// ------------------------------
///
/// We are using a hierarchical data model like Bigtable's. For example, TPC-C
/// represents a number of Warehouses. (In this discussion, capitalizing words
/// like Warehouse/District/Customer means I'm referring to the abstract object
/// being represented. Keys, table names, etc. will be uncapitalized.)
/// Example Warehouse key:
///
///   warehouse(1)
///
/// Each Warehouse has 100000 Stocks. Example Stock key:
///
///   warehouse(1).stock(1337)
///
/// Each Warehouse has 10 Districts. Example District key:
///
///   warehouse(1).district(3)
///
/// Note that this is NOT the same District object as
///
///   warehouse(2).district(3)
///
/// Each District has 3000 Customers. Example Customer key:
///
///   warehouse(1).district(3).customer(91)
///
/// In a relational model, Warehouses, Districts and Customers would live in
/// three separate tables:
///
///   warehouse [ primary key = warehouse_id ]
///   district  [ primary key = (warehouse_id, district_id) ]
///   customer  [ primary key = (warehouse_id, district_id, customer_id) ]
///
/// To look up the above Customer in the relational model you might write:
///   SELECT * FROM customer WHERE warehouse_id = 1 AND
///                                district_id = 3 AND
///                                customer_id = 91
///
/// But since our model is hierarchical and data isn't necessarily stored in
/// separate tables, the primary defining characteristic of Customers in our
/// system is the sequence of 'atoms' that appears in all Customer keys:
///
///   warehouse, district, customer
///
/// Atoms are human-readable annotations of nodes in the hierarchy:
///
///       Warehouse
///        /     \                                                             -
///     Stock  District
///               |
///            Customer
///
/// Note that using this data model doesn't constrain you to actually USING the
/// hierarchical key-space. You could always declare district and customer as
/// top-level atoms, and have keys like:
///
///   warehouse(1)
///   district(1,3)
///   customer(1,3,91)
///
/// But note that we don't support true composite keys, so '1,3' and  '1,3,91'
/// in the second and third keys above would have to be strings. Alternatively,
/// you could manually do some bit-level manipulations to divide each 64-bit
/// ids into multiple keys. For example:
///
///   customer((1<<32) + (3<<16) + (91<<0))
///
/// Whatever. It's ugly. Don't do it. If you want composite keys, just do it
/// the hierarchical way plz kthks.
///
/// Labels, tables and secondary indexes
/// ------------------------------------
///
/// A sequence of one or more atoms put together represents a 'label'
/// which in the examples we've seen so far, is the equivalent of a table name.
///
/// Labels can also be used to identify secondary indexes. For example, suppose
/// we have a secondary index on 'warehouse.district.customer.name'. Then any
/// time we insert a new Customer:
///
///   INSERT
///     key:   'warehouse(1).district(3).customer(92)'
///     value: 'name=diamond;age=22;notes=thad is smart and kind of tall'
///
/// We also insert a new index entry:
///
///   INSERT
///     key:   'index.name('diamond').warehouse(1).district(3).customer(92)'
///     value: ''
///
/// The index entry doesn't need to store a value. The existence of the key
/// alone is sufficient to tell us everything we need to know.
///
/// Note that this data model also inherently supports columnar layouts. For
/// example, if we wanted to store a field (or subset of fields) separately
/// from the main blob, we could break the first INSERT above into two inserts:
///
///   INSERT
///     key:   'warehouse(1).district(3).customer(92)'
///     value: 'name=diamond;age=22'                     // notes field omitted
///
///   INSERT
///     key:   'warehouse(1).district(3).customer(92).notes'
///     value: 'thad is smart and kind of tall'
///
/// When/why is this useful?
///   (a) variable-length fields can be stored separately from the nice
///       densely packed stuff
///   (b) fast scans of particular columns (or of everything BUT a particular
///       column, as in this case)
///
///
/// Key formats
/// -----------
///
/// Keys exist in two formats. In the human readable form we've been using thus
/// far in the discussion, they take the form:
///
///   \<key\>         ::= \<clause-list\>
///
///   \<clause-list\> ::= \<clause\>
///                   | \<clause-list\> . \<clause\>
///
///   \<clause\>      ::= atom
///                   | atom ( \<id\> )
///
///   \<id\>          ::= uint64
///                   | string
///
/// Note that atoms may or may not be followed by an id. This depends on the
/// atom. All atoms are stored in a global AtomTable; the AtomTable stores
/// annotations that determine whether the atom is followed by a uint64, a
/// string, or nothing when it appears in a key.
///
/// In order to (a) store smaller keys and (b) assure that records sort
/// correctly, keys are NOT stored in their human-readable form. Instead, all
/// atoms are appear in order, followed by all ids. For example, the human-
/// readable key
///
///   index.name('diamond').warehouse(1).district(3).customer(92)
///
/// would be stored as
///
///   index.name.warehouse.district.customer $ ('diamond')(1)(3)(92)
///
/// where '$' denotes a special divider character that is distinct from the
/// first character of any atom.
///
/// In addition, atoms and uint64-type ids are not written out verbatim to key
/// strings. Instead, atoms are stored using a dictionary encoding (atom codes
/// are uint64s stored in the AtomTable). So if the dictionary encoding were
///
///   warehouse   ->  2
///   district    ->  1
///   customer    ->  1
///   name        ->  1
///   index       ->  1
///
/// then the above key would be actually appear as
///
///   1.1.2.1.1 $ ('diamond')(1)(3)(92)
///
/// Note here that multiple atoms encode to the SAME atom-code! Why is this
/// okay? Because when decoding atoms, we know that certain sequences are
/// possible and others are impossible based on the catalog manager. In our
/// example, only following tables have been declared:
///
///   human-readable                              encoded
///   --------------                              -------
///   warehouse                                   2
///   warehouse.district                          2.1
///   warehouse.district.customer                 2.1.1
///   index.name.warehouse.district.customer      1.1.2.1.1
///
/// Only 'warehouse' (=2) and 'index' (=1) can appear as the first atom in a
/// key, so they must be distinct. 'district' can only follow 'warehouse',
/// 'customer' can only follow 'district', 'name' can only follow 'index', and
/// 'index' must be the first atom in the string, so they do not need to be
/// distinct. [Note: This is a fancy-ass optimization and will not be
/// implemented in the first pass. For now, all atom codes must be unique.]
///
/// Furthermore, each uint64 above is CVarint-encoded into the string, and
/// strings are null-terminated. Spaces, dots and parens shown above are
/// omitted.
///
/// Each uint64 encodes to a single byte if its value is less than 128, and the
/// special '$' symbol takes 1 byte to encode, so
///
///   warehouse(1).district(3).customer(92)
///
/// encodes to 7 bytes, and
///
///   index.name('diamond').warehouse(1).district(3).customer(92)
///
/// encodes to 17 bytes.

#ifndef _DB_QUERY_PROCESSOR_KEY_FORMAT_H_
#define _DB_QUERY_PROCESSOR_KEY_FORMAT_H_

#include <map>
#include <string>

#include "common/types.h"

using std::map;
using std::string;

namespace calvin {

/// @class AtomTable
/// @brief Converts between human-readable and coded keys
class AtomTable {
 public:
  /// Identifiers for atom types.
  enum IDType {
    NONE = 0,    // Atom is not followed by an id.
    UINT64 = 1,  // Atom is followed by a uint64.
    STRING = 2,  // Atom is followed by a string.
  };

  /// Each atom is always followed by the same ID type---either nothing, a
  /// uint64, or a (null-terminated) string.
  void AddAtom(const string& hr, uint64 code, IDType id_type);

  /// Returns the coded version of the human-readable key 'hr'.
  string HRToCoded(const string& hr);

  /// Returns the human-readable version of the coded key 'coded'.
  string CodedToHR(const string& coded);

 private:
  /// @TODO(alex): Use faster data structures for these tables.

  /// Table for converting human-readable string to atom code.
  map<string, uint64> hr_to_code_;

  /// Table for converting atom code to human-readable string.
  map<uint64, string> code_to_hr_;

  /// Table for looking up atom id type from atom code.
  map<uint64, IDType> code_to_id_type_;
};

}  // namespace calvin

#endif  // _DB_QUERY_PROCESSOR_KEY_FORMAT_H_

