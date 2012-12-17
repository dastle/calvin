/// @file
/// @author Alexander Thomson (thomson@cs.yale.edu)
/// @version 0.1
/// @since 5 April 2012
///
/// @section DESCRIPTION
///
/// A database node's Scheduler determines what transactions should be run when
/// at that node. It is responsible for communicating with other nodes when
/// necessary to determine whether a transaction can be scheduled. It also
/// forwards messages on to the backend that are sent from other nodes
/// participating in distributed transactions.

#ifndef _DB_SCHEDULER_SCHEDULER_H_
#define _DB_SCHEDULER_SCHEDULER_H_

#include <glog/logging.h>
#include <pthread.h>
#include <deque>

#include "common/mutex.h"

namespace zmq {
  class socket_t;
  class message_t;
}
using zmq::socket_t;

class Configuration;
class Checkpointer;
class Connection;
class LockManager;
class Storage;
class TxnProto;

namespace calvin {

using std::deque;

/// Hard-configurable number of worker threads. For good performance, this
/// should not exceed (total cores - 3) or (total cores / 2), whichever is
/// greater.
#define NUM_THREADS 5

class Scheduler {
 public:
  explicit Scheduler(Connection* batch_connection);
  ~Scheduler();

  /// Scheduler::Run() is the Scheduler's main loop. It receives new txn
  /// requests, handle's all locking, and coordinates execution of txns by
  /// worker threads, function. All input to the Scheduler (e.g. new txn
  /// requests) is sent via messages addressed to the "scheduler_" channel.
  void Run(const Application* application);

 private:
  // Application currently being run.
  const Application* application_;

  // Function for starting main loops in a separate pthreads.
  static void* RunWorkerThread(void* arg);

  void SendTxnPtr(socket_t* socket, TxnProto* txn);
  TxnProto* GetTxnPtr(socket_t* socket, zmq::message_t* msg);

  // Thread contexts and their associated Connection objects.
  pthread_t threads_[NUM_THREADS];
  Connection* thread_connections_[NUM_THREADS];

  // Connection for receiving txn batches from sequencer.
  Connection* batch_connection_;

  // The per-node lock manager tracks what transactions have temporary ownership
  // of what database objects, allowing the scheduler to track LOCAL conflicts
  // and enforce equivalence to transaction orders.
  LockManager* lock_manager_;

  // Queue of transaction ids of transactions that have acquired all locks that
  // they have requested.
  std::deque<TxnProto*> ready_txns_;

  // Sockets for communication between main scheduler thread and worker threads.
  /// @TODO(alex) Use a ThreadPool for all threads + communications!
  socket_t* requests_out_;
  socket_t* requests_in_;
  socket_t* responses_out_[NUM_THREADS];
  socket_t* responses_in_;

  // mutex guarding 'requests_in_' since multiple threads will try to read
  // the next request off of it simultaneously
  Mutex requests_in_lock_;
};

}  // namespace calvin

#endif  // _DB_SCHEDULER_SCHEDULER_H_
