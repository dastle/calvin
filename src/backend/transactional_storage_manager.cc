/// @file
/// @author Alexander Thomson <thomson@cs.yale.edu>
/// @author Thaddeus Diamond <diamond@cs.yale.edu>
/// @version 0.1
/// @since 26 Nov 2011
///
/// @section DESCRIPTION
///
/// This is the implementation for the TSM (a CRUD interface to the database).

#include "backend/transactional_storage_manager.h"

namespace calvin {

TransactionalStorageManager::TransactionalStorageManager(
      Connection* connection,
      TxnProto* txn)
    : connection_(connection), txn_(txn) {
  Config* config = Config::GetConfigInstance();
  Storage* actual_storage = Storage::GetStorageInstance();
  MessageProto message;

  // If reads are performed at this node, execute local reads and broadcast
  // results to all (other) writers.
  bool reader = false;
  for (int i = 0; i < txn->readers_size(); i++) {
    if (txn->readers(i) == config->this_node_id)
      reader = true;
  }

  if (reader) {
    message.set_destination_channel(IntToString(txn->txn_id()));
    message.set_type(MessageProto::READ_RESULT);

    // Execute local reads.
    for (int i = 0; i < txn->read_set_size(); i++) {
      const Key& key = txn->read_set(i);
      if (config->LookupPartition(key) ==
          config->this_node_id) {
        Value* val = actual_storage->Get(key, txn->txn_id());
        objects_[key] = val;
        message.add_keys(key);
        message.add_values(val == NULL ? "" :
                           reinterpret_cast<char*>(val->data()),
                           val == NULL ? 0 : val->size());
      }
    }
    for (int i = 0; i < txn->read_write_set_size(); i++) {
      const Key& key = txn->read_write_set(i);
      if (config->LookupPartition(key) ==
          config->this_node_id) {
        Value* val = actual_storage->Get(key, txn->txn_id());
        objects_[key] = val;
        message.add_keys(key);
        message.add_values(val == NULL ? "" :
                           reinterpret_cast<char*>(val->data()),
                           val == NULL ? 0 : val->size());
      }
    }

    // Broadcast local reads to (other) writers.
    for (int i = 0; i < txn->writers_size(); i++) {
      if (txn->writers(i) != config->this_node_id) {
        message.set_destination_node(txn->writers(i));
        connection_->Send(message);
      }
    }
  }

  // Note whether this node is a writer. If not, no need to do anything further.
  writer_ = false;
  for (int i = 0; i < txn->writers_size(); i++) {
    if (txn->writers(i) == config->this_node_id)
      writer_ = true;
  }

  // Scheduler is responsible for calling HandleReadResponse. We're done here.
}

void TransactionalStorageManager::HandleReadResult(const MessageProto& msg) {
  assert(msg.type() == MessageProto::READ_RESULT);
  for (int i = 0; i < msg.keys_size(); i++) {
    Value* val = new Value(msg.values(i));
    objects_[msg.keys(i)] = val;
    remote_reads_.push_back(val);
  }
}

bool TransactionalStorageManager::ReadyToExecute() {
  return static_cast<int>(objects_.size()) ==
         txn_->read_set_size() + txn_->read_write_set_size();
}

TransactionalStorageManager::~TransactionalStorageManager() {
  for (vector<Value*>::iterator it = remote_reads_.begin();
       it != remote_reads_.end(); ++it) {
    delete *it;
  }
}

bool TransactionalStorageManager::Delete(const Key& key) {
  // Delete object from storage if applicable.
  Config* config = Config::GetConfigInstance();
  if (config->LookupPartition(key) == config->this_node_id)
    return Storage::GetStorageInstance()->Delete(key, txn_->txn_id());

  return true;  // Not this node's problem.
}

}  // namespace calvin
