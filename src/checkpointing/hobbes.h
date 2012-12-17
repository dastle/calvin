/// @file
/// @author Thaddeus Diamond <diamond@cs.yale.edu>
/// @version 0.1
/// @since 26 Jan 2012
///
/// @section DESCRIPTION
///
/// This file describes an extension to storage, our asynchronous method
/// of checkpointing being presented at VLDB.

#ifndef _DB_CHECKPOINTING_HOBBES_H_
#define _DB_CHECKPOINTING_HOBBES_H_

#include <gtest/gtest.h>
#include <fcntl.h>
#include <pthread.h>
#include <cstdio>
#include <string>
#include <vector>
#include "backend/storage.h"
#include "backend/storage_decorator.h"
#include "checkpointing/checkpointer.h"
#include "common/configuration.h"
#include "common/utils.h"

namespace calvin {

using std::string;
using std::vector;

/// @class Hobbes
/// @brief The custom version of checkpointing being implemented for VLDB '12
///
/// The Hobbes class is responsible for capturing a checkpoint and writing it
/// to file asynchronously.  It also intercepts reads and writes in order
/// to keep one stable and one unstable version of each database row.
class Hobbes : public Checkpointer {
 public:
  /// @enum  HashType
  /// @brief The different hashes available to improve Hobbes
  ///
  /// We provide a set of hash types that can be passed to the constructor
  /// in order to make a fast hash detecting Put()'s in the database and
  /// avoiding duped writes.
  enum HashType {
    NONE = 0,
    DICTIONARY_HASH,
    BLOOM_FILTER,
  };

  /// A constructor can be passed another decorator, following the decorator DP
  /// in order to recursively call it when handling gets and puts.
  ///
  /// @param    filter      Which fast hash to apply if any
  /// @param    decorator   The StorageDecorator to adorn this object with
  explicit Hobbes(HashType filter = NONE)
    : stable_(-1 * VERSION_ID_MAX),
      filter_type_(filter), filter_stable_(NULL), filter_unstable_(NULL) {
    if (filter == BLOOM_FILTER) {
      filter_length_ = 2 * TOTAL_RECORDS / 8 + 1;
      filter_stable_ = new char[filter_length_];
      filter_unstable_ = new char[filter_length_];
    } else if (filter == DICTIONARY_HASH) {
      filter_length_ = TOTAL_RECORDS;
      filter_stable_ = new char[filter_length_];
      filter_unstable_ = new char[filter_length_];
    }
  }

  /// Hobbes frees only it's decorator if it was given one
  virtual ~Hobbes() {
    if (filter_stable_ != NULL)
      delete[] filter_stable_;
    if (filter_unstable_ != NULL)
      delete[] filter_unstable_;
  }

  virtual const char* type();
  virtual const char* name();
  virtual uint64    HandleDatabaseSize();
  virtual Value*    HandleGet(const Key& key, VersionID version);
  virtual bool      HandlePut(const Key& key, Value* value, VersionID version);
  virtual bool      PrepareForCheckpoint(VersionID capture_time);
  virtual bool      CaptureCheckpoint();
  virtual void      BackgroundedCheckpointer();
  virtual bool      RecoverFromCheckpoint(string filename, VersionID version);

 private:
  /// We maintain the stable version which we are about to checkpoint
  VersionID stable_;

  /// Keep track of which type of filter we're using
  HashType filter_type_;

  /// We run the actual checkpoint in a separate thread
  pthread_t checkpointing_daemon_;

  /// For convenience when zero'ing, we keep the length of each filter array
  int filter_length_;

  /// If we're using the bloom filter optimization, then we maintain a byte
  /// array that is shifted and later translated into a bit array for faster I/O
  /// The filter size depends on which HashType was passed to constructor.
  char* filter_stable_;

  /// We maintain an additional bloom filter since we always keep access to
  /// two versions of every data piece
  char* filter_unstable_;

  /// We need to acquire locks in order to change the pointers between stable
  /// and unstable without causing race conditions
  MutexRW filter_lock_;

  /// We want only one checkpoint to proceed at a time
  MutexRW checkpoint_lock_;

  /// We keep track a priori how many inserts we've made at the time of
  /// checkpointing
  int c_inserts_;

  /// For granular filter checking, we make a couple tests friend methods
  FRIEND_TEST(HobbesTest, BloomFilterTest);
  FRIEND_TEST(HobbesTest, DictionaryHashTest);
};

}  // namespace calvin

#endif  // _DB_CHECKPOINTING_HOBBES_H_
