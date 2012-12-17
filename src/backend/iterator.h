/// @file
/// @author Alexander Thomson <thomson@cs.yale.edu>
/// @version 0.1
/// @since 5 May 2012
///
/// @section DESCRIPTION
///
/// General iterator interface for Calvin. Based on LevelDB's iterator.h.

#ifndef _DB_BACKEND_ITERATOR_H_
#define _DB_BACKEND_ITERATOR_H_

#include <vector>

#include "common/types.h"
#include "backend/value.h"

namespace calvin {

class Iterator {
 public:
  // Virtual destructor
  virtual ~Iterator() {}

  // An iterator is either positioned at a key/value pair, or
  // not valid.  This method returns true iff the iterator is valid.
  virtual bool Valid() = 0;

  /// Returns a const ref to the current key.
  ///
  /// Requires: !done().
  /// Requires: Next() or Advance() must have been called already.
  virtual const Key& key() = 0;

  /// Returns a (const) pointer to the current value.
  ///
  /// Requires: !done().
  /// Requires: Next() or Advance() must have been called already.
  virtual Value* value() = 0;

  /// Moves to the next entry in the source.  After this call, Valid() is
  /// true iff the iterator was not positioned at the last entry in the source.
  /// REQUIRES: Valid()
  virtual void Next() = 0;

  /// Moves to the previous entry in the source.  After this call, Valid() is
  /// true iff the iterator was not positioned at the first entry in source.
  /// REQUIRES: Valid()
  virtual void Prev() = 0;

  /// Position at the first key in the source that at or past 'target'.
  /// The iterator is Valid() after this call iff the source contains
  /// an entry that comes at or past target.
  virtual void Seek(const Key& target) = 0;

  /// Position at the first key in storage. The iterator is Valid() after this
  /// call iff storage is not empty.
  virtual void SeekToFirst() = 0;

  /// Position at the final key in storage. The iterator is Valid() after this
  /// call iff storage is not empty.
  virtual void SeekToLast() = 0;
};

}  // namespace calvin

#endif  // _DB_BACKEND_ITERATOR_H_

