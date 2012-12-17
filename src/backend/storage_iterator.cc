/// @file
/// @author Alexander Thomson <thomson@cs.yale.edu>
/// @version 0.1
/// @since 5 May 2012
///
/// @section DESCRIPTION
///
/// This version of the storage iterator uses only methods exposed by the
/// default Storage implementation. It may be very slow.

#include <cstdio>
#include "common/utils.h"
#include "backend/storage_iterator.h"

namespace calvin {

StorageIterator::StorageIterator(Storage* storage, VersionID version)
    : storage_(storage), version_(version), before_first_(true),
      after_last_(false) {
}

bool StorageIterator::Valid() {
  return (!before_first_ && !after_last_);
}

void StorageIterator::Next() {
  const Key* next;
  if (before_first_) {
    before_first_ = false;
    next = storage_->FirstKey(version_);
  } else {
    next = storage_->GetNextKey(key_, version_);
  }
  if (next == NULL) {
    // No key found. Set done; no need to update key_.
    after_last_ = true;
  } else {
    // Set key to next key found.
    key_ = *next;
  }
}

void StorageIterator::Prev() {
  const Key* next;
  if (after_last_) {
    after_last_ = false;
    next = storage_->LastKey(version_);
  } else {
    next = storage_->GetPrevKey(key_, version_);
  }
  if (next == NULL) {
    // No key found. Set done; no need to update key_.
    before_first_ = true;
  } else {
    // Set key to next key found.
    key_ = *next;
  }
}

const Key& StorageIterator::key() {
  return key_;
}

Value* StorageIterator::value() {
  return storage_->Get(key_, version_);
}

void StorageIterator::Seek(const Key& target) {
  key_ = target;
  if (!storage_->KeyExists(key_, version_))
    Next();
}

void StorageIterator::SeekToFirst() {
  const Key* first = storage_->FirstKey(version_);
  if (first == NULL) {
    // Storage is empty. Go to after_last_, since that is the position
    // immediately following before_first_.
    before_first_ = false;
    after_last_ = true;
  } else {
    // Found the first key!
    key_ = *first;
    before_first_ = false;
    after_last_ = false;
  }
}

void StorageIterator::SeekToLast() {
  const Key* last = storage_->LastKey(version_);
  if (last == NULL) {
    // Storage is empty. Go to before_first_, since that is the position
    // immediately preceding after_last_.
    before_first_ = true;
    after_last_ = false;
  } else {
    // Found the last key!
    key_ = *last;
    before_first_ = false;
    after_last_ = false;
  }
}

}  // namespace calvin
