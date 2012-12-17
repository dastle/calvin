/// @file
/// @author Thaddeus Diamond <diamond@cs.yale.edu>
/// @version 0.1
/// @since 26 Jan 2012
///
/// @section DESCRIPTION
///
/// Microbenchmark unit test suite

#include <gtest/gtest.h>
#include <set>
#include "stored_procedures/microbenchmark/microbenchmark.h"
#include "common/black_box_factory.h"

namespace calvin {

using std::set;

class MicrobenchmarkTest : public testing::Test {
 protected:
  MicrobenchmarkTest() {
    BlackBoxFactory::ConfigurationBlackBox();
    multiplexer_    = BlackBoxFactory::ConnectionMultiplexerBlackBox();
    storage_        = BlackBoxFactory::StorageBlackBox();
    connection_     = BlackBoxFactory::ConnectionBlackBox();
    Microbenchmark::InitializeApplication(
      Config::GetConfigInstance()->all_nodes.size(), 1000, 10, 1);
    microbenchmark_
      = reinterpret_cast<Microbenchmark*>(
          Application::GetApplicationInstance());
  }

  virtual ~MicrobenchmarkTest() {
    Storage::DestroyStorage();
    delete connection_;
    delete multiplexer_;
  }

  ConnectionMultiplexer* multiplexer_;
  Connection* connection_;
  Storage* storage_;
  Microbenchmark* microbenchmark_;
};

#define CHECK_OBJECT(KEY, EXPECTED_VALUE, VERSION) do {                   \
  Value* actual_value;                                                    \
  actual_value = storage_->Get(KEY, VERSION);                             \
  EXPECT_EQ(EXPECTED_VALUE,                                               \
            string(reinterpret_cast<char*>(actual_value->data())));       \
} while (0)

TEST_F(MicrobenchmarkTest, SinglePartitionTxn) {
  // Initialize storage.
  microbenchmark_->InitializeStorage();

  // Execute a 'MICROTXN_SP' txn.
  TxnProto* txn = microbenchmark_->MicroTxnSP(1, 0);
  txn->add_readers(0);
  txn->add_writers(0);

  TransactionalStorageManager* storage
    = BlackBoxFactory::TransactionalStorageManagerBlackBox(txn, false);
  microbenchmark_->Execute(txn, storage);

  // Check post-execution storage state.
  set<int> write_set;
  for (int i = 0; i < microbenchmark_->kRWSetSize; i++) {
    write_set.insert(StringToInt(txn->read_write_set(i)));
  }
  for (int i = 0; i < Microbenchmark::kDBSize; i++) {
    EXPECT_EQ(strlen(reinterpret_cast<char*>(
                storage_->Get(IntToString(i), 10)->data())),
              static_cast<unsigned int>(Microbenchmark::kValueSize - 1));
//    if (write_set.count(i))
//      CHECK_OBJECT(IntToString(i), IntToString(i+1), 10);
//    else
//      CHECK_OBJECT(IntToString(i), IntToString(i), 10);
  }

  delete storage;
  delete txn;
}

}  // namespace calvin

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
