/// @file
/// @author Thaddeus Diamond <diamond@cs.yale.edu>
/// @version 0.1
/// @since 26 Jan 2012
///
/// @section DESCRIPTION
///
/// This file is a unit test for our asynchronous checkpointing mechanism

#include <gtest/gtest.h>
#include <glog/logging.h>
#include <string>
#include "checkpointing/naive.h"
#include "common/black_box_factory.h"

namespace calvin {

class NaiveCheckpointerTest : public testing::Test {
 protected:
  NaiveCheckpointerTest() {
    naive_ = new NaiveCheckpointer();
    storage_ = BlackBoxFactory::StorageBlackBox(naive_);
  }

  virtual ~NaiveCheckpointerTest() {}

  Storage* storage_;
  NaiveCheckpointer* naive_;
};

/// @test     Ensure NaiveCheckpointer handles GET requests from DB correctly
TEST_F(NaiveCheckpointerTest, HandlesGets) {
  EXPECT_EQ(NULL, naive_->HandleGet(Key("key"), 0));
}

/// @test     Ensure NaiveCheckpointer handles PUT requests from DB correctly
TEST_F(NaiveCheckpointerTest, HandlesPuts) {
  EXPECT_TRUE(naive_->HandlePut(Key("key"), new Value(string("value")), 0));
}

/// @test     Capture a checkpoint with naive checkpointer and ensure it works
TEST_F(NaiveCheckpointerTest, CapturesCheckpoints) {
  storage_->Put(Key("key"), new Value(string("value")), 0);
  EXPECT_TRUE(naive_->CaptureCheckpoint());
  storage_->Put(Key("key_2"), new Value(string("value_not_in_chkpnt")), 10);
  sleep(1);
}

/// @test     Ensure that we can recover from a captured checkpoint
TEST_F(NaiveCheckpointerTest, RecoversFromCheckpoint) {
  storage_->Put(Key("key"), new Value(string("value")), 0);
  int timestamp = time(NULL);
  naive_->CaptureCheckpoint();
  sleep(1);

  // Reinitialize DB
  naive_ = new NaiveCheckpointer();
  storage_ = BlackBoxFactory::StorageBlackBox(naive_);

  // Load checkpoint
  char checkpoint_path[100];
  snprintf(checkpoint_path, sizeof(checkpoint_path), "%s/%d.chkpnt",
           CHKPNTDIR, timestamp);

  EXPECT_TRUE(naive_->RecoverFromCheckpoint(string(checkpoint_path), 0));
  Value* result = storage_->Get(Key("key"), 5);
  EXPECT_EQ(string("value"), string(reinterpret_cast<char*>(result->data())));
}

/// @test     This test ensures that multiple overflowing checkpoints collapse
TEST_F(NaiveCheckpointerTest, CollapsesCheckpoints) {
  naive_->CaptureCheckpoint();
  sleep(1);
  naive_->CaptureCheckpoint();
  sleep(1);

  EXPECT_EQ(naive_->CollapseCheckpoints(), 1);
}

}  // namespace calvin

int main(int argc, char **argv) {
  google::InitGoogleLogging(argv[0]);
  system(("rm " + string(CHKPNTDIR) + "/* 2> /dev/null").c_str());
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

