/// @file
/// @author Alexander Thomson <thomson@cs.yale.edu>
/// @version 0.1
/// @since 9 April 2012
///
/// @section DESCRIPTION
///
/// Storage layer interface connected to LevelDB as a backend

#ifndef _DB_BACKEND_LEVELDB_STORAGE_H_
#define _DB_BACKEND_LEVELDB_STORAGE_H_

#include <leveldb/db.h>
#include <string>
#include <cstring>
#include <vector>

#include "common/types.h"
#include "backend/value.h"
#include "backend/iterator.h"
#include "backend/storage.h"
#include "backend/storage_decorator.h"
#include "common/atomic.h"
#include "common/mutex.h"

namespace calvin {

using std::vector;
using std::string;

/// @class  LevelDBStorage
/// @brief  Storage layer connector to LevelDB
///
/// @attention
/// Storage uses the Singleton design pattern to ensure there is only one
/// instance of it ever.
class LevelDBStorage : public Storage {
 public:
  LevelDBStorage();
  virtual ~LevelDBStorage();
  virtual bool KeyExists(const Key& key, VersionID version);
  virtual const Key* FirstKey(VersionID version);
  virtual const Key* LastKey(VersionID version);
  virtual vector<Key>* GetKeyList(VersionID version);
  virtual const Key* GetNextKey(const Key& target, VersionID version);
  virtual const Key* GetPrevKey(const Key& target, VersionID version);
  virtual bool Put(const Key& key, Value* value, VersionID version = 0);
  virtual Value* Get(const Key& key, VersionID version = 0);
  virtual bool Delete(const Key& key, VersionID version = 0);
  virtual Iterator* GetIterator(VersionID version = 0);

 private:
  friend class StorageIterator;

  // Pointer to LevelDB object
  leveldb::DB* db_;

  // LevelDB options
  leveldb::Options options_;

  // LevelDB status
  leveldb::Status status_;
};

}  // namespace calvin

#endif  // _DB_BACKEND_LEVELDB_STORAGE_H_

