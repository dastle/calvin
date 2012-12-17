/// @file
/// @author Alexander Thomson <thomson@cs.yale.edu>
/// @author Thaddeus Diamond <diamond@cs.yale.edu>
/// @version 0.1
/// @since 26 Nov 2011
///
/// @section DESCRIPTION
///
/// The Storage class provides an interface for writing and accessing data
/// objects stored by the system
///
/// @TODO(alex/thad) THERE IS A HUGE-ASS MEMORY LEAK: New strings are being
///                  allocated all over the place by atomic map.

#ifndef _DB_BACKEND_STORAGE_H_
#define _DB_BACKEND_STORAGE_H_

#include <cstring>
#include <vector>

#include "common/types.h"
#include "backend/value.h"
#include "backend/iterator.h"
#include "checkpointing/checkpointer.h"
#include "common/atomic.h"
#include "common/mutex.h"

using std::vector;

namespace calvin {

class Checkpointer;

/// @class  Storage
/// @brief  The physical data layer (with no upper abstractions)
///
/// The actual data layer is described by a Storage class that can have
/// various properties.  Algorithms used in the course of the database's
/// lifecycle should use methods of the class.
///
/// @attention
/// Storage uses the Singleton design pattern to ensure there is only one
/// instance of it ever.
class Storage {
 public:
  /// The GetStorageInstance() method returns a pointer to the sole application
  /// and, if one does not exist, creates it.
  static Storage* GetStorageInstance();

  /// The InitializeStorage() method takes a decorator (i.e. checkpointer,
  /// fetcher, some combination of multiple) and instantiates the Storage
  /// Singleton with it.
  ///
  /// @param    decorator     A pointer to the decorator with which to adorn DB
  /// @param    opts          A string representation of option flags
  ///
  /// @attention    Available opts:
  ///                 - --leveldb (initializes LevelDBStorage class)
  ///
  /// @attention    Logs a FATAL level message on failure, causing crash
  ///               If the database was already initialized, WARNING logged
  static void InitializeStorage(Checkpointer* decorator, const char* opts);

  /// The DestroyStorage() method properly disposes of the entiring existing
  /// storage layer, and then sets the instance to NULL.
  ///
  /// @returns  False if there was no database to begin with, true o/w
  static bool DestroyStorage();

  /// Return the current size of the database
  ///
  /// @returns    An unsigned int representing the number of records in DB
  virtual uint64 DatabaseSize();

  /// Check for the checkpointing mechanism in the database
  ///
  /// @returns    A pointer to the first checkpointing decorator, NULL if none
  virtual Checkpointer* GetCheckpointer();

  /// For use in debugging we provide a convenient way to check if there's a
  /// duped pointer in the MV scheme.
  ///
  /// @param      key         The key to be checked
  /// @param      value       The value pointer being matched again
  ///
  /// @returns    True if a pointer match was found, false o/w
  virtual bool DupedValue(const Key& key, Value* value);

  /// Simple way to check if a key exists (may not be the most efficient)
  ///
  /// @param      key         Key whose existence in DB we're checking for
  /// @param      version     Version with respect to which checking existence
  ///
  /// @returns    True if key existed in DB at specified version, false o/w
  virtual bool KeyExists(const Key& key, VersionID version);

  /// We want to be able to perform an entire scan, so we grant access to the
  /// first key in the keyset in sorted order
  ///
  /// @param      version     The version with respect to which we're iterating
  ///
  /// @returns    A pointer to the contents of the first key, NULL if empty map
  virtual const Key* FirstKey(VersionID version);

  /// We want to be able to perform an entire scan (backwards), so we grant
  /// access to the last key in the keyset in sorted order
  ///
  /// @param      version     The version with respect to which we're iterating
  ///
  /// @returns    A pointer to the contents of the last key, NULL if empty map
  virtual const Key* LastKey(VersionID version);

  /// Here's an accessor that gets all the keys available in the database
  /// for a given VersionID.
  ///
  /// @param      version     The version with regards to which we get the list
  ///
  /// @returns    Pointer to a vector of keys in the database at that time
  virtual vector<Key>* GetKeyList(VersionID version);

  /// Get the next key just after the one passed in
  ///
  /// @param      target      The key we're looking for
  /// @param      version     The version with respect to which we're looking
  ///
  /// @returns    The next key in sorted order in storage, NULL if end of list
  virtual const Key* GetNextKey(const Key& target, VersionID version);

  /// Get the previous key just before the one passed in
  ///
  /// @param      target      The key we're looking for
  /// @param      version     The version with respect to which we're looking
  ///
  /// @returns    The previous key in sorted order in storage, NULL if start
  ///             of list
  virtual const Key* GetPrevKey(const Key& target, VersionID version);

