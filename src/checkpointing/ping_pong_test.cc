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

#define TEST_MODE

#include "checkpointing/ping_pong.h"
#include "common/black_box_factory.h"

namespace calvin {

using std::string;

class PingPongTest : public testing::Test {
 protected:
  PingPongTest() {
    ping_pong_ = new PingPong(10000);
    storage_ = BlackBoxFactory::StorageBlackBox(ping_pong_);
  }

  virtual ~PingPongTest() {}

  PingPong* ping_pong_;
  Storage* storage_;
};

/// @test    Ensure PingPong is handling GET requests from DB correctly
TEST_F(PingPongTest, HandlesGets) {
  storage_->Put(Key("w1"), new Value(string("value")), 0);
  EXPECT_EQ(string("value"), string(reinterpret_cast<char*>(
                                    storage_->Get(Key("w1"), 5)->data())));

  EXPECT_EQ(string("value"),
            string(reinterpret_cast<char*>(
                   ping_pong_->HandleGet(Key("w1"), 5)->data())));
  EXPECT_TRUE(ping_pong_->HandleGet(Key("w1"), 5) != NULL);
}

/// @test    Ensure PingPong is handling PUT requests from DB correctly
TEST_F(PingPongTest, HandlesPuts) {
  EXPECT_FALSE(ping_pong_->HandlePut(Key("w1"), new Value(string("val_1")), 0));
  EXPECT_EQ(string("val_1"),
            string(reinterpret_cast<char*>(ping_pong_->RetrieveValue(Key("w1"),
                                           PingPong::APP_STATE)->data())));
  EXPECT_EQ(string("val_1"),
            string(reinterpret_cast<char*>(ping_pong_->RetrieveValue(Key("w1"),
                                           PingPong::ODD)->data())));
  EXPECT_EQ(NULL, ping_pong_->RetrieveValue(Key("w1"), PingPong::EVEN));

  ping_pong_->PrepareForCheckpoint(5);
  EXPECT_FALSE(ping_pong_->HandlePut(Key("w1"), new Value(string("val_2")), 9));
  EXPECT_EQ(string("val_2"),
            string(reinterpret_cast<char*>(ping_pong_->RetrieveValue(Key("w1"),
                                           PingPong::APP_STATE)->data())));
  EXPECT_EQ(string("val_1"),
            string(reinterpret_cast<char*>(ping_pong_->RetrieveValue(Key("w1"),
                                           PingPong::ODD)->data())));
  EXPECT_EQ(string("val_2"),
            string(reinterpret_cast<char*>(ping_pong_->RetrieveValue(Key("w1"),
                                           PingPong::EVEN)->data())));
}

/// @test     Capture a checkpoint with Ping-Pong and ensure it works
TEST_F(PingPongTest, CapturesCheckpoints) {
  storage_->Put(Key("w1"), new Value(string("value")), 0);
  ping_pong_->PrepareForCheckpoint(5);
  EXPECT_TRUE(ping_pong_->CaptureCheckpoint());
  sleep(2);

  storage_->Put(Key("w2"), new Value(string("only_val_in_2nd_cp")), 10);
  ping_pong_->PrepareForCheckpoint(15);
  EXPECT_TRUE(ping_pong_->CaptureCheckpoint());
  sleep(2);
}

/// @test     Ensure that we can recover from a captured checkpoint
TEST_F(PingPongTest, RecoversFromCheckpoint) {
  storage_->Put(Key("w1"), new Value(string("value")), 0);
  ping_pong_->PrepareForCheckpoint(5);
  int version = time(NULL);
  ping_pong_->CaptureCheckpoint();
  sleep(2);

  // Reinitialize DB
  ping_pong_ = new PingPong(10000);
  storage_ = BlackBoxFactory::StorageBlackBox(ping_pong_);

  // Load checkpoint
  char checkpoint_path[100];
  snprintf(checkpoint_path, sizeof(checkpoint_path), "%s/%d.chkpnt",
           CHKPNTDIR, version);

  // Finally, do the damn thing
  EXPECT_TRUE(ping_pong_->RecoverFromCheckpoint(string(checkpoint_path),
                                                version));
  Value* result = storage_->Get(Key("w1"), 5);
  EXPECT_EQ(string("value"), string(reinterpret_cast<char*>(result->data())));
}

/// @test     This test ensures that multiple overflowing checkpoints collapse
TEST_F(PingPongTest, CollapsesCheckpoints) {
  ping_pong_->PrepareForCheckpoint(5);
  ping_pong_->CaptureCheckpoint();
  sleep(1);
  ping_pong_->PrepareForCheckpoint(5);
  ping_pong_->CaptureCheckpoint();
  sleep(1);

  EXPECT_EQ(ping_pong_->CollapseCheckpoints(), 1);
  sleep(1);
}

}  // namespace calvin

int main(int argc, char **argv) {
  google::InitGoogleLogging(argv[0]);
  system(("rm " + string(CHKPNTDIR) + "/* 2> /dev/null").c_str());
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

