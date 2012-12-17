/// @file
/// @author Thaddeus Diamond <diamond@cs.yale.edu>
/// @version 0.1
/// @since 26 Jan 2012
///
/// @section DESCRIPTION
///
/// This file is an implementation of our asynchronous checkpointing mechanism

#include "stored_procedures/application.h"
#include "checkpointing/hobbes.h"

namespace calvin {

const char* Hobbes::type() {
  return "Checkpointer";
}

const char* Hobbes::name() {
  switch (filter_type_) {
    case NONE:
      return "Hobbes";
    case DICTIONARY_HASH:
      return "Dictionary Hash Hobbes";
    case BLOOM_FILTER:
      return "Bloom Filter Hobbes";
  }
  return NULL;
}

uint64 Hobbes::HandleDatabaseSize() {
  return 0;
}

Value* Hobbes::HandleGet(const Key& key, VersionID version) {
  return NULL;
}

bool Hobbes::HandlePut(const Key& key, Value* value, VersionID version) {
  // Prune, if necessary, the closest version on either side of the stable_
  // line.  However, it's tricky if this is a put after stable_.  Then,
  // we definitely don't want to prune the stable_ version.
  Storage* storage = Storage::GetStorageInstance();
  storage->PruneRow(key, version, true,
                    (version < stable_ ? -1 * VERSION_ID_MAX : stable_), true);

  // Don't go any further if you're not doing hashing
  if (filter_type_ == NONE)
    return true;

  // Ensure you get the lock for thread safety, update b/c it's now dirty
  filter_lock_.ReadLock();
  char* updatable = NULL;
  if (version <= stable_ || stable_ < 0)
    updatable = filter_stable_;
  else if (version > stable_)
    updatable = filter_unstable_;

  if (filter_type_ == BLOOM_FILTER) {
    unsigned int loc_one      = FNVHash(key)    % filter_length_;
    unsigned int loc_one_rem  = FNVHash(key)    % 8;
    unsigned int loc_two      = FNVModHash(key) % filter_length_;
    unsigned int loc_two_rem  = FNVModHash(key) % 8;

    updatable[loc_one] |= (1 << loc_one_rem);
    updatable[loc_two] |= (1 << loc_two_rem);
  } else if (filter_type_ == DICTIONARY_HASH) {
    updatable[Application::GetApplicationInstance()->KeyToInt(key)] = 1;
  }

  filter_lock_.Unlock();
  return true;
}

bool Hobbes::PrepareForCheckpoint(VersionID stable) {
  if (!checkpoint_lock_.TryReadLock())
    return false;

  stable_ = stable;
  checkpoint_lock_.Unlock();
  return true;
}

static inline void* RunHobbesCheckpointer(void* hobbes) {
  (reinterpret_cast<Hobbes*>(hobbes))->BackgroundedCheckpointer();
  return NULL;
}

bool Hobbes::CaptureCheckpoint() {
  return !pthread_create(&checkpointing_daemon_, NULL,
                         &RunHobbesCheckpointer, this);
}

void Hobbes::BackgroundedCheckpointer() {
  Storage* storage = Storage::GetStorageInstance();

  // First, we open the file for writing
  char log_name[200];
  snprintf(log_name, sizeof(log_name), "%s/%ld.chkpnt", CHKPNTDIR, stable_);
  FILE* checkpoint = fopen(log_name, "w");
  if (!checkpoint)
    return;

  // Get a checkpointer lock to avoid wierd state
  checkpoint_lock_.WriteLock();
  double checkpoint_start_time = GetTime();

  // Next we iterate through all of the objects and write the stable version
  // to disk
  double total_out = 0.0;
  vector<Key>* keys = storage->GetKeyList(stable_);
  c_inserts_ = keys->size();
  for (vector<Key>::iterator it = keys->begin(); it != keys->end(); it++) {
    // Check for filter dirty when option enabled
    if (filter_type_ == BLOOM_FILTER) {
      unsigned int loc_one      = FNVHash(*it)    % filter_length_;
      unsigned int loc_one_rem  = FNVHash(*it)    % 8;
      unsigned int loc_two      = FNVModHash(*it) % filter_length_;
      unsigned int loc_two_rem  = FNVModHash(*it) % 8;
      if (!((filter_stable_[loc_one] & ((1 << loc_one_rem)))
             && (filter_stable_[loc_two] & ((1 << loc_two_rem)))))
        continue;
    } else if (filter_type_ == DICTIONARY_HASH) {
      if (!filter_stable_[Application::GetApplicationInstance()->KeyToInt(*it)])
        continue;
    }

    // Read in the stable value
    Value* value = storage->Get(*it, stable_);
    if (!value)
      continue;

    // Write <len_key_bytes|key|len_value_bytes|value> to disk
    total_out++;
    PrintToFile(checkpoint, it->length(), value->size(), value->data(), *it);

    // Prune out anything less than the stable value, so long as its not newest
    storage->PruneRow(*it, stable_ + 1, true);
  }

  // Reset stable period, close checkpoint and report to user
  stable_ = -1 * VERSION_ID_MAX;
  if (c_inserts_ > 0)
    LOG(INFO) << "Wrote chkpnt (" << (100 * total_out / c_inserts_) << "%%)";
  LOG(INFO) << (GetTime() - GlobalStartTime())
            << "Finished capturing checkpoint ("
            << (GetTime() - checkpoint_start_time) << "s total)";

  // Don't go forward after closing file if no bloom filter
  if (filter_type_ == NONE) {
    fclose(checkpoint);
    checkpoint_lock_.Unlock();
    return;
  }

  // Re-zero the bloom filter, and swap the bloom filter pointers
  for (int i = 0; i < filter_length_; i++)
    filter_stable_[i] = '\0';
  filter_lock_.WriteLock();
  char* temp = filter_unstable_;
  filter_unstable_ = filter_stable_;
  filter_stable_ = temp;

  // Unlock exclusive locks
  fclose(checkpoint);
  filter_lock_.Unlock();
  checkpoint_lock_.Unlock();
}

bool Hobbes::RecoverFromCheckpoint(string filename, VersionID version) {
  // First, we open the file for writing
  FILE* checkpoint = fopen(filename.c_str(), "r");
  if (!checkpoint)
    return false;

  // Prevent other data layer access
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
