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
#include "stored_procedures/application.h"
#include "common/utils.h"
#include "common/black_box_factory.h"
#include "checkpointing/hobbes.h"

namespace calvin {

using std::string;

class HobbesTest : public testing::Test {
 protected:
  HobbesTest() {
    hobbes_  = new Hobbes();
    storage_ = BlackBoxFactory::StorageBlackBox(hobbes_);

    string_vals = new string[2];
    string_vals[0] = bytes("value_one");
    string_vals[1] = bytes("value_two");

    key = bytes("key");
    values = new Value*[8];
    for (int i = 0; i < 8; i++) {
      if (i % 2 == 0)
        values[i] = new Value(string_vals[0]);
      else
        values[i] = new Value(string_vals[1]);
    }
  }

  virtual ~HobbesTest() {}

  Storage* storage_;
  Hobbes* hobbes_;

  Key key;
  string* string_vals;
  Value **values, *result;
};

/// @test     Ensure Hobbes is handling GET requests from DB correctly
TEST_F(HobbesTest, HandlesGets) {
  EXPECT_EQ(NULL, hobbes_->HandleGet(Key("key"), 0));
}

/// @test     Ensure Hobbes is handling PUT requests from DB correctly
TEST_F(HobbesTest, HandlesPuts) {
  EXPECT_TRUE(hobbes_->HandlePut(Key("key"), new Value(string("value")), 0));
}

/// @test     Checks Hobbes' checkpointing capabilities
TEST_F(HobbesTest, CapturesCheckpoints) {
  storage_->Put(key, values[0], 10);
  hobbes_->PrepareForCheckpoint(15);
  storage_->Put(key, values[1], 20);
  EXPECT_TRUE(hobbes_->CaptureCheckpoint());
  sleep(1);

  char checkpoint_path[100];
  snprintf(checkpoint_path, sizeof(checkpoint_path), "%s/15.chkpnt",
           CHKPNTDIR);
  FILE* checkpoint = fopen(checkpoint_path, "r");
  EXPECT_TRUE(checkpoint != NULL);
  fclose(checkpoint);

  EXPECT_EQ(NULL, storage_->Get(key, 10));
  result = storage_->Get(key, 30);
  EXPECT_EQ(string_vals[1], string(reinterpret_cast<char*>(result->data())));
}

/// @test     Ensure Hobbes is pruning correctly
TEST_F(HobbesTest, CarefulPruningTest) {
  storage_->Put(key, values[0], 0);
  storage_->Put(key, values[1], 20);
  EXPECT_EQ(NULL, storage_->Get(key, 10));

  VersionID stable = 30;
  hobbes_->PrepareForCheckpoint(stable);
  storage_->Put(key, values[2], 40);

  storage_->Put(key, values[4], 15);
  result = storage_->Get(key, 16);
  EXPECT_EQ(string_vals[0], string(reinterpret_cast<char*>(result->data())));
  result = storage_->Get(key, stable);
  EXPECT_EQ(string_vals[1], string(reinterpret_cast<char*>(result->data())));

  storage_->Put(key, values[6], 25);
  result = storage_->Get(key, 30);
  EXPECT_EQ(string_vals[0], string(reinterpret_cast<char*>(result->data())));
  result = storage_->Get(key, stable);
  EXPECT_EQ(string_vals[0], string(reinterpret_cast<char*>(result->data())));

  storage_->Put(key, values[5], 35);
  result = storage_->Get(key, stable);
  EXPECT_EQ(string_vals[0], string(reinterpret_cast<char*>(result->data())));
  result = storage_->Get(key, 36);
  EXPECT_EQ(string_vals[1], string(reinterpret_cast<char*>(result->data())));
  result = storage_->Get(key, 41);
  EXPECT_EQ(string_vals[0], string(reinterpret_cast<char*>(result->data())));

  storage_->Put(key, values[7], 45);
  result = storage_->Get(key, stable);
  EXPECT_EQ(string_vals[0], string(reinterpret_cast<char*>(result->data())));
  result = storage_->Get(key, 40);
  EXPECT_EQ(string_vals[0], string(reinterpret_cast<char*>(result->data())));
  result = storage_->Get(key, 50);
  EXPECT_EQ(string_vals[1], string(reinterpret_cast<char*>(result->data())));

  EXPECT_TRUE(hobbes_->CaptureCheckpoint());
  sleep(1);
  EXPECT_EQ(NULL, storage_->Get(key, stable));
}

/// @test     The following test proves the functionality of our bloom filter
TEST_F(HobbesTest, BloomFilterTest) {
  hobbes_  = new Hobbes(Hobbes::BLOOM_FILTER);
  storage_ = BlackBoxFactory::StorageBlackBox(hobbes_);

  unsigned int loc_one      = FNVHash(key)    % hobbes_->filter_length_;
  unsigned int loc_one_rem  = FNVHash(key)    % 8;
  unsigned int loc_two      = FNVModHash(key) % hobbes_->filter_length_;
  unsigned int loc_two_rem  = FNVModHash(key) % 8;

  EXPECT_EQ(0, hobbes_->filter_stable_[loc_one]);
  EXPECT_EQ(0, hobbes_->filter_stable_[loc_two]);
  storage_->Put(key, values[1], 40);
  EXPECT_EQ((1 << loc_one_rem), hobbes_->filter_stable_[loc_one]);
  EXPECT_EQ((1 << loc_two_rem), hobbes_->filter_stable_[loc_two]);

  hobbes_->PrepareForCheckpoint(45);

  EXPECT_EQ(0, hobbes_->filter_unstable_[loc_one]);
  EXPECT_EQ(0, hobbes_->filter_unstable_[loc_two]);
  storage_->Put(key, values[0], 50);
  EXPECT_EQ((1 << loc_one_rem), hobbes_->filter_unstable_[loc_one]);
  EXPECT_EQ((1 << loc_two_rem), hobbes_->filter_unstable_[loc_two]);

  hobbes_->CaptureCheckpoint();
  sleep(1);

  EXPECT_EQ(0, hobbes_->filter_unstable_[loc_one]);
  EXPECT_EQ(0, hobbes_->filter_unstable_[loc_two]);
  EXPECT_EQ((1 << loc_one_rem), hobbes_->filter_stable_[loc_one]);
  EXPECT_EQ((1 << loc_two_rem), hobbes_->filter_stable_[loc_two]);

  char checkpoint_path[100];
  snprintf(checkpoint_path, sizeof(checkpoint_path), "%s/45.chkpnt",
           CHKPNTDIR);
  FILE* checkpoint = fopen(checkpoint_path, "r");
  EXPECT_TRUE(checkpoint != NULL);
  fclose(checkpoint);
}

/// @test     This test checks the dictionary hash capabilities of the filter
TEST_F(HobbesTest, DictionaryHashTest) {
  hobbes_  = new Hobbes(Hobbes::DICTIONARY_HASH);
  storage_ = BlackBoxFactory::StorageBlackBox(hobbes_);

  key = "w1";     // TPCC specific so we don't go out of bounds
  int array_location = Application::GetApplicationInstance()->KeyToInt(key);

  EXPECT_EQ(0, hobbes_->filter_stable_[array_location]);
  storage_->Put(key, values[1], 60);
  EXPECT_EQ(1, hobbes_->filter_stable_[array_location]);

  hobbes_->PrepareForCheckpoint(65);

  EXPECT_EQ(0, hobbes_->filter_unstable_[array_location]);
  storage_->Put(key, values[1], 70);
  EXPECT_EQ(1, hobbes_->filter_unstable_[array_location]);

  hobbes_->CaptureCheckpoint();
  sleep(1);

  EXPECT_EQ(0, hobbes_->filter_unstable_[array_location]);
  EXPECT_EQ(1, hobbes_->filter_stable_[array_location]);

  char checkpoint_path[100];
  snprintf(checkpoint_path, sizeof(checkpoint_path), "%s/65.chkpnt",
           CHKPNTDIR);
  FILE* checkpoint = fopen(checkpoint_path, "r");
  EXPECT_TRUE(checkpoint != NULL);
  fclose(checkpoint);
}

/// @test     This test ensures that we can recover from a backup
TEST_F(HobbesTest, RecoversFromCheckpoint) {
  char checkpoint_path[100];
  snprintf(checkpoint_path, sizeof(checkpoint_path), "%s/30.chkpnt",
           CHKPNTDIR);

  EXPECT_TRUE(hobbes_->RecoverFromCheckpoint(string(checkpoint_path), 30));
  result = storage_->Get(key, 31);
  EXPECT_EQ(string_vals[0], string(reinterpret_cast<char*>(result->data())));

  snprintf(checkpoint_path, sizeof(checkpoint_path), "%s/45.chkpnt",
           CHKPNTDIR);

  EXPECT_TRUE(hobbes_->RecoverFromCheckpoint(string(checkpoint_path), 45));
  result = storage_->Get(key, 46);
  EXPECT_EQ(string_vals[1], string(reinterpret_cast<char*>(result->data())));
}

/// @test     This test ensures that multiple overflowing checkpoints collapse
TEST_F(HobbesTest, CollapsesCheckpoints) {
  hobbes_->PrepareForCheckpoint(115);
  hobbes_->CaptureCheckpoint();
  sleep(1);
  EXPECT_EQ(hobbes_->CollapseCheckpoints(), 1);
  sleep(1);

  FILE* fchk = fopen((string(CHKPNTDIR) + "/65.collapsed_chkpnt").c_str(), "r");
  EXPECT_TRUE(fchk != NULL);
}

}  // namespace calvin

int main(int argc, char **argv) {
  google::InitGoogleLogging(argv[0]);
  system(("rm " + string(CHKPNTDIR) + "/* 2> /dev/null").c_str());
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

