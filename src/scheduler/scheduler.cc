/// @file
/// @author Alexander Thomson (thomson@cs.yale.edu)
/// @version 0.1
/// @since 5 April 2012
///
/// @section DESCRIPTION
///
/// The deterministic lock manager implements deterministic locking as described
/// in 'The Case for Determinism in Database Systems', VLDB 2010. Each
/// transaction must request all locks it will ever need before the next
/// transaction in the specified order may acquire any locks. Each lock is then
/// granted to transactions in the order in which they requested them (i.e. in
/// the global transaction order).

#include <python/Python.h>
#include <tr1/unordered_map>
#include <cstdlib>
#include <string>
#include <utility>
#include <set>
#include <glog/logging.h>

#include "stored_procedures/application.h"
#include "checkpointing/checkpointer.h"
#include "common/utils.h"
#include "common/zmq.hpp"
#include "connection/connection.h"
#include "backend/storage.h"
#include "backend/transactional_storage_manager.h"
#include "proto/message.pb.h"
#include "proto/txn.pb.h"
/// @TODO: Get rid of this dependency, all it is for is total_active_txns_
#include "sequencer/sequencer.h"
#include "scheduler/lock_manager.h"
#include "scheduler/scheduler.h"

#define COL_INT 100
#define CHKPNT_INTERVAL 30
#define MAX_TIME_RUNNING 150

