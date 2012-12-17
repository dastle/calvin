/// @file
/// @author Thaddeus Diamond <diamond@cs.yale.edu>
/// @version 0.1
/// @since 26 Jan 2012
///
/// @section DESCRIPTION
///
/// Test suite for singleton backend

#include "backend/storage.h"
#include <gtest/gtest.h>
#include <glog/logging.h>
#include <string>

namespace calvin {

class StorageTest : public testing::Test {
 protected:
  StorageTest() {
    storage_ = Storage::GetStorageInstance();

    key = Key("key");
    ex_val_one = string("value_one");
    ex_val_two = string("value_two");
    value_one   = new Value(ex_val_one);
    value_two   = new Value(ex_val_two);
    value_three = new Value(ex_val_one);
  }

  virtual ~StorageTest() {
    Storage::DestroyStorage();
  }

  Storage* storage_;

  string ex_val_one, ex_val_two, ex_val_three, ex_val_four;
  Key key;
  Value *value_one, *value_two, *value_three, *value_four, *result;
};

/// @test   First, we test simple Get's, Put's and Delete's
TEST_F(StorageTest, SimpleTest) {
  EXPECT_EQ(NULL, storage_->Get(key, 0));

  EXPECT_TRUE(storage_->Put(key, value_one, 0));
  result = storage_->Get(key, 1);
  EXPECT_EQ(ex_val_one, string(reinterpret_cast<char*>(result->data())));

  EXPECT_TRUE(storage_->Delete(key, 2));
  EXPECT_EQ(NULL, storage_->Get(key, 3));
}

/// @test   Next, we test the multi-versioning of our scheme (w/pruning)
TEST_F(StorageTest, MultiversionedTest) {
  EXPECT_TRUE(storage_->Put(key, value_one,   30));
  EXPECT_TRUE(storage_->Put(key, value_two,   20));
  EXPECT_TRUE(storage_->Put(key, value_three, 10));

  Value* result = storage_->Get(key, 5);
  EXPECT_EQ(NULL, result);
  result = storage_->Get(key, 15);
  EXPECT_EQ(ex_val_one, string(reinterpret_cast<char*>(result->data())));
  result = storage_->Get(key, 25);
  EXPECT_EQ(ex_val_two, string(reinterpret_cast<char*>(result->data())));
  result = storage_->Get(key, 35);
  EXPECT_EQ(ex_val_one, string(reinterpret_cast<char*>(result->data())));

  EXPECT_TRUE(storage_->Delete(key, 11));
  EXPECT_EQ(NULL, storage_->Get(key, 15));

  EXPECT_FALSE(storage_->PruneRow(key, 50, true, 0));

  EXPECT_TRUE(storage_->PruneRow(key, 15, true, 0));
  EXPECT_EQ(NULL, storage_->Get(key, 15));

  EXPECT_TRUE(storage_->Put(key, new Value(ex_val_two), 15));
  EXPECT_TRUE(storage_->PruneRow(key, 20, false, 15));
  result = storage_->Get(key, 25);
  EXPECT_EQ(ex_val_two, string(reinterpret_cast<char*>(result->data())));

  EXPECT_TRUE(storage_->PruneRow(key, 25, true, 0));
  EXPECT_FALSE(storage_->PruneRow(key, 35, false, 0));

  result = storage_->Get(key, 50);
  EXPECT_EQ(ex_val_one, string(reinterpret_cast<char*>(result->data())));

  EXPECT_TRUE(storage_->Delete(key, 60));
  EXPECT_EQ(NULL, storage_->Get(key, 65));
}

/// @test   Test if key iteration works in our system
TEST_F(StorageTest, IterationTest) {
  EXPECT_TRUE(storage_->Put(Key("key_c"), value_one,   10));
  EXPECT_TRUE(storage_->Put(Key("key_b"), value_two,   20));
  EXPECT_TRUE(storage_->Put(Key("key_a"), value_three, 30));

  EXPECT_FALSE(storage_->KeyExists("key_c", 5));
  EXPECT_TRUE(storage_->KeyExists("key_c", 15));

  const Key* retrieved_key = storage_->FirstKey(10);
  EXPECT_EQ(NULL, retrieved_key);
  retrieved_key = storage_->FirstKey(20);
  EXPECT_EQ(*retrieved_key, "key_c");
  EXPECT_EQ(NULL, storage_->GetNextKey(*retrieved_key, 20));

  retrieved_key = storage_->FirstKey(30);
  ASSERT_TRUE(retrieved_key != NULL);
  EXPECT_EQ(*retrieved_key, "key_b");
  retrieved_key =  storage_->GetNextKey(*retrieved_key, 30);
  EXPECT_EQ(*retrieved_key, "key_c");
  EXPECT_EQ(NULL, storage_->GetNextKey(*retrieved_key, 20));

  retrieved_key = storage_->FirstKey(40);
  ASSERT_TRUE(retrieved_key != NULL);
  EXPECT_EQ(*retrieved_key, "key_a");
  retrieved_key =  storage_->GetNextKey(*retrieved_key, 30);
  EXPECT_EQ(*retrieved_key, "key_b");
  retrieved_key =  storage_->GetNextKey(*retrieved_key, 30);
  EXPECT_EQ(*retrieved_key, "key_c");
  EXPECT_EQ(NULL, storage_->GetNextKey(*retrieved_key, 20));
}

}  // namespace calvin

int main(int argc, char **argv) {
  google::InitGoogleLogging(argv[0]);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
