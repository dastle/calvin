/// @file
/// @author Thaddeus Diamond <diamond@cs.yale.edu>
/// @version 0.1
/// @since 26 Jan 2012
///
/// @section DESCRIPTION
///
/// This file is an implementation of our asynchronous checkpointing mechanism

#include "checkpointing/multiversion.h"

namespace calvin {

const char* MultiversionCheckpointer::type() {
  return "Checkpointer";
}

const char* MultiversionCheckpointer::name() {
  return "Multiversioning Checkpointer";
}

uint64 MultiversionCheckpointer::HandleDatabaseSize() {
  return 0;
}

Value* MultiversionCheckpointer::HandleGet(const Key& key, VersionID version) {
  return NULL;
}

bool MultiversionCheckpointer::HandlePut(const Key& key, Value* value,
                                  VersionID version) {
  return true;
}

bool MultiversionCheckpointer::PrepareForCheckpoint(VersionID version) {
  if (!checkpoint_lock_.TryReadLock())
    return false;
  historical_version_ = version;
  checkpoint_lock_.Unlock();
  return true;
}

static inline void* RunMultiversionCheckpointer(void* multiversion) {
  (reinterpret_cast<MultiversionCheckpointer*>(multiversion))->BackgroundedCheckpointer();
  return NULL;
}

bool MultiversionCheckpointer::CaptureCheckpoint() {
  return !pthread_create(&checkpointing_daemon_, NULL,
                         &RunMultiversionCheckpointer, this);
}

void MultiversionCheckpointer::BackgroundedCheckpointer() {
  // Get a checkpointer lock to avoid wierd state
  checkpoint_lock_.WriteLock();
  Storage* storage = Storage::GetStorageInstance();
  double checkpoint_start_time = GetTime();

  // Perform a 'SELECT *' against a historical version
  char log_name[200];
  snprintf(log_name, sizeof(log_name), "%s/%ld.chkpnt", CHKPNTDIR, time(NULL));
  FILE* checkpoint = fopen(log_name, "w");
  if (!checkpoint) {
    checkpoint_lock_.Unlock();
    return;
  }

  // Next we iterate through all of the objects and write the stable version
  // to disk
  double total_out = 0.0;
  vector<Key>* keys = storage->GetKeyList(historical_version_);
  unsigned int inserts = keys->size();
  for (vector<Key>::iterator it = keys->begin(); it != keys->end(); it++) {
    // Read in the stable value
    Value* value = storage->Get(*it, historical_version_);
    if (!value)
      continue;

    // Write <len_key_bytes|key|len_value_bytes|value> to disk
    total_out++;
    PrintToFile(checkpoint, it->length(), value->size(), value->data(), *it);
  }

  // Close the file and release the freeze on the database
  fclose(checkpoint);
  checkpoint_lock_.Unlock();

  // Report to user
  if (inserts > 0)
    LOG(INFO) << "Wrote chkpnt (" << (100 * total_out / inserts) << "%%)";
  LOG(INFO) << (GetTime() - GlobalStartTime())
            << "Finished capturing checkpoint ("
            << (GetTime() - checkpoint_start_time) << "s total)";
  return;
}

bool MultiversionCheckpointer::RecoverFromCheckpoint(string filename,
                                              VersionID version) {
  // First, we open the file for writing
  FILE* checkpoint = fopen(filename.c_str(), "r");
  if (!checkpoint)
    return false;

  // Get the main data layer
  Storage* storage = Storage::GetStorageInstance();

  // Retrieve in the following format <keylen|key|vallen|val>
  char inbuf[4096];
  while (ReadFromFile(checkpoint, inbuf, 4) == 4) {
    int key_len = ByteArrayToInt(inbuf);
    if (ReadFromFile(checkpoint, inbuf, key_len) != key_len)
      return false;
    Key key = string(inbuf);

    if (ReadFromFile(checkpoint, inbuf, 4) != 4)
      return false;
    int val_len = ByteArrayToInt(inbuf);

    if (ReadFromFile(checkpoint, inbuf, val_len) != val_len)
      return false;
    Value* value = new Value(CreatePermBuffer(inbuf, val_len), val_len);

    storage->Put(key, value, version);
  }

  // Everything went well
  LOG(INFO) << "Recovered in " << (GetTime() - GlobalStartTime()) << "s";
  return true;
}

}  // namespace calvin
