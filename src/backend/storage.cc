/// @file
/// @author Alexander Thomson <thomson@cs.yale.edu>
/// @author Thaddeus Diamond <diamond@cs.yale.edu>
/// @version 0.1
/// @since 26 Nov 2011
///
/// @section DESCRIPTION
///
/// A simple implementation of the storage interface using an stl map

#include <cstdio>
#include "common/utils.h"
#include "backend/storage.h"
#include "backend/storage_iterator.h"

namespace calvin {

Storage* Storage::storage_instance_ = NULL;
Storage* Storage::GetStorageInstance() {
  if (storage_instance_ == NULL)
    storage_instance_ = new Storage();
  return storage_instance_;
}

void Storage::InitializeStorage(Checkpointer* decorator, const char* opts) {
  if (storage_instance_ != NULL) {
    LOG(WARNING) << "Attempted to reinitialize storage layer, ignoring call";
    return;
  }

  storage_instance_ = new Storage(decorator);
  LOG(INFO) << "Successfully spawned database";
}

bool Storage::DestroyStorage() {
  if (storage_instance_ == NULL)
    return false;

  delete storage_instance_;
  storage_instance_ = NULL;
  return true;
}

uint64 Storage::DatabaseSize() {
  unsigned int size;
  if (decorator_ != NULL && (size = decorator_->HandleDatabaseSize()))
    return size;

  return *database_size_;
}

Checkpointer* Storage::GetCheckpointer() {
  return decorator_;
}

bool Storage::KeyExists(const Key& key, VersionID version) {
  return Get(key, version) != NULL;
}

const Key* Storage::FirstKey(VersionID version) {
  const Key* first = objects_.FirstKey();
  while (first != NULL && !Get(*first, version))
    first = objects_.GetNextKey(*first);

  return first;
}

const Key* Storage::LastKey(VersionID version) {
  const Key* last = objects_.LastKey();
  while (last != NULL && !Get(*last, version))
    last = objects_.GetPrevKey(*last);

  return last;
}

vector<Key>* Storage::GetKeyList(VersionID version) {
  return objects_.Keys();
}

const Key* Storage::GetNextKey(const Key& target, VersionID version) {
  const Key* current = &target;
  do {
    current = objects_.GetNextKey(*current);
  } while (current != NULL && !Get(*current, version));

  return current;
}

const Key* Storage::GetPrevKey(const Key& target, VersionID version) {
  const Key* current = &target;
  do {
    current = objects_.GetPrevKey(*current);
  } while (current != NULL && !Get(*current, version));

  return current;
}

bool Storage::DupedValue(const Key& key, Value* value) {
  if (!objects_.Contains(key))
    return false;

  Row* haystack = objects_.Get(key);
  while (haystack) {
    if (haystack->value == value)
      return true;
    haystack = haystack->next;
  }

  return false;
}

bool Storage::PruneRow(const Key& key, VersionID version, bool multiple,
                       VersionID minimum, bool unsafe) {
  // Don't search through if the key has no versions
  if (!objects_.Contains(key))
    return false;

  // Search through the linked list for the value
  Row* fetching = objects_.Get(key);
  while (fetching && fetching->version > version)
    fetching = fetching->next;

  // No matching version, or this version is the only version
  if (!fetching || (!fetching->prev && !fetching->next && !unsafe))
    return false;

  while (fetching && fetching->version > minimum) {
    // Rearrange backward pointers
    Row* next = fetching->next;
    if (!fetching->prev && multiple && !unsafe)
      return false;
    else if (!fetching->prev)
      objects_.Set(key, fetching->next);
    else
      fetching->prev->next = fetching->next;

    // Rearrange forward pointers
    if (fetching->next)
      fetching->next->prev = fetching->prev;

    /// @bug: Causes memory problems (delete fetching->value;)
    if (fetching->value)
      database_size_ -= fetching->value->size();
    delete fetching;

    // If not multiple, stop after one
    if (!multiple)
      break;
    fetching = next;
  }

  return true;
}

bool Storage::Put(const Key& key, Value* value, VersionID version) {
  // Don't operate on any transactions while DB is frozen
  freeze_lock_.ReadLock();

  // Follow decorator DP paradigm
  if (decorator_ != NULL && !decorator_->HandlePut(key, value, version)) {
    freeze_lock_.Unlock();
    return false;
  }

  // Find a place to insert for specified version
  Row *current_row = (version < 0 ? NULL : objects_.Get(key)),
      *prev_row    = NULL;
  while (current_row && current_row->version > version) {
    prev_row = current_row;
    current_row = current_row->next;
  }

  // Overwrite an existing version to avoid duplicates w/same timestamp
  if (current_row && current_row->version == version) {
    current_row->value = value;
    freeze_lock_.Unlock();
    return true;
  }

  // Insert new row w/attributes for given timestamp
  Row* new_row = new Row();
  new_row->version = version;
  new_row->value = value;
  new_row->prev = prev_row;
  new_row->next = current_row;

  // Update size of database
  if (value)
    database_size_ += value->size();

  // Update surrounding linked list
  if (prev_row)
    prev_row->next = new_row;
  if (current_row)
    current_row->prev = new_row;

  // If no header or we're at the front, insert
  if (version < 0 || !objects_.Contains(key))
    objects_.Insert(key, new_row);
  else if (new_row->prev == NULL)
    objects_.Set(key, new_row);

  freeze_lock_.Unlock();
  return true;
}

Value* Storage::Get(const Key& key, VersionID version) {
  // Follow decorator DP paradigm
  Value* deco_result;
  if (decorator_ != NULL && (deco_result = decorator_->HandleGet(key, version)))
    return deco_result;

  // Only search if there's actual a chance you'll find it
  Row* current_row = objects_.Get(key);
  while (current_row && current_row->version >= version)
    current_row = current_row->next;

  // As long as we didn't go off the end, return the value
  if (current_row)
    return current_row->value;
  return NULL;
}

bool Storage::Delete(const Key& key, VersionID version) {
  // Follow decorator DP paradigm
  if (decorator_ != NULL && !decorator_->HandlePut(key, NULL, version))
    return false;

  // We're using a multi-versioned system so we just insert a NULL key
  Put(key, NULL, version);
  return true;
}

Iterator* Storage::GetIterator(VersionID version) {
  return new StorageIterator(this, version);
}

}  // namespace calvin
