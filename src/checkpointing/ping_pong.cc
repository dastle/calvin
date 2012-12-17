/// @file
/// @author Thaddeus Diamond <diamond@cs.yale.edu>
/// @version 0.1
/// @since 26 Jan 2012
///
/// @section DESCRIPTION
///
/// This file is an implementation of Ping-Pong, a competitor checkpointer

#include "checkpointing/ping_pong.h"

namespace calvin {

const char* PingPong::type() {
  return "Checkpointer";
}

const char* PingPong::name() {
  switch (level_) {
    case NONE:
      return "Ping-Pong";
    case INTERLEAVED:
      return "Interleaved Ping-Pong";
    case HASH_MAP:
      return "Hash Map Ping-Pong";
    case INTERLEAVED_HASH_MAP:
      return "Interleaved Hash Map Ping-Pong";
  }
  return NULL;
}

uint64 PingPong::HandleDatabaseSize() {
  unsigned int value_size = 0;
  Value* sample_value;
  IPPRow* sample_row;
  switch (level_) {
    case NONE:
    case INTERLEAVED:
      value_size = RetrieveValue(Key("BOGUS"), APP_STATE, 0)->size();
      break;

    case HASH_MAP:
      if ((sample_value = application_hash_map_.SampleValue()))
        value_size = sample_value->size();
      break;

    case INTERLEAVED_HASH_MAP:
      if ((sample_row = interleaved_hash_map_.SampleValue()))
        value_size = sample_row->app_value->size();
      break;
  }

  return (array_size_ / 4) + (3 * array_size_ * value_size);
}

Value* PingPong::HandleGet(const Key& key, VersionID version) {
  return RetrieveValue(key, APP_STATE);
}

bool PingPong::HandlePut(const Key& key, Value* value, VersionID version) {
  // Update our version of the DB as specified in Ping-Pong algorithm
  SetValue(key, value, APP_STATE);
  if (odd_is_being_updated_) {
    SetValue(key, value, ODD);
    SetDirtyBit(key, ODD, true);
  } else {
    SetValue(key, value, EVEN);
    SetDirtyBit(key, EVEN, true);
  }

  // Prevent anything from happening in the database
  return false;
}

bool PingPong::PrepareForCheckpoint(VersionID stable) {
  // Use this as a point of consistency
  if (!checkpoint_lock_.TryReadLock())
    return false;
  Storage::GetStorageInstance()->Freeze();

  // Update the total amount written to the DB
  switch (level_) {
    case NONE:
    case INTERLEAVED:
      c_inserts_ = total_inserts_;
      break;

    case HASH_MAP:
      c_inserts_ = application_hash_map_.Size();
      break;

    case INTERLEAVED_HASH_MAP:
      c_inserts_ = interleaved_hash_map_.Size();
      break;
  }

  // Flip odd and even, clean up
  odd_is_being_updated_ = !odd_is_being_updated_;
  Storage::GetStorageInstance()->Unfreeze();
  checkpoint_lock_.Unlock();
  return true;
}

static inline void* RunPingPongCheckpointer(void* ping_pong) {
  (reinterpret_cast<PingPong*>(ping_pong))->BackgroundedCheckpointer();
  return NULL;
}

bool PingPong::CaptureCheckpoint() {
  return !pthread_create(&checkpointing_daemon_, NULL,
                         &RunPingPongCheckpointer, this);
}

void PingPong::BackgroundedCheckpointer() {
  // Get a checkpointer lock to avoid wierd state
  checkpoint_lock_.WriteLock();
  double checkpoint_start_time = GetTime();

  // Get previous checkpoint if it exists to do a partial checkpoint
  FILE* prev_checkpoint = NULL;
  vector<pair<int, string> > files;
  if (GetFileListing(CHKPNTDIR, "chkpnt", &files))
    prev_checkpoint
      = fopen((CHKPNTDIR + ("/" + (files.end() - 1)->second)).c_str(), "r");

  // Once the database has been frozen, we sequentially traverse the elements
  char log_name[200];
  snprintf(log_name, sizeof(log_name), "%s/%ld.chkpnt", CHKPNTDIR, time(NULL));
  FILE* checkpoint = fopen(log_name, "w");
  if (!checkpoint) {
    if (prev_checkpoint)
      fclose(prev_checkpoint);
    checkpoint_lock_.Unlock();
    return;
  }

  // Are we writing out the odd or even array?
  ValueLocation current_location = (odd_is_being_updated_ ? EVEN : ODD);

  // Fuck switches, switches get stitches
  vector<Key>* keys;
  double total_out = 0.0;
  char buffer[4096];
  unsigned int last_chkpnt_pos = 0;
  unsigned int last_val_length = 0;

  // Iterate through all of the objects and write the stable version to disk
  switch (level_) {
    // Write <array_pos|len_value_bytes|value> to disk (array-based)
    case NONE:
    case INTERLEAVED:
      for (unsigned int i = 0; i < array_size_; i++) {
        // Get to the write (get it! like "right"?) spot in the last chkpnt
        if (prev_checkpoint != NULL && (i == 0 || i > last_chkpnt_pos)
            && fread(buffer, sizeof(buffer[0]), 4, prev_checkpoint) == 4) {
          last_chkpnt_pos = ByteArrayToUnsignedInt(buffer);

          if (ReadFromFile(prev_checkpoint, buffer, 4) != 4) {
            fclose(checkpoint);
            if (prev_checkpoint)
              fclose(prev_checkpoint);
            checkpoint_lock_.Unlock();
            return;
          }
          last_val_length = ByteArrayToUnsignedInt(buffer);
        }

        // Do the bit comparison w/old checkpoint to avoid wasted time
        if (!Dirty(Key("BOGUS"), current_location, i)) {
          if (prev_checkpoint != NULL && last_chkpnt_pos == i) {
            fread(buffer, sizeof(buffer[0]), last_val_length, prev_checkpoint);
            PrintToFile(checkpoint, i, last_val_length, buffer);
          }
          continue;
        } else if (prev_checkpoint != NULL && last_chkpnt_pos == i) {
          fseek(prev_checkpoint, last_val_length, SEEK_CUR);
        }

        // Reset dirty bit, then read in the stable value
        SetDirtyBit(Key("BOGUS"), current_location, false, i);
        Value* value = RetrieveValue(Key("BOGUS"), current_location, i);
        if (!value)
          continue;

        // Write <array_key|len_value_bytes|value> to disk
        total_out++;
        PrintToFile(checkpoint, i, value->size(), value->data());
      }
      break;

    // Write <len_key_bytes|key|len_value_bytes|value> to disk (hash-based)
    case HASH_MAP:
    case INTERLEAVED_HASH_MAP:
      keys = (level_ == HASH_MAP ? application_hash_map_.Keys()
                                 : interleaved_hash_map_.Keys());
      for (vector<Key>::iterator it = keys->begin(); it != keys->end(); it++) {
        // Do the bit comparison to avoid wasted time
        if (!Dirty(*it, current_location))
          continue;

        // Reset dirty bit, then read in the stable value
        SetDirtyBit(*it, current_location, false);
        Value* val = RetrieveValue(*it, current_location);
        if (!val)
          continue;

        // Write <len_key_bytes|key|len_value_bytes|value> to disk
        total_out++;
        PrintToFile(checkpoint, it->length(), val->size(), val->data(), *it);
      }
      break;
  }

  // Report to user
  if (c_inserts_ > 0)
    LOG(INFO) << "Wrote chkpnt (" << (100 * total_out / c_inserts_) << "%%)";
  LOG(INFO) << (GetTime() - GlobalStartTime())
            << "Finished capturing checkpoint ("
            << (GetTime() - checkpoint_start_time) << "s total)";

  // Close the file and release the freeze on the database
  fclose(checkpoint);
  if (prev_checkpoint)
    fclose(prev_checkpoint);
  checkpoint_lock_.Unlock();

  return;
}

bool PingPong::RecoverFromCheckpoint(string filename, VersionID version) {
  // First, we open the file for writing
  FILE* checkpoint = fopen(filename.c_str(), "r");
  if (!checkpoint)
    return false;

  // Prevent other data layer access
  Storage* storage = Storage::GetStorageInstance();

  char inbuf[4096];
  switch (level_) {
    // Retrieve in the following format <intkey|vallen|val> (array-based)
    case NONE:
    case INTERLEAVED:
      while (ReadFromFile(checkpoint, inbuf, 4) == 4) {
        int array_key = ByteArrayToUnsignedInt(inbuf);

        if (ReadFromFile(checkpoint, inbuf, 4) != 4)
          return false;
        int val_len = ByteArrayToUnsignedInt(inbuf);
        if (ReadFromFile(checkpoint, inbuf, val_len) != val_len)
          return false;
        Value* value = new Value(CreatePermBuffer(inbuf, val_len), val_len);

        SetValue(Key("bogus"), value, APP_STATE, array_key);
      }
      break;

    // Retrieve in the following format <keylen|key|vallen|val> (hash-based)
    case HASH_MAP:
    case INTERLEAVED_HASH_MAP:
      while (ReadFromFile(checkpoint, inbuf, 4) == 4) {
        int key_len = ByteArrayToInt(inbuf);
        if (ReadFromFile(checkpoint, inbuf, key_len)  != key_len)
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
      break;
  }

  // Everything went well
  LOG(INFO) << "Recovered in " << (GetTime() - GlobalStartTime()) << "s";
  return true;
}

}  // namespace calvin
