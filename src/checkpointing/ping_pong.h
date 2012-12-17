/// @file
/// @author Thaddeus Diamond <diamond@cs.yale.edu>
/// @version 0.1
/// @since 26 Jan 2012
///
/// @section DESCRIPTION
///
/// This file describes an extension to storage, our asynchronous method
/// of checkpointing being presented at VLDB.

#ifndef _DB_CHECKPOINTING_PING_PONG_H_
#define _DB_CHECKPOINTING_PING_PONG_H_

#include <proto/tpcc_structs.h>
#include <fcntl.h>
#include <gtest/gtest.h>
#include <utility>
#include <vector>
#include <cstdio>
#include <string>

#include "stored_procedures/application.h"
#include "backend/storage.h"
#include "backend/storage_decorator.h"
#include "checkpointing/checkpointer.h"
#include "common/configuration.h"
#include "common/atomic.h"

class Application;

namespace calvin {

using std::string;
using std::vector;
using std::pair;

class PingPong : public Checkpointer {
 public:
  /// @enum  SpecLevel
  /// @brief The different specification levels for Ping-Pong
  ///
  /// We provide a set of spec levels that can be passed to the constructor
  /// in order to evolve different Ping-Pong characteristics.
  enum SpecLevel {
    NONE = 0,
    INTERLEAVED,
    HASH_MAP,
    INTERLEAVED_HASH_MAP,
  };

  /// A constructor can be passed another decorator, following the decorator DP
  /// in order to recursively call it when handling gets and puts.
  ///
  /// @param    array_size    How big the Ping-Pong array (DB) should be
  /// @param    level         What type of Ping-Pong (\ref SpecLevel) this is
  /// @param    decorator     The StorageDecorator to adorn this object with
  explicit PingPong(unsigned int array_size, SpecLevel level = NONE)
    : odd_is_being_updated_(true), level_(level),
      total_inserts_(0), array_size_(array_size) {
    switch (level) {
      case NONE:
        application_state_ = new Value*[array_size_];
        odd_  = new OddEvenRow[array_size_];
        even_ = new OddEvenRow[array_size_];

        for (unsigned int i = 0; i < array_size_; i++) {
          application_state_[i] = NULL;
          odd_[i].dirty  = false;
          odd_[i].value  = NULL;
          even_[i].dirty = false;
          even_[i].value = NULL;
        }

      case INTERLEAVED:
        inter_db_ = new IPPRow[array_size_];
        for (unsigned int i = 0; i < array_size_; i++) {
          inter_db_[i].app_value  = NULL;
          inter_db_[i].odd_dirty  = false;
          inter_db_[i].odd_value  = NULL;
          inter_db_[i].even_dirty = false;
          inter_db_[i].even_value = NULL;
        }
        break;

      case HASH_MAP:
      case INTERLEAVED_HASH_MAP:
        break;
    }
  }

  /// PingPong frees its decorator if it was given one and all app data
  virtual ~PingPong() {
    switch (level_) {
      case INTERLEAVED:
        delete[] inter_db_;

      case NONE:
        delete[] application_state_;
        delete[] odd_;
        delete[] even_;
        break;

      case HASH_MAP:
      case INTERLEAVED_HASH_MAP:
        break;
    }
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

  /// @enum  ValueLocation
  /// @brief Which of the three arrays the item we are seeking is
  ///
  /// We create an enum in order to access values fast in either situation
  enum ValueLocation {
    APP_STATE = 0,
    ODD,
    EVEN
  };

 private:
  /// We run the actual checkpoint in a separate thread
  pthread_t checkpointing_daemon_;

  /// We provide a convenient way to access a specific value pointer in any
  /// of the three possible memory spaces...
  ///
  /// @param    key           The key whose value is to be retrieved
  /// @param    location      The location (AS, Odd, Even) being accessed
  /// @param    key_override  Optional key to override the array access w/
  ///
  /// @returns  A pointer to the value found at that location
  Value* RetrieveValue(const Key& key, ValueLocation location,
                       int key_override = -1);
  FRIEND_TEST(PingPongTest, HandlesPuts);

  /// ... And then a way to set it
  ///
  /// @param    key           The key whose value is to be retrieved
  /// @param    value         The value being overwritten
  /// @param    location      The location (AS, Odd, Even) being accessed
  /// @param    key_override  Optional key to override the array access w/
  ///
  /// @returns  A pointer to the value found at that location
  ///
  /// @attention  It is not a bug here that we do not free memory here.  We are
  ///             dealing with pointer writes that are extremely reused.  Due to
  ///             the relatively low cost of memory leaks but high cost of
  ///             correctly managing memory (harder in this case than expected)
  ///             we avoid the situation altogether.
  void SetValue(const Key& key, Value* value, ValueLocation location,
                int key_override = -1);

  /// We provide a simple way to check if a key is dirty or not...
  ///
  /// @param    key           The key checking if it's dirty
  /// @param    location      The location (AS, Odd, Even) being accessed
  /// @param    key_override  Optional key to override the array access w/
  ///
  /// @returns  True or false dependent on key's dirty bit
  bool Dirty(const Key& key, ValueLocation location, int key_override = -1);

  /// ... And then a way to set it
  ///
  /// @param    key           The key whose dirty bit is being cleared
  /// @param    location      The location (AS, Odd, Even) being accessed
  /// @param    value         The value to set the dirty bit to
  /// @param    key_override  Optional key to override the array access w/
  void SetDirtyBit(const Key& key, ValueLocation location, bool value,
                   int key_override = -1);

  /// Maintain whether odd is the volatile copy or not
  bool odd_is_being_updated_;

  /// What type of Ping-Pong is this?  We got OPTIONS BITCHES...
  SpecLevel level_;

  /// For IPP, we maintain Odd, Even, and hey, even the application state as
  /// one giant clusterfc*k
  typedef struct IPPRow {
    Value* app_value;
    bool   odd_dirty  : 1;        // Only one bit as they do in their paper
    Value* odd_value;
    bool   even_dirty : 1;
    Value* even_value;
  } IPPRow;

  /// Keep the entire application state as a giant array
  IPPRow* inter_db_;

  /// We represent the Odd/Even arrays' contents as unique structs
  typedef struct OddEvenRow {
    bool    dirty : 1;
    Value*  value;
  } OddEvenRow;

  /// Maintain the application state as an array of Value pointers...
  Value** application_state_;

  /// ... and keep odd and even separately when not interleaved...
  OddEvenRow* odd_;

  /// ... true dat
  OddEvenRow* even_;

  /// If we're using a hash map, keep a hash map...
  AtomicMap<Key, Value*> application_hash_map_;

  /// ... and, well then, do that...
  AtomicMap<Key, OddEvenRow*> odd_hash_map_;

  /// ... true dat
  AtomicMap<Key, OddEvenRow*> even_hash_map_;

  /// ...and if it's interleaved so be it
  AtomicMap<Key, IPPRow*> interleaved_hash_map_;

  /// We want only one checkpoint to proceed at a time
  MutexRW checkpoint_lock_;

  /// When capturing checkpoints, we output the percentage captured.  This is
  /// computed a priori to save time
  int total_inserts_;
  int c_inserts_;

  /// How long the array is to avoid segfaults in test
  unsigned int array_size_;
};

}  // namespace calvin

#endif  // _DB_CHECKPOINTING_PING_PONG_H_
