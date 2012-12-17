/// @file
/// @author Alexander Thomson (thomson@cs.yale.edu)
/// @version 0.1
/// @since 5 April 2012
///
/// @section DESCRIPTION
///
/// Microbenchmark implementation.

#include "stored_procedures/microbenchmark/microbenchmark.h"

namespace calvin {

const char* Microbenchmark::Name() const {
  return "Microbenchmark";
}

// Fills '*keys' with num_keys unique ints k where
// 'key_start' <= k < 'key_limit', and k == part (mod nparts).
// Requires: key_start % nparts == 0
void Microbenchmark::GetRandomKeys(set<int>* keys, int num_keys, int key_start,
                                   int key_limit, int part) const {
  assert(key_start % nparts == 0);
  keys->clear();
  for (int i = 0; i < num_keys; i++) {
    // Find a key not already in '*keys'.
    int key;
    do {
      key = key_start + part +
            nparts * (rand() % ((key_limit - key_start)/nparts));
    } while (keys->count(key));
    keys->insert(key);
  }
}

TxnProto* Microbenchmark::MicroTxnSP(int64 txn_id, int part) const {
  // Create the new transaction object
  TxnProto* txn = new TxnProto();

  // Set the transaction's standard attributes
  txn->set_txn_id(txn_id);
  txn->set_txn_type(MICROTXN_SP);

  // Add one hot key to read/write set if requested
  set<int> hot_keys;
  for (int i = 0; i < kNumHotkeys; i++) {
    int hotkey;
    do {
      hotkey = part + nparts * (rand() % hot_records);
    } while (hot_keys.count(hotkey));
    hot_keys.insert(hotkey);
  }

  // Insert set of kRWSetSize - 1 random cold keys from specified partition into
  // read/write set.
  set<int> keys;
  GetRandomKeys(&keys,
                kRWSetSize - kNumHotkeys,
                nparts * hot_records,
                nparts * kDBSize,
                part);

  // Add em to read/write set
  for (set<int>::iterator it = hot_keys.begin(); it != hot_keys.end(); ++it)
    txn->add_read_write_set(IntToString(*it));
  for (set<int>::iterator it = keys.begin(); it != keys.end(); ++it)
    txn->add_read_write_set(IntToString(*it));

  return txn;
}

TxnProto* Microbenchmark::MicroTxnMP(int64 txn_id, int part1, int part2) const {
  assert(part1 != part2 || nparts == 1);
  // Create the new transaction object
  TxnProto* txn = new TxnProto();

  // Set the transaction's standard attributes
  txn->set_txn_id(txn_id);
  txn->set_txn_type(MICROTXN_MP);

  // Add two hot keys to read/write set---one in each partition.
  int hotkey1 = part1 + nparts * (rand() % hot_records);
  int hotkey2 = part2 + nparts * (rand() % hot_records);
  txn->add_read_write_set(IntToString(hotkey1));
  txn->add_read_write_set(IntToString(hotkey2));

  // Insert set of kRWSetSize/2 - 1 random cold keys from each partition into
  // read/write set.
  set<int> keys;
  GetRandomKeys(&keys,
                kRWSetSize/2 - 1,
                nparts * hot_records,
                nparts * kDBSize,
                part1);
  for (set<int>::iterator it = keys.begin(); it != keys.end(); ++it)
    txn->add_read_write_set(IntToString(*it));
  GetRandomKeys(&keys,
                kRWSetSize/2 - 1,
                nparts * hot_records,
                nparts * kDBSize,
                part2);
  for (set<int>::iterator it = keys.begin(); it != keys.end(); ++it)
    txn->add_read_write_set(IntToString(*it));

  return txn;
}

// The load generator can be called externally to return a transaction proto
// containing a new type of transaction.
TxnProto* Microbenchmark::NewTxn(int64 txn_id, int txn_type,
                                 string args) const {
  return MicroTxnSP(txn_id, 0);
}

TxnStatus Microbenchmark::Execute(TxnProto* txn,
                            TransactionalStorageManager* storage) const {
  // Read all elements of 'txn->read_set()', add one to each, write them all
  // back out.
  for (int i = 0; i < txn->read_write_set_size(); i++) {
    char* val = storage->GetMutable<char>(txn->read_write_set(i), kValueSize);
    if (!val) {
      val = new char[kValueSize];
      for (int j = 0; j < kValueSize - 1; j++)
        val[j] = (rand() % 26 + 'a');
      val[kValueSize - 1] = '\0';
    }
    for (int j = rand() % (kValueSize - 10), k = j; j < k + 10; j++)
      val[j] = (rand() % 26 + 'a');
  }

  return SUCCESS;
}

}  // namespace calvin
