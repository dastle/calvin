/// @file
/// @author Thaddeus Diamond <diamond@cs.yale.edu>
/// @version 0.1
/// @since 26 Nov 2011
///
/// @section DESCRIPTION
///
/// This file defines a general interface for a StorageDecorator (checkpoint,
/// prefetch, etc.)

#ifndef _DB_BACKEND_STORAGE_DECORATOR_H_
#define _DB_BACKEND_STORAGE_DECORATOR_H_

#include "common/types.h"

namespace calvin {

class Value;

/// @class  StorageDecorator
/// @brief  Any extenstion to Storage (i.e. Checkpoint, fetching, etc.)
///
/// The StorageDecorator class defines a set of methods that any object
/// extending functionality on the Storage must abide by.
///
/// @attention
/// This class uses the Decorator design pattern that allows chained
/// construction such as:
/// new PingPong(new Prefetcher(new LazyEvaluator()))
class StorageDecorator {
 public:
  /// The destructor for a storage decorator must free all memory
  virtual ~StorageDecorator() {}

  /// What type of StorageDecorator is you?
  virtual const char* type() = 0;

  /// If the database size is different because we intercept Put's, denote
  /// that
  ///
  /// @returns    0 for database to proceed as normal, size o/w
  virtual uint64 HandleDatabaseSize() = 0;

  /// Every StorageDecorator must be able to handle a Get request to the
  /// Storage
  ///
  /// @param      key     The key the read was performed on
  /// @param      version The version the read was performed during
  ///
  /// @returns    NULL if the Storage should continue it's execution, pointer
  ///             to value Get should return o/w
  virtual Value* HandleGet(const Key& key, VersionID version) = 0;

  /// Every StorageDecorator must be able to handle a Put request to the
  /// Storage
  ///
  /// @param      key     The key the write was performed on
  /// @param      value   The value being written
  /// @param      version The version the write was performed during
  ///
  /// @returns    True if the Storage should continue it's execution, false o/w
  virtual bool HandlePut(const Key& key, Value* value, VersionID version) = 0;

  /// Accessor method for decoration on top of each StorageDecorator
  ///
  /// @returns    A pointer to the object's current decorator
  StorageDecorator* decorator()     { return decorator_; }

 protected:
  /// Constructor called from subclasses (non-instantiable)
  explicit StorageDecorator(StorageDecorator* decorator)
    : decorator_(decorator) {}

  /// Maintain a private variable representing the decorator a la decorator DP
  StorageDecorator* decorator_;
};

}  // namespace calvin

#endif  // _DB_BACKEND_STORAGE_DECORATOR_H_
