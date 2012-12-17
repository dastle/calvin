/// @file
/// @author Alexander Thomson (thomson@cs.yale.edu)
/// @version 0.1
/// @since 5 April 2012
///
/// @section DESCRIPTION
///
/// A microbenchmark application that reads all elements of the read_set, does
/// some trivial computation, and writes to all elements of the write_set.

#ifndef _DB_STORED_PROCEDURES_MICROBENCHMARK_MICROBENCHMARK_H_
#define _DB_STORED_PROCEDURES_MICROBENCHMARK_MICROBENCHMARK_H_

#include <cstdio>
#include <set>
#include <string>

#include "stored_procedures/application.h"
#include "common/utils.h"
#include "common/configuration.h"
#include "proto/txn.pb.h"

namespace calvin {

using std::set;
using std::string;

class Microbenchmark : public Application {
 public:
  /// A la Singleton DP, we allow a manner in which the user can initialize
  /// the application as a TPCC application
  static void InitializeApplication(int nodecount, int hotcount, int RWSetSize,
                                    int keyhotcount);

  enum TxnType {
    MICROTXN_SP = 0,
    MICROTXN_MP = 1,
  };

  virtual const char* Name() const;
  virtual TxnProto* NewTxn(int64 txn_id, int txn_type, string args) const;
  virtual TxnStatus Execute(TxnProto* txn,
                            TransactionalStorageManager* storage) const;
  virtual void InitializeStorage() const;

  TxnProto* MicroTxnSP(int64 txn_id, int part) const;
  TxnProto* MicroTxnMP(int64 txn_id, int part1, int part2) const;

  int nparts;
  int hot_records;
  const int kRWSetSize;  // MUST BE EVEN
  const int kNumHotkeys;
  static const int kValueSize = 512;
  static const int kDBSize = 1000000;


  /// @TODO (Alex): Comment entire Microbenchmark class
  virtual int KeyToInt(const Key& key) const  { return atoi(key.c_str()); }
  virtual int DBSize() const                  { return kDBSize;           }

 private:
  Microbenchmark(int nodecount, int hotcount, int RWSetSize, int NumHotkeys)
    : Application(), nparts(nodecount), hot_records(hotcount),
      kRWSetSize(RWSetSize), kNumHotkeys(NumHotkeys) {}

  virtual ~Microbenchmark() {}

  void GetRandomKeys(set<int>* keys, int num_keys, int key_start,
                     int key_limit, int part) const;
};

}  // namespace calvin

#endif  // _DB_STORED_PROCEDURES_MICROBENCHMARK_MICROBENCHMARK_H_
