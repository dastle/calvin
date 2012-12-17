/// @file
/// @author Alexander Thomson (thomson@cs.yale.edu)
/// @version 0.1
/// @since 5 April 2012
///
/// @section DESCRIPTION
///
/// The sequencer component of the system is responsible for choosing a global
/// serial order of transactions to which execution must maintain equivalence.

#include "sequencer/sequencer.h"
#include "stored_procedures/application.h"
#include "stored_procedures/microbenchmark/microbenchmark.h"

Atomic<int> total_active_txns_;
Atomic<int> total_completed_txns_;

namespace calvin {

void* Sequencer::RunSequencerWriter(void *arg) {
  reinterpret_cast<Sequencer*>(arg)->RunWriter();
  return NULL;
}

void* Sequencer::RunSequencerReader(void *arg) {
  reinterpret_cast<Sequencer*>(arg)->RunReader();
  return NULL;
}

Sequencer::Sequencer(Connection* connection)
    : epoch_duration_(0.01), connection_(connection),
      deconstructor_invoked_(false) {
  total_active_txns_ = 0;
  pthread_mutex_init(&mutex_, NULL);
  // Start Sequencer main loops running in background thread.
  pthread_create(&writer_thread_, NULL, RunSequencerWriter,
      reinterpret_cast<void*>(this));
  pthread_create(&reader_thread_, NULL, RunSequencerReader,
      reinterpret_cast<void*>(this));
}

Sequencer::~Sequencer() {
  LOG(INFO) << "... shutting down sequencer ...";
  deconstructor_invoked_ = true;
  pthread_join(writer_thread_, NULL);
  pthread_join(reader_thread_, NULL);
}

void Sequencer::FindParticipatingNodes(const TxnProto& txn, set<int>* nodes) {
  nodes->clear();
  Config* config = Config::GetConfigInstance();
  for (int i = 0; i < txn.read_set_size(); i++)
    nodes->insert(config->LookupPartition(txn.read_set(i)));
  for (int i = 0; i < txn.write_set_size(); i++)
    nodes->insert(config->LookupPartition(txn.write_set(i)));
  for (int i = 0; i < txn.read_write_set_size(); i++)
    nodes->insert(config->LookupPartition(txn.read_write_set(i)));
}

void Sequencer::RunWriter() {
  Config* config = Config::GetConfigInstance();
  Spin(1);

#ifdef PAXOS
  Paxos paxos(ZOOKEEPER_CONF, false);
#endif

  // Synchronization loadgen start with other sequencers.
  MessageProto synchronization_message;
  synchronization_message.set_type(MessageProto::EMPTY);
  synchronization_message.set_destination_channel("sequencer");
  for (uint32 i = 0; i < config->all_nodes.size(); i++) {
    synchronization_message.set_destination_node(i);
    if (i != static_cast<uint32>(config->this_node_id))
      connection_->Send(synchronization_message);
  }

  /// @TODO: (Alex) There is some sort of bug with synchronization messages
  ///               in cluster deployments
  uint32 synchronization_counter = 1;
  while (synchronization_counter < config->all_nodes.size()) {
    synchronization_message.Clear();
    if (connection_->GetMessage(&synchronization_message)) {
      assert(synchronization_message.type() == MessageProto::EMPTY);
      synchronization_counter++;
    }
  }
  LOG(INFO) << "Starting sequencer";

  // Set up batch messages for each system node.
  MessageProto batch;
  batch.set_destination_channel("sequencer");
  batch.set_destination_node(-1);
  string batch_string;
  batch.set_type(MessageProto::TXN_BATCH);

  for (int batch_number = config->this_node_id;
       !deconstructor_invoked_;
       batch_number += config->all_nodes.size()) {
    // Begin epoch.
    double epoch_start = GetTime();
    batch.set_batch_number(batch_number);
    batch.clear_data();

    // Collect txn requests for this epoch.
    int txn_id_offset = 0;
    while (!deconstructor_invoked_ &&
           GetTime() < epoch_start + epoch_duration_) {
      // Add next txn request to batch.
      if (*total_active_txns_ < MAX_ACTIVE_TXNS
          && batch.data_size() < MAX_BATCH_SIZE) {

          // LOADGEN code used for checkpointing paper (November 2012).
          ++total_active_txns_;
          int txn_id = batch_number * MAX_BATCH_SIZE + txn_id_offset;
          TxnProto* txn =
              Application::GetApplicationInstance()->NewTxn(txn_id, 0, "");
          string txn_string;
          txn->SerializeToString(&txn_string);
          batch.add_data(txn_string);
          txn_id_offset++;
          delete txn;
	
//        @TODO (Alex): Put this back and get txn REQUESTS working?
//        if (ProcessMgr::GetProcessMgrInstance()->GetQueryNonblocking()) {
//        }
      } else {
        usleep(10);
      }
    }

    // Send this epoch's requests to Paxos service.
    batch.SerializeToString(&batch_string);
#ifdef PAXOS
    paxos.SubmitBatch(batch_string);
#else
    pthread_mutex_lock(&mutex_);
    batch_queue_.push(batch_string);
    pthread_mutex_unlock(&mutex_);
#endif
  }

  Spin(1);
}

void Sequencer::RunReader() {
  Config* config = Config::GetConfigInstance();
  Spin(1);
#ifdef PAXOS
  Paxos paxos(ZOOKEEPER_CONF, true);
#endif

  // Set up batch messages for each system node.
  map<int, MessageProto> batches;
  for (map<int, Node*>::iterator it = config->all_nodes.begin();
       it != config->all_nodes.end(); ++it) {
    batches[it->first].set_destination_channel("scheduler_");
    batches[it->first].set_destination_node(it->first);
    batches[it->first].set_type(MessageProto::TXN_BATCH);
  }

  double time = GetTime();
  int txn_count = 0;
  int batch_count = 0;
  int batch_number = config->this_node_id;

  while (!deconstructor_invoked_) {
    // Get batch from Paxos service.
    string batch_string;
    MessageProto batch_message;
#ifdef PAXOS
    paxos.GetNextBatchBlocking(&batch_string);
#else
    bool got_batch = false;
    do {
      pthread_mutex_lock(&mutex_);
      if (batch_queue_.size()) {
        batch_string = batch_queue_.front();
        batch_queue_.pop();
        got_batch = true;
      }
      pthread_mutex_unlock(&mutex_);
      if (!got_batch)
        Spin(0.001);
    } while (!got_batch);
#endif

    batch_message.ParseFromString(batch_string);
    for (int i = 0; i < batch_message.data_size(); i++) {
      TxnProto txn;
      txn.ParseFromString(batch_message.data(i));

      // Compute readers & writers; store in txn proto.
      set<int> readers;
      set<int> writers;
      for (int i = 0; i < txn.read_set_size(); i++)
        readers.insert(config->LookupPartition(txn.read_set(i)));
      for (int i = 0; i < txn.write_set_size(); i++)
        writers.insert(config->LookupPartition(txn.write_set(i)));
      for (int i = 0; i < txn.read_write_set_size(); i++) {
        writers.insert(config->LookupPartition(txn.read_write_set(i)));
        readers.insert(config->LookupPartition(txn.read_write_set(i)));
      }

      for (set<int>::iterator it = readers.begin(); it != readers.end(); ++it)
        txn.add_readers(*it);
      for (set<int>::iterator it = writers.begin(); it != writers.end(); ++it)
        txn.add_writers(*it);

      bytes txn_data;
      txn.SerializeToString(&txn_data);

      // Compute union of 'readers' and 'writers' (store in 'readers').
      for (set<int>::iterator it = writers.begin(); it != writers.end(); ++it)
        readers.insert(*it);

      // Insert txn into appropriate batches.
      for (set<int>::iterator it = readers.begin(); it != readers.end(); ++it)
        batches[*it].add_data(txn_data);

      txn_count++;
    }

    // Send this epoch's requests to all schedulers.
    if (batch_message.data_size() > 0) {
      for (map<int, MessageProto>::iterator it = batches.begin();
           it != batches.end(); ++it) {
        it->second.set_batch_number(batch_number);
        connection_->Send(it->second);
        it->second.clear_data();
      }
      batch_number += config->all_nodes.size();
      batch_count++;
    }

    // Report output.
    if (GetTime() > time + 1) {
#ifdef VERBOSE_SEQUENCER
      LOG(INFO) << "Submitted " << txn_count << " txns in " << batch_count
                << " batches.";
#endif
      // Reset txn count.
      time = GetTime();
      txn_count = 0;
      batch_count = 0;
    }
  }
  Spin(1);
}

}  // namespace calvin
