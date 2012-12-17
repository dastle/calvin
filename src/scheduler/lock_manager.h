/// @file
/// @author Alexander Thomson (thomson@cs.yale.edu)
/// @version 0.1
/// @since 5 April 2012
///
/// @section DESCRIPTION
///
/// Lock manager implementing deterministic two-phase locking as described in
/// 'The Case for Determinism in Database Systems'.

#ifndef _DB_SCHEDULER_LOCK_MANAGER_H_
#define _DB_SCHEDULER_LOCK_MANAGER_H_

#include <tr1/unordered_map>
#include <deque>

#include "common/configuration.h"
#include "scheduler/lock_manager.h"

using std::tr1::unordered_map;
using std::deque;

/// @TODO(alex): Is this hash-based monstrosity really what we want to use?
#define TABLE_SIZE 1000000

class TxnProto;

/// Locks may be requested in either read(shared) or write(exclusive) mode.
enum LockMode {
  READ = 0,
  WRITE = 1,
  UNLOCKED = 2,
};

/// @class  LockManager
/// @brief  Calvin's main lock manager for discovering RW/WW conflicts between
///         transactions.
///
/// @TODO(alex): Update documentation for Lock/Release.
class LockManager {
 public:
  explicit LockManager(deque<TxnProto*>* ready_txns);
  ~LockManager() {}

  /// Attempts to assign the lock for each key in keys to the specified
  /// transaction. Returns the number of requested locks NOT assigned to the
  /// transaction (therefore Lock() returns 0 if the transaction successfully
  /// acquires all locks).
  ///
  /// Requires: 'read_keys' and 'write_keys' do not overlap, and neither
  ///           contains duplicate keys.
  /// Requires: Lock has not previously been called with this txn_id. Note that
  ///           this means Lock can only ever be called once per txn.
  int Lock(TxnProto* txn);

  /// For each key in 'keys':
  ///   - If the specified transaction owns the lock on the item, the lock is
  ///     released.
  ///   - If the transaction is in the queue to acquire a lock on the item, the
  ///     request is cancelled and the transaction is removed from the item's
  ///     queue.
  void Release(const Key& key, TxnProto* txn);
  void Release(TxnProto* txn);

 private:
  int Hash(const Key& key) {
    uint64 hash = 2166136261;
    for (size_t i = 0; i < key.size(); i++) {
      hash = hash ^ (key[i]);
      hash = hash * 16777619;
    }
    return hash % TABLE_SIZE;
  }

  /// Returns true iff key is stored at this node.
  bool IsLocal(const Key& key) {
    Config* config = Config::GetConfigInstance();
    return config->LookupPartition(key) == config->this_node_id;
  }

  // The LockManager's lock table tracks all lock requests. For a
  // given key, if 'lock_table_' contains a nonempty queue, then the item with
  // that key is locked and either:
  //  (a) first element in the queue specifies the owner if that item is a
  //      request for a write lock, or
  //  (b) a read lock is held by all elements of the longest prefix of the queue
  //      containing only read lock requests.
  // Note: using STL deque rather than queue for erase(iterator position).
  struct LockRequest {
    LockRequest(LockMode m, TxnProto* t) : txn(t), mode(m) {}
    TxnProto* txn;  // Pointer to txn requesting the lock.
    LockMode mode;  // Specifies whether this is a read or write lock request.
  };
  deque<LockRequest>* lock_table_[TABLE_SIZE];

  // Queue of pointers to transactions that have acquired all locks that
  // they have requested. 'ready_txns_[key].front()' is the owner of the lock
  // for a specified key.
  //
  // Owned by the DeterministicScheduler.
  deque<TxnProto*>* ready_txns_;

  // Tracks all txns still waiting on acquiring at least one lock. Entries in
  // 'txn_waits_' are invalided by any call to Release() with the entry's
  // txn.
  unordered_map<TxnProto*, int> txn_waits_;
};
#endif  // _DB_SCHEDULER_LOCK_MANAGER_H_