namespace calvin {

using std::set;
using std::pair;
using std::string;
using std::tr1::unordered_map;
using zmq::socket_t;

/// @todo put these back in deterministic scheduler::run
int txns = 0;
int executing_txns = 0;
int pending_txns = 0;
double time;
double start_time;
bool scheduler_is_running = true;

// reporting stuff
pthread_t reporter_thread;
void* report(void* arg) {
  double duration = 10;  // seconds
  double frequency = 10;  // hertz

  double start = GetTime();
  double tick = start;
  while (scheduler_is_running) {
    // Exit when we're done taking data.
    if (GetTime() > start + duration)
      exit(0);

    // Report output if enough time has passed since last tick.
    if (GetTime() > tick + 1.0/frequency) {
      tick = GetTime();
      LOG(INFO) << "time " << tick - start
                << " executed " << *total_completed_txns_;
      total_completed_txns_ = 0;
      // Reset txn count.
      time = GetTime();
    }
    // Sleep a bit.
    usleep(0.1/frequency);
  }
  return NULL;
}

// end reporting stuff

static void DeleteTxnPtr(void* data, void* hint) { free(data); }

void Scheduler::SendTxnPtr(socket_t* socket, TxnProto* txn) {
  TxnProto** txn_ptr = reinterpret_cast<TxnProto**>(malloc(sizeof(txn)));
  *txn_ptr = txn;
  zmq::message_t msg(txn_ptr, sizeof(*txn_ptr), DeleteTxnPtr, NULL);
  socket->send(msg);
}

TxnProto* Scheduler::GetTxnPtr(socket_t* socket,
                                            zmq::message_t* msg) {
  if (!socket->recv(msg, ZMQ_NOBLOCK))
    return NULL;
  TxnProto* txn = *reinterpret_cast<TxnProto**>(msg->data());
  return txn;
}

Scheduler::Scheduler(Connection* batch_connection)
    : batch_connection_(batch_connection) {
  lock_manager_ = new LockManager(&ready_txns_);
  scheduler_is_running = true;

  // Initialize the Python interpreter.
  Py_Initialize();
}

void* Scheduler::RunWorkerThread(void* arg) {
  int thread =
      reinterpret_cast<pair<int, Scheduler*>*>(arg)->first;
  Scheduler* scheduler =
      reinterpret_cast<pair<int, Scheduler*>*>(arg)->second;

  unordered_map<string, TransactionalStorageManager*> active_txns;

  // Begin main loop.
  MessageProto message;
  zmq::message_t msg;
  while (scheduler_is_running) {
    if (scheduler->thread_connections_[thread]->GetMessage(&message)) {
      // Remote read result.
      assert(message.type() == MessageProto::READ_RESULT);
      TransactionalStorageManager* manager
        = active_txns[message.destination_channel()];
      manager->HandleReadResult(message);
      if (manager->ReadyToExecute()) {
        // Execute transaction and clean up.
        TxnProto* txn = manager->txn_;
        if (txn->has_python_code()) {
          // Txn provides user-specified python code to execute.
          PyRun_SimpleString(txn->python_code().c_str());
        } else {
          // No python code specified. Treat txn as a stored procedure.
          scheduler->application_->Execute(txn, manager);
        }
        ++total_completed_txns_;
        delete manager;

        scheduler->thread_connections_[thread]->
            UnlinkChannel(IntToString(txn->txn_id()));
        active_txns.erase(message.destination_channel());
        // Respond to scheduler;
        scheduler->SendTxnPtr(scheduler->responses_out_[thread], txn);
      }
    } else {
      // No remote read result found, start on next txn if one is waiting.
      scheduler->requests_in_lock_.Lock();
      TxnProto* txn = scheduler->GetTxnPtr(scheduler->requests_in_, &msg);
      scheduler->requests_in_lock_.Unlock();
//      if (txn != NULL)
//        scheduler->SendTxnPtr(scheduler->responses_out_[thread], txn);
      if (txn != NULL) {
        scheduler->thread_connections_[thread]->
            LinkChannel(IntToString(txn->txn_id()));

        // Create manager.
        TransactionalStorageManager* manager =
            new TransactionalStorageManager(
                      scheduler->thread_connections_[thread],
                      txn);

        if (manager->writer()) {
          // Writes occur at this node.
          if (manager->ReadyToExecute()) {
            // No remote reads. Execute and clean up.
            scheduler->application_->Execute(txn, manager);
            ++total_completed_txns_;
            delete manager;

            scheduler->thread_connections_[thread]->
                UnlinkChannel(IntToString(txn->txn_id()));
            // Respond to scheduler;
            scheduler->SendTxnPtr(scheduler->responses_out_[thread], txn);
          } else {
            // There are outstanding remote reads.
            active_txns[IntToString(txn->txn_id())] = manager;
          }
        } else {
          // No writes at this node. We're done here.
          delete manager;

          scheduler->thread_connections_[thread]->
              UnlinkChannel(IntToString(txn->txn_id()));
          // Respond to scheduler;
          scheduler->SendTxnPtr(scheduler->responses_out_[thread], txn);
        }
      }
    }
  }
  return NULL;
}

Scheduler::~Scheduler() {
  LOG(INFO) << "... shutting down scheduler ...";
  scheduler_is_running = false;
  for (int i = 0; i < NUM_THREADS; i++)
    pthread_join(threads_[i], NULL);
  pthread_join(reporter_thread, NULL);
}

// Returns ptr to heap-allocated
unordered_map<int, MessageProto*> batches;
MessageProto* GetBatch(int batch_id, Connection* connection) {
  if (batches.count(batch_id) > 0) {
    // Requested batch has already been received.
    MessageProto* batch = batches[batch_id];
    batches.erase(batch_id);
    return batch;
  } else {
    MessageProto* message = new MessageProto();
    while (connection->GetMessage(message)) {
      assert(message->type() == MessageProto::TXN_BATCH);
      if (message->batch_number() == batch_id) {
        return message;
      } else {
        batches[message->batch_number()] = message;
        message = new MessageProto();
      }
    }
    delete message;
    return NULL;
  }
}

void Scheduler::Run(const Application* application) {
  // Set up request channel.
  requests_out_ =
        new socket_t(*batch_connection_->multiplexer()->context(), ZMQ_PAIR);
  requests_in_ =
        new socket_t(*batch_connection_->multiplexer()->context(), ZMQ_PAIR);
  requests_out_->bind("inproc://requests");
  requests_in_->connect("inproc://requests");

  // Set up response channels.
  char endpoint[256];
  snprintf(endpoint, sizeof(endpoint), "inproc://responses");
  responses_in_ =
      new socket_t(*batch_connection_->multiplexer()->context(), ZMQ_PULL);
  responses_in_->bind(endpoint);
  for (int i = 0; i < NUM_THREADS; i++) {
    responses_out_[i] =
        new socket_t(*batch_connection_->multiplexer()->context(), ZMQ_PUSH);
    responses_out_[i]->connect(endpoint);
  }

  // Start all worker threads.
  for (int i = 0; i < NUM_THREADS; i++) {
    string channel("scheduler");
    channel.append(IntToString(i));
    thread_connections_[i] =
        batch_connection_->multiplexer()->NewConnection(channel);
    pthread_create(&(threads_[i]), NULL, RunWorkerThread,
                   reinterpret_cast<void*>(
                      new pair<int, Scheduler*>(i, this)));
  }

  pthread_create(&reporter_thread, NULL, report, NULL);

  // Run main loop.
  application_ = application;
  Checkpointer* checkpointer = Storage::GetStorageInstance()->GetCheckpointer();
  LOG(INFO) << "checkpoint: " << (checkpointer ? checkpointer->name() : "none");
  MessageProto message;
  MessageProto* batch_message = NULL;
  time = GetTime();
  start_time = GetTime();
  SetGlobalStartTime();
  int batch_offset = 0;
  int batch_number = 0;

  // Set up checkpointing vars
  int64 upcoming_checkpoint = LLONG_MAX;
  double chkpnt_period = 0.1;
  bool* collapsed = new bool[20];
  for (int i = 0; i < 20; i++)
    collapsed[i] = false;

  zmq::message_t msg;
  set<TxnProto*> active_txn_set;
  while (scheduler_is_running) {
    TxnProto* done_txn = GetTxnPtr(responses_in_, &msg);

    if (done_txn != NULL) {
      // We have received a finished transaction back, release the lock
      lock_manager_->Release(done_txn);
      executing_txns--;
      --total_active_txns_;

      // Capture checkpoint if we're ready
      if (checkpointer && upcoming_checkpoint != LLONG_MAX &&
          done_txn->txn_id() > upcoming_checkpoint + 1000) {
        upcoming_checkpoint = LLONG_MAX;
        LOG(INFO) << (GetTime() - start_time) << "Beginning checkpoint capture";
        checkpointer->CaptureCheckpoint();
      }

      if (rand() % done_txn->writers_size() == 0)
        txns++;

      active_txn_set.erase(done_txn);
      delete done_txn;

    } else {
      // Have we run out of txns in our batch? Let's get some new ones.
      if (batch_message == NULL) {
        batch_message = GetBatch(batch_number, batch_connection_);

      // Done with current batch, get next.
      } else if (batch_offset >= batch_message->data_size()) {
        batch_offset = 0;
        batch_number++;
        delete batch_message;
        batch_message = GetBatch(batch_number, batch_connection_);

      // Current batch has remaining txns, grab up to 10.
      } else if (executing_txns + pending_txns < 2000) {
        for (int i = 0; i < 100; i++) {
          if (batch_offset >= batch_message->data_size()) {
            // Oops we ran out of txns in this batch. Stop adding txns for now.
            break;
          }
          TxnProto* txn = new TxnProto();
          txn->ParseFromString(batch_message->data(batch_offset));
          batch_offset++;

          // Prepare for checkpoint
          int time_run = GetTime() - start_time;
          if (checkpointer && time_run > CHKPNT_INTERVAL * chkpnt_period
              && checkpointer->PrepareForCheckpoint(txn->txn_id() + 1000)) {
            chkpnt_period++;
            upcoming_checkpoint = txn->txn_id() + 1000;
          }

          lock_manager_->Lock(txn);
          pending_txns++;
        }
      }
    }

    // Start executing any and all ready transactions to get them off our plate
    while (!ready_txns_.empty()) {
      TxnProto* txn = ready_txns_.front();
      ready_txns_.pop_front();

      pending_txns--;
      executing_txns++;
      active_txn_set.insert(txn);

      SendTxnPtr(requests_out_, txn);
    }

    // NOTE: all reporting moved to "report()" function above

    // Collapse checkpoint if of the right type
    if (checkpointer
        && (!strcmp(checkpointer->name(), "Bloom Filter Hobbes")
            || !strcmp(checkpointer->name(),
                       "Dictionary Hash Hobbes")
            || !strcmp(checkpointer->name(),
                       "Interleaved Hash Map Ping-Pong")
            || !strcmp(checkpointer->name(),
                       "Hash Map Ping-Pong"))
        && !collapsed[static_cast<int>((GetTime() - start_time) / COL_INT)]
        && checkpointer->CollapseCheckpoints()) {
      LOG(INFO) << (GetTime() - start_time) << "Now collapsing checkpoints";
      collapsed[static_cast<int>(GetTime() - start_time) / COL_INT] = true;
    }
  }
}

}  // namespace calvin
