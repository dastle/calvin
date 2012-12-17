/// @file
/// @author Alexander Thomson <thomson@cs.yale.edu>
/// @version 0.1
/// @since 9 April 2012
///
/// @section DESCRIPTION
///
/// LevelDBStorage implementation

#include "backend/leveldb_storage.h"

#include <leveldb/slice.h>

namespace calvin {

LevelDBStorage::LevelDBStorage() {
  options_.create_if_missing = true;
  status_ = leveldb::DB::Open(options_, "/tmp/testdb", &db_);
  assert(status_.ok());
};

LevelDBStorage::~LevelDBStorage() {
  delete db_;
}

/// @TODO(alex): implement this
bool LevelDBStorage::KeyExists(const Key& key, VersionID version) {
  return false;
}

/// @TODO(alex): implement this
const Key* LevelDBStorage::FirstKey(VersionID version) {
  return NULL;
}

/// @TODO(alex): implement this
const Key* LevelDBStorage::LastKey(VersionID version) {
  return NULL;
}

/// @TODO(alex): implement this
vector<Key>* LevelDBStorage::GetKeyList(VersionID version) {
  return NULL;
}

/// @TODO(alex): implement this
const Key* LevelDBStorage::GetNextKey(const Key& target, VersionID version) {
  return NULL;
}

/// @TODO(alex): implement this
const Key* LevelDBStorage::GetPrevKey(const Key& target, VersionID version) {
  return NULL;
}

/// @TODO(alex): implement versioning
bool LevelDBStorage::Put(const Key& key, Value* value, VersionID version) {
  leveldb::Slice k(key.data(), key.size());
  leveldb::Slice v(reinterpret_cast<char*>(value->data()), value->size());

  status_ = db_->Put(leveldb::WriteOptions(), k, v);
  return status_.ok();
}

/// @TODO(alex): implement versioning
/// @TODO(alex): less string copying
Value* LevelDBStorage::Get(const Key& key, VersionID version) {
  leveldb::Slice k(key.data(), key.size());
  string result;
  status_ = db_->Get(leveldb::ReadOptions(), k, &result);
  return new Value(result);
}

/// @TODO(alex): implement this
bool LevelDBStorage::Delete(const Key& key, VersionID version) {
  return false;
}

/// @TODO(alex): implement this
Iterator* LevelDBStorage::GetIterator(VersionID version) {
  return NULL;
}

}  // namespace calvin
