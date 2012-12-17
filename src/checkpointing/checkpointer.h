/// @file
/// @author Thaddeus Diamond <diamond@cs.yale.edu>
/// @version 0.1
/// @since 26 Jan 2012
///
/// @section DESCRIPTION
///
/// This file describes the interface for a generic checkpointer

#ifndef _DB_CHECKPOINTING_CHECKPOINTER_H_
#define _DB_CHECKPOINTING_CHECKPOINTER_H_

#include <glog/logging.h>
#include <tr1/unordered_map>
#include <string>
#include <utility>
#include <vector>
#include "common/utils.h"
#include "common/mutex.h"

#define CHKPNTDIR \
  calvin::Checkpointer::EnsureCheckpointDir("/tmp/tcd2_checkpoints_")

namespace calvin {

using std::vector;
using std::string;
using std::pair;
using std::tr1::unordered_map;

/// Only flush file buffers every 1MB (2048 writes)
static int NumWrites = 0;
static inline void* RunCollapser(void* checkpointer);

/// Utilities for creating checkpoint directories
static bool ChkpntDirCreated = false;
static char* ChkpntDir = NULL;

class Checkpointer {
 public:
  /// The destructor must free all aggregated memory (including decorators)
  virtual ~Checkpointer() {}

  /// When prompted, every checkpoint must return a label for itself
  ///
  /// @returns  A string representing the checkpoints name
  virtual const char* name() = 0;

  /// Every checkpointer must prepare for an upcoming checkpoint which they
  /// receive from the scheduler
  ///
  /// @param    capture_time    The transaction ID at which the checkpoint is
  ///                           to be captured
  ///
  /// @returns  True if the checkpoint can succeed, false o/w
  virtual bool PrepareForCheckpoint(VersionID capture_time) = 0;

  /// Every checkpointer must actually be capable of capturing checkpoints
  ///
  /// @returns  True if the checkpoint was successfully captured, false o/w
  virtual bool CaptureCheckpoint() = 0;

  /// Given a filename and the version being restored, any checkpointer must be
  /// able to issue historical puts and restore the database.
  ///
  /// @param    filename        The file to restore the database from
  /// @param    version         The version the file represents
  ///
  /// @returns  True if the recovery was successful, false o/w (i.e. bad format)
  virtual bool RecoverFromCheckpoint(string filename, VersionID version) = 0;

  /// In order to ensure that the checkpoint directory exists we have a utility
  /// to return the name of it and run mkdir on the desired directory
  ///
  /// @param    prefix      The prefix of the directory being created
  ///
  /// @returns  A pointer to a byte array containing the directory name
  static const char* EnsureCheckpointDir(const char* prefix) {
    if (!ChkpntDirCreated) {
      char host_name[32];
      gethostname(host_name, 32);

      ChkpntDir = new char[256];
      snprintf(ChkpntDir, sizeof(ChkpntDir), "%s%s", prefix, host_name);
      mkdir(ChkpntDir,
            S_IFDIR | S_IRWXU | S_IWGRP | S_IWOTH | S_IXGRP | S_IXOTH);

      ChkpntDirCreated = true;
    }

    return ChkpntDir;
  }

  /// This is a utility function for all checkpoint implementations to print to
  /// a file with (left this out of utility because it is checkpoint specific)
  /// and then flush the output buffer.
  ///
  /// @param    file            The file to be written to
  /// @param    key_or_key_len  The key (or key length) to be written as 4 bytes
  /// @param    value_size      How many bytes are in the void buffer
  /// @param    value           The void buffer to be written
  /// @param    keystr          (Optional) A key string to be written out
  void PrintToFile(FILE* file, int key_or_key_len, int value_size, void* value,
                   Key keystr = "") {
    fprintf(file, "%c%c%c%c",
            static_cast<char>(key_or_key_len >> 24),
            static_cast<char>(key_or_key_len >> 16),
            static_cast<char>(key_or_key_len >> 8),
            static_cast<char>(key_or_key_len));
    if (keystr.length() > 0)
      fprintf(file, "%s", keystr.c_str());
    fprintf(file, "%c%c%c%c",
            static_cast<char>(value_size >> 24),
            static_cast<char>(value_size >> 16),
            static_cast<char>(value_size >> 8),
            static_cast<char>(value_size));
    fwrite(value, sizeof(reinterpret_cast<char*>(value)[0]), value_size, file);
    if (NumWrites++ % 2048 == 0)
      fsync(fileno(file));
  }

  /// Collapse checkpoints in the background by spawning a thread.  See the
  /// BackgroundCollapser() method.
  ///
  /// @returns  True if the thread collapsed successfully, false o/w
  virtual bool CollapseCheckpoints() {
    if (!collapser_lock_.TryReadLock())
      return false;

    pthread_t checkpointing_daemon;
    pthread_create(&checkpointing_daemon, NULL, &RunCollapser, this);
    collapser_lock_.Unlock();
    return true;
  }

