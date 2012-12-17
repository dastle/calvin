/// @file
/// @author Alexander Thomson <thomson@cs.yale.edu>
/// @version 0.1
/// @since 5 May 2012
///
/// @section DESCRIPTION
///
/// An iterator exposing sequential access to stored records.
///
/// Note: The below implementation is for an arbitrary storage engine.
///       We keep track of the key and look it up in storage at each step.
///       This may be VERY SLOW. An implementation using level_db's native
///       snapshot/iterators should be much faster.

#ifndef _DB_BACKEND_STORAGE_ITERATOR_H_
#define _DB_BACKEND_STORAGE_ITERATOR_H_

#include <vector>

#include "common/types.h"
#include "backend/value.h"
#include "backend/storage.h"
#include "backend/iterator.h"

using std::vector;

namespace calvin {

/// @class  StorageIterator
/// Exposes an iterator over a set of records. Note that this is for READING
/// records only; updates are not supported.
///
/// Example usage: Print out first three (Key, Value) pairs in storage:
///   // Create an iterator to get a view of the database at version 101.
///   StorageIterator* iter = GetIterator(101);
///   // The Iterator starts out positioned BEFORE the first key, so
///   if (!iter->done()
///     printf("(%s, %s)\n", iter->key(), iter->value());
///
/// Example usage: Print out all (Key, Value) pairs with keys in the range
///                ["alpha", "bravo") at timestamp 101.
///
///   // Create an iterator to get a view of the database at version 101.
///   StorageIterator* iter = GetIterator(101);
///   iter->SetLimit("bravo"); // Set upper bound (ignore all keys >= "bravo")
///   iter->Advance("alpha");  // Point the iterator at first key >= "alpha"
///   // Print out all records in range.
///   while (!iter->done()) {
///     printf("(%s, %s)\n", iter->key(), iter->value());
///     iter->Next();
///   }
///
/// An alternative way to create the same iterator (replaces the calls to the
/// constructor, SetLimit, and Advance in the above example):
///
///   StorageIterator* iter = GetIterator(101, "bravo", "alpha");
///
/// Note that in this case, the iterator starts out positioned AT (not before)
/// the first record in the specified key range.
///
class StorageIterator : public Iterator {
 public:
  // An iterator is either positioned at a key/value pair, or
  // not valid.  This method returns true iff the iterator is valid.
  virtual bool Valid();

  /// Returns a const ref to the current key.
  ///
  /// Requires: !done().
  /// Requires: Next() or Advance() must have been called already.
  virtual const Key& key();

  /// Returns a (const) pointer to the current value.
  ///
  /// Requires: !done().
  /// Requires: Next() or Advance() must have been called already.
  virtual Value* value();

  /// Moves to the next entry in the source.  After this call, Valid() is
  /// true iff the iterator was not positioned at the last entry in the source.
  /// REQUIRES: Valid()
  virtual void Next();

  /// Moves to the previous entry in the source.  After this call, Valid() is
  /// true iff the iterator was not positioned at the first entry in source.
  /// REQUIRES: Valid()
  virtual void Prev();

  /// Position at the first key in the source that at or past 'target'.
  /// The iterator is Valid() after this call iff the source contains
  /// an entry that comes at or past target.
  virtual void Seek(const Key& target);

  /// Position at the first key in storage. The iterator is Valid() after this
  /// call iff storage is not empty.
  virtual void SeekToFirst();

  /// Position at the final key in storage. The iterator is Valid() after this
  /// call iff storage is not empty.
  virtual void SeekToLast();

 private:
  friend class Storage;
  /// Constructor is private so that Iterator can only be obtained through
  /// Storage::GetIterator method.
  StorageIterator(Storage* storage, VersionID version);

  /// Pointer to storage object over which we are iterating.
  Storage* storage_;

  /// Latest version of records to expose.
  VersionID version_;

  /// True if iterator is currently positioned before the first element.
  bool before_first_;

  /// True if iterator is currently positioned after the last element.
  bool after_last_;

  /// Current key.
  Key key_;
};

}  // namespace calvin

#endif  // _DB_BACKEND_STORAGE_ITERATOR_H_