  /// The database needs a way to prune specific versions of a row. Note that
  /// if the version found is the only version in the DB, the DB will not
  /// allow you to prune it out.  Also, there is an option to specify a wider
  /// version of pruning, cutting out the entire list before a certain value.
  ///
  /// @param      key         The key in the database we are pruning
  /// @param      version     What we prune will be the first <= this value
  /// @param      multiple    Should multiple be pruned before the time
  ///                         If false, only exact match will be pruned
  /// @param      minimum     If this is set, nothing below this number can
  ///                         be pruned from the tree (helps w/Hobbes)
  /// @param      unsafe      If this is true, we override the failsafes in
  ///                         place (can't make row NULL)
  ///
  /// @returns    True if the node was successfully pruned, false o/w
  virtual bool PruneRow(const Key& key, VersionID version, bool multiple,
                        VersionID minimum = -1 * VERSION_ID_MAX,
                        bool unsafe = false);

  /// The Put() function serves as our mechanism for create in the database,
  /// pointing a key in the data layer to the specified pointer
  ///
  /// @param      key     The key where we want to store the new value
  /// @param      value   The pointer to the value we wish to store
  /// @param      version The version we want to write in the MVCC
  ///
  /// @returns    True if write succeeds, false if it fails for any reason
  virtual bool Put(const Key& key, Value* value, VersionID version);

  /// We implement the Read interface via the Get() method, which returns
  /// a constant reference to the data layer object.
  ///
  /// @param      key     The key to be read in the data layer
  /// @param      version The version we want to retrieve in the MVCC
  ///
  /// @returns    A mutable pointer to the val struct contained at that record
  virtual Value* Get(const Key& key, VersionID version);

  /// We implement the Delete interface via the Delete() method, which takes
  /// an arbitrary key and deletes it from the data layer.
  ///
  /// @param      key     The key of the record to be deleted
  /// @param      version The version we want to delete in the MVCC
  ///
  /// @returns    True if the deletion succeeds (or if no object is found with
  ///             the specified key), or false if it fails for any reason.
  virtual bool Delete(const Key& key, VersionID version);

  /// Prefetch() loads the object specified by 'key' into memory if it is
  /// currently stored on disk, asynchronously or otherwise
  ///
  /// @param      key         The key of the record to be loaded
  /// @param      wait_time   The amount of time estimated to retrieve the
  ///                         key (this is populated by dereferencing)
  ///
  /// @returns    True unless there was an I/O error on disk
  ///
  /// @todo Implement this!
  virtual bool Prefetch(const Key &key, double* wait_time) { return false; }

  /// Unfetch() unloads the object in memory by writing it off to disk
  /// asynchronously or otherwise
  ///
  /// @param      key         The key of the record to be unloaded
  ///
  /// @returns    True unless there was an I/O error on disk
  ///
  /// @todo Implement this!
  virtual bool Unfetch(const Key &key)          { return false; }

  /// This is a simple, stupid way to lock the entire database for a naive
  /// snapshot.
  virtual void Freeze()                         { freeze_lock_.WriteLock(); }

  /// Simple way to prevent anyone else from obtaining a freeze on the DB
  /// (i.e. stall out NS)
  virtual void PreventFreeze()                  { freeze_lock_.ReadLock(); }

  /// This is a simple, stupid way to unlock the entire database when a naive
  /// snapshot completes
  virtual void Unfreeze()                       { freeze_lock_.Unlock(); }

  /// Returns an Iterator exposing all records in storage as of the
  /// specified 'version'. The iterator begins positioned BEFORE the first
  /// record, so Next() or Advance() must be called before the iterator
  /// actually exposes any Key or Value.
  virtual Iterator* GetIterator(VersionID version);

 private:
  friend class StorageIterator;

 protected:
  /// To ensure the Singleton DP is enforced we make the constructor private
  Storage() : decorator_(NULL), database_size_(0) {}

  /// The Initialize() method can be called to initialize a set of decorators
  /// on the storage layer (i.e. checkpointing, etc.)
  explicit Storage(Checkpointer* decorator)
    : decorator_(decorator), database_size_(0) {}

  /// Storage's destructor must free all of the memory allocated for the data
  /// layer, but none of the pointers aggregated
  virtual ~Storage() {
    if (decorator_ != NULL)
      delete decorator_;
  }

  /// The sole application instance is maintained as a private static
  static Storage* storage_instance_;

  /// The decorator for the class (if it exists) is maintained privately
  Checkpointer* decorator_;

  /// We create a sentinel for whether the database is frozen or not, allowing
  /// us to determine whether a checkpoint is in place/to proceed
  MutexRW freeze_lock_;

  /// @struct Row
  /// @brief  Defines a single row of a multi-versioned DB table
  ///
  /// We create a private struct for use internally in the DBMS backend so that
  /// we can quickly traverse multiple versions.  The rows are maintained as
  /// a doubly linked list.
  typedef struct Row {
    Value* value;
    VersionID version;
    Row* prev;
    Row* next;
  } Row;

  /// Represent how big the data layer is locally
  Atomic<uint64> database_size_;

  /// We represent the data layer as a simple AtomicMap (see utils/atomic.h)
  AtomicMap<Key, Row*> objects_;
};

}  // namespace calvin

#endif  // _DB_BACKEND_STORAGE_H_

