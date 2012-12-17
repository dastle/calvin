/// @file
/// @author Alexander Thomson (thomson@cs.yale.edu)
/// @version 0.1
/// @since 5 April 2012
///
/// @section DESCRIPTION
///
/// The sequencer component of the system is responsible for choosing a global
/// serial order of transactions to which execution must maintain equivalence.

#ifndef _DB_SEQUENCER_SEQUENCER_H_
#define _DB_SEQUENCER_SEQUENCER_H_

#include <set>
#include <string>
#include <queue>
#include <map>
#include <utility>

#include "backend/storage.h"
#include "common/configuration.h"
#include "connection/connection.h"
#include "common/atomic.h"
#include "common/utils.h"
#include "process_mgr/process_mgr.h"
#include "proto/message.pb.h"
#include "proto/txn.pb.h"

// #define PAXOS
#ifdef PAXOS
  #include "paxos/paxos.h"
#endif

#define VERBOSE_SEQUENCER

using std::set;
using std::string;
using std::queue;
using std::map;
using std::multimap;

extern Atomic<int> total_active_txns_;
extern Atomic<int> total_completed_txns_;

namespace calvin {

class Client {
 public:
  virtual ~Client() {}
  virtual TxnProto* GetTxn(int txn_id) = 0;
};

class Sequencer {
 public:
  // The constructor creates background threads and starts the Sequencer's main
  // loops running.
  explicit Sequencer(Connection* connection);

  // Halts the main loops.
  ~Sequencer();

 private:
  // Sequencer's main loops:
  //
  // RunWriter:
  //  while true:
  //    Spend epoch_duration collecting client txn requests into a batch.
  //    Send batch to Paxos service.
  //
  // RunReader:
  //  while true:
  //    Spend epoch_duration collecting client txn requests into a batch.
  //
  // Executes in a background thread created and started by the constructor.
  void RunWriter();
  void RunReader();

  // Functions to start the Multiplexor's main loops, called in new pthreads by
  // the Sequencer's constructor.
  static void* RunSequencerWriter(void *arg);
  static void* RunSequencerReader(void *arg);

  // Sets '*nodes' to contain the node_id of every node participating in 'txn'.
  void FindParticipatingNodes(const TxnProto& txn, set<int>* nodes);

  // Length of time spent collecting client requests before they are ordered,
  // batched, and sent out to schedulers.
  double epoch_duration_;

  // Connection for sending and receiving protocol messages.
  Connection* connection_;

  // Client from which to get incoming txns.
  Client* client_;

  // Pointer to this node's storage object, for prefetching.
  Storage* storage_;

  // Separate pthread contexts in which to run the sequencer's main loops.
  pthread_t writer_thread_;
  pthread_t reader_thread_;

  // False until the deconstructor is called. As soon as it is set to true, the
  // main loop sees it and stops.
  bool deconstructor_invoked_;

  // Queue for sending batches from writer to reader if not in paxos mode.
  queue<string> batch_queue_;
  pthread_mutex_t mutex_;
};

}  // namespace calvin

#endif  // _DB_SEQUENCER_SEQUENCER_H_
