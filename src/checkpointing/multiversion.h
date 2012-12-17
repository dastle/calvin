/// @file
/// @author Thaddeus Diamond <diamond@cs.yale.edu>
/// @version 0.1
/// @since 26 Jan 2012
///
/// @section DESCRIPTION
///
/// This file describes an extension to storage, our asynchronous method
/// of checkpointing being presented at VLDB.

#ifndef _DB_CHECKPOINTING_MULTIVERSION_H_
#define _DB_CHECKPOINTING_MULTIVERSION_H_

#include <fcntl.h>
#include <cstdio>
#include <vector>
#include <string>

#include "backend/storage.h"
#include "backend/storage_decorator.h"
#include "checkpointing/checkpointer.h"

namespace calvin {

using std::string;
using std::vector;

/// @class MultiversionCheckpointer
/// @brief Capturing snapshots from the multiversioned database
///
/// The MultiversionCheckpointer is a dummy checkpointer, which does nothing on
/// reads/writes/deletes, but is responsible for freezing the database and
/// writing out a raw checkpoint whenever it is prompted.
class MultiversionCheckpointer : public Checkpointer {
 public:
  /// A constructor can be passed another decorator, following the decorator DP
  /// in order to recursively call it when handling gets and puts.
  ///
  /// @param    decorator   The StorageDecorator to adorn this object with
  MultiversionCheckpointer() {}

  /// The MultiversionCheckpointer frees only it's decorator if it was given one
  virtual ~MultiversionCheckpointer() {}

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
  /// We want only one checkpoint to proceed at a time
  MutexRW checkpoint_lock_;

  /// We run the actual checkpoint in a separate thread
  pthread_t checkpointing_daemon_;
  
  /// The version that we run the SELECT * against
  VersionID historical_version_;
};

}  // namespace calvin

#endif  // _DB_CHECKPOINTING_MULTIVERSION_H_