  /// Although not strictly necessary, every checkpointer should think about
  /// "collapsing checkpoints".  Because this varies based on the
  /// implementation (i.e. what is checkpointed), each checkpointer must do
  /// this on their own.
  ///
  /// @returns  The VersionID the database was successfully restored to
  ///           (note that this may be a timestamp based on whether or not the
  ///            checkpointer has an appropriate checkpoint -> txn_id converter)
  virtual VersionID BackgroundCollapser() {
    // Collapse only one set of checkpoints at a time
    collapser_lock_.WriteLock();
    double start_time = GetTime();

    // Get initial listing of files in CHKPNTDIR
    vector<pair<int, string> > files;
    if (!GetFileListing(CHKPNTDIR, "chkpnt", &files)) {
      LOG(ERROR) << (GetTime() - GlobalStartTime()) << "Error getting filelist";
      collapser_lock_.Unlock();
      return -1;
    }
    files.erase(files.end());
    if (files.size() < 2) {
      LOG(ERROR) << (GetTime() - GlobalStartTime()) << "Too few files";
      collapser_lock_.Unlock();
      return -1;
    }

    char chkpnt_name[200];
    snprintf(chkpnt_name, sizeof(chkpnt_name), "%s/%d.collapsed_chkpnt",
             CHKPNTDIR, (files.end() - 1)->first);
    if (fopen(chkpnt_name, "r")) {
      LOG(ERROR) << (GetTime() - GlobalStartTime()) << "Merged checkpnt exists";
      collapser_lock_.Unlock();
      return -1;
    }

    // Open a new total checkpoint
    FILE* total_chkpnt = fopen(chkpnt_name, "w");
    if (!total_chkpnt) {
      LOG(ERROR) << (GetTime() - GlobalStartTime()) << "Couldn't open new file";
      collapser_lock_.Unlock();
      return -1;
    }

    // Iterate backwards through all files available, merging if unseen
    unordered_map<Key, bool> key_chain;
    vector<pair<int, string> >::reverse_iterator r_it;
    for (r_it = files.rbegin(); r_it != files.rend(); r_it++) {
      FILE* cur_chkpnt = fopen((CHKPNTDIR + ("/" + r_it->second)).c_str(), "r");

      char inbuf[4096];
      while (ReadFromFile(cur_chkpnt, inbuf, 4) == 4) {
        int key_len = ByteArrayToInt(inbuf);
        if (ReadFromFile(cur_chkpnt, inbuf, key_len) != key_len) {
          collapser_lock_.Unlock();
          return -1;
        }
        Key key = string(inbuf);

        if (ReadFromFile(cur_chkpnt, inbuf, 4)  != 4) {
          collapser_lock_.Unlock();
          return -1;
        }
        int val_len = ByteArrayToInt(inbuf);

        if (key_chain.count(key) > 0) {
          fseek(cur_chkpnt, val_len, SEEK_CUR);
        } else {
          key_chain[key] = true;
          int digits;
          if ((digits = ReadFromFile(cur_chkpnt, inbuf, val_len)) != val_len) {
            collapser_lock_.Unlock();
            return -1;
          }
          PrintToFile(total_chkpnt, key_len, val_len, inbuf, key.c_str());
        }
      }
      fclose(cur_chkpnt);
      remove((CHKPNTDIR + ("/" + r_it->second)).c_str());
    }

    // Report to user
    LOG(INFO) << (GetTime() - GlobalStartTime())
              << "Collapsing of checkpoints done. ("
              << (GetTime() - start_time) << "s total)";

    // Clean up and return collapsed checkpoint's version
    fclose(total_chkpnt);
    collapser_lock_.Unlock();
    return (files.end() - 1)->first;
  }

  // Previously part of StorageDecorator.
  virtual uint64    HandleDatabaseSize() = 0;
  virtual Value*    HandleGet(const Key& key, VersionID version) = 0;
  virtual bool      HandlePut(const Key& key, Value* value, VersionID version) = 0;


 protected:
  /// We need to call a separate method from inside the spawned thread because
  /// we can't call member functions directly
  virtual void BackgroundedCheckpointer() = 0;

 private:
  /// Only one thread can collapse checkpoints at a time to prevent wierd
  /// file locking
  MutexRW collapser_lock_;
};

static inline void* RunCollapser(void* checkpointer) {
  (reinterpret_cast<Checkpointer*>(checkpointer))->BackgroundCollapser();
  return NULL;
}

}  // namespace calvin

#endif  // _DB_CHECKPOINTING_CHECKPOINTER_H_
