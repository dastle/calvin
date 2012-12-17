/// @file
/// @author Thaddeus Diamond <diamond@cs.yale.edu>
/// @version 0.1
/// @since 26 Jan 2012
///
/// @section DESCRIPTION
///
/// Test suite for singleton backend

#include "backend/leveldb_storage.h"
#include <gtest/gtest.h>
#include <glog/logging.h>
#include <string>

namespace calvin {

class LevelDBStorageTest : public testing::Test {
 protected:
  LevelDBStorageTest() {
    storage_ = new LevelDBStorage();
  }

  virtual ~LevelDBStorageTest() {
    delete storage_;
  }

  LevelDBStorage* storage_;
};

/// @test   First, we test simple Get's, Put's and Delete's
TEST_F(LevelDBStorageTest, SimpleTest) {
  Key key("key");
  Value val("value");
  EXPECT_TRUE(storage_->Put(key, &val));

  Value* result = storage_->Get(key);
  EXPECT_EQ(val.ToString(), result->ToString());
}

}  // namespace calvin

int main(int argc, char **argv) {
  google::InitGoogleLogging(argv[0]);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
