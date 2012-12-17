/// @file
/// @author Alexander Thomson <thomson@cs.yale.edu>
/// @author Thaddeus Diamond <diamond@cs.yale.edu>
/// @version 0.1
/// @since 26 Nov 2011
///
/// @section DESCRIPTION
///
/// A wrapper for a storage layer that can be used by an Application to simplify
/// application code by hiding all inter-node communication logic.

#ifndef _DB_BACKEND_TRANSACTIONAL_STORAGE_MANAGER_H_
#define _DB_BACKEND_TRANSACTIONAL_STORAGE_MANAGER_H_

#include <ucontext.h>
#include <tr1/unordered_map>
#include <vector>

#include "backend/storage.h"
#include "common/configuration.h"
#include "connection/connection.h"
#include "common/types.h"
#include "common/utils.h"
#include "proto/txn.pb.h"
#include "proto/message.pb.h"

using std::vector;
using std::tr1::unordered_map;

namespace calvin {

/// @todo Alex, can you clarify why DeterministicScheduler is a friend class?
/// @todo Alex, objects_ should somehow be templated types... maybe we should
///       template the entire TransactionalStorageManager class?
/// @todo Alex, please document remote_reads_

/// @class  TransactionalStorageManager
/// @brief  Wrapper for a CRUD interface to the database layer
///
/// By using this class as primary interface for stored procedures to interact
/// w/storage of actual data objects, stored procedures can be written without
/// paying any attention to partitioning at all.
///
/// TransactionalStorageManager use:
///  - Each transaction execution creates a new TransactionalStorageManager and
///    deletes it upon completion.
///  - No GetMutable<T*> call takes as an argument any value that depends on the
///    result of a previous GetMutable<T*> call.
///  - In any transaction execution, a call to DoneReading must follow ALL calls
///    to GetMutable<T*> and must precede BOTH (a) any actual interaction with
///    the values 'read' by earlier calls to GetMutable<T*> and (b) any calls to
///    PutObject or DeleteObject.
class TransactionalStorageManager {
 public:
  /// The constructor for a TransactionalStorageManager takes a configuration,
  /// a network connection, the actual data layer, and the transaction it is
  /// wrapping so the application can practice "super-blindness" when calling
  /// CRUD interface
  TransactionalStorageManager(Connection* connection, TxnProto* txn);

  /// The destructor for a TransactionalStorageManager should not free any
  /// storage it was passed, but it must delete memory it allocated in the
  /// remote reads vector.
  ~TransactionalStorageManager();

  /// We implement the Create interface via the Put() method, which
  /// sticks an arbitrary type into the database (via templated pointer).
  ///
  /// @tparam     T       T represents an arbitrary type to be added to the
  ///                     database (avoids widespread use of reinterpret_cast)
  ///
  /// @param      key     The key at which to insert the new value
  /// @param      value   A pointer of arbitrary type to be inserted into the
  ///                     data layer
  /// @param      size    Size of the value to be written to storage
  ///
  /// @returns    True if the key was successfully inserted, false otherwise
  template<class T>
  bool Put(const Key& key, T* value, int size = sizeof(T)) {
    // Write object to storage if applicable.
    Config* config = Config::GetConfigInstance();
    if (config->LookupPartition(key) == config->this_node_id) {
      Value* val = new Value(value, size);
      return Storage::GetStorageInstance()->Put(key, val, txn_->txn_id());
    }

    return true;  // Not this node's problem.
  }

  /// We implement the Update interface via the GetMutable() method, which uses
  /// reinterpret_cast to avoid widespread use of it in application code.
  ///
  /// @tparam     T       T represents an arbitrary type to be read from the
  ///                     database (avoids widespread use of reinterpret_cast)
  ///
  /// @param      key     The key to be read in the data layer
  /// @param      size    Size of the value to be written to storage
  ///
  /// @returns    A pointer to the key if it was declared in the transaction
  ///             wrapping the transaction manager, NULL otherwise
  template<class T>
  T* GetMutable(const Key& key, int size = sizeof(T)) {
    if (objects_.count(key) > 0 && objects_[key] != NULL) {
      // Need to issue a put because this is essentially a substitute for
      // the app programmer
      Put(key, reinterpret_cast<T*>(objects_[key]->data()), size);
      return reinterpret_cast<T*>(objects_[key]->data());
    }
    return NULL;
  }

  /// We implement the Read interface via the Get() method, which returns
  /// a constant reference to the data layer object.
  ///
  /// @tparam     T       T represents an arbitrary type to be read from the
  ///                     database (avoids widespread use of reinterpret_cast)
  ///
  /// @param      key     The key to be read in the data layer
  ///
  /// @returns    An immutable cast of the pointer returned by GetMutable()
  template<class T>
  const T* Get(const Key& key) {
    if (objects_.count(key) > 0 && objects_[key] != NULL)
      return reinterpret_cast<T*>(objects_[key]->data());
    return NULL;
  }

  /// We implement the Delete interface via the Delete() method, which takes an
  /// arbitrary key and deletes it from the data layer.
  ///
  /// @param      key     The key of the record to be deleted
  ///
  /// @returns    True if the key is not local, otherwise the value returned
  ///             by calling delete on the actual storage.
  bool Delete(const Key& key);

  /// @todo Alex, you have to document these methods with enough
  ///       information so everyone knows what's going on
  void HandleReadResult(const MessageProto& msg);
  bool ReadyToExecute();

  /// Accessor method for writer_ member variable.  Indicates whether 'txn'
  /// involves any writes at this node.
  bool writer() { return writer_; }

 private:
  friend class Scheduler;

  /// A Connection object that can be used to send and receive messages.
  Connection* connection_;

  /// Transaction that corresponds to this instance of a TSM.
  TxnProto* txn_;

  /// Set by the constructor, indicating whether 'txn' involves any writes at
  /// this node.
  bool writer_;

  /// Local copy of all data objects read/written by 'txn_', populated at
  /// TransactionalStorageManager construction time.
  unordered_map<Key, Value*> objects_;

  vector<Value*> remote_reads_;
};

}  // namespace calvin

#endif  // _DB_BACKEND_TRANSACTIONAL_STORAGE_MANAGER_H_
