/// @file
/// @author Alexander Thomson <thomson@cs.yale.edu>
/// @version 0.1
/// @since 5 May 2012
///
/// @section DESCRIPTION
///
/// Test suite for singleton backend

#include "backend/storage_iterator.h"

#include <gtest/gtest.h>
#include <glog/logging.h>
#include <string>

#include "backend/storage.h"

namespace calvin {

class StorageIteratorTest : public testing::Test {
 protected:
  StorageIteratorTest() {
    storage_ = Storage::GetStorageInstance();

    key = Key("key");
    ex_val_one = string("value_one");
    ex_val_two = string("value_two");
    value_one   = new Value(ex_val_one);
    value_two   = new Value(ex_val_two);
    value_three = new Value(ex_val_one);
  }

  virtual ~StorageIteratorTest() {
    Storage::DestroyStorage();
  }

  Storage* storage_;

  string ex_val_one, ex_val_two, ex_val_three, ex_val_four;
  Key key;
  Value *value_one, *value_two, *value_three, *value_four, *result;
};

/// @test   StorageIterator functionality test
/// @TODO(alex):  Test Advance/SetLimit once they are implemented.
TEST_F(StorageIteratorTest, IteraterTest) {
  // Create Storage with 3 objects at 3 different version ids (10, 20, and 30).
  EXPECT_TRUE(storage_->Put(Key("key_c"), value_one,   10));
  EXPECT_TRUE(storage_->Put(Key("key_b"), value_two,   20));
  EXPECT_TRUE(storage_->Put(Key("key_a"), value_three, 30));

  // Get iterator at version 40. Expect to see all three versions, then done.
  Iterator* iter = storage_->GetIterator(40);
  EXPECT_FALSE(iter->Valid());
  iter->SeekToFirst();  // Advance to first version.
  EXPECT_TRUE(iter->Valid());
  EXPECT_EQ(iter->key(), "key_a");
  EXPECT_EQ(iter->value(), value_three);
  iter->Next();
  EXPECT_TRUE(iter->Valid());
  EXPECT_EQ(iter->key(), "key_b");
  EXPECT_EQ(iter->value(), value_two);
  iter->Next();
  EXPECT_TRUE(iter->Valid());
  EXPECT_EQ(iter->key(), "key_c");
  EXPECT_EQ(iter->value(), value_one);
  iter->Next();
  EXPECT_FALSE(iter->Valid());
  delete iter;

  // Get iterator at version 40. Expect to see all three versions, then done.
  // BUT BACKWARDS
  iter = storage_->GetIterator(40);
  EXPECT_FALSE(iter->Valid());
  iter->SeekToLast();  // Advance to first version.
  EXPECT_TRUE(iter->Valid());
  EXPECT_EQ(iter->key(), "key_c");
  EXPECT_EQ(iter->value(), value_one);
  iter->Prev();
  EXPECT_TRUE(iter->Valid());
  EXPECT_EQ(iter->key(), "key_b");
  EXPECT_EQ(iter->value(), value_two);
  iter->Prev();
  EXPECT_TRUE(iter->Valid());
  EXPECT_EQ(iter->key(), "key_a");
  EXPECT_EQ(iter->value(), value_three);
  iter->Prev();
  EXPECT_FALSE(iter->Valid());
  delete iter;

  // Get iterator at version 25. Expect to see only two versions, then done.
  iter = storage_->GetIterator(25);
  iter->SeekToFirst();  // Advance to first version.
  EXPECT_TRUE(iter->Valid());
  EXPECT_EQ(iter->key(), "key_b");
  EXPECT_EQ(iter->value(), value_two);
  iter->Next();
  EXPECT_TRUE(iter->Valid());
  EXPECT_EQ(iter->key(), "key_c");
  EXPECT_EQ(iter->value(), value_one);
  iter->Next();
  EXPECT_FALSE(iter->Valid());
  delete iter;

  // Get iterator at version 25. Expect to see only two versions, then done.
  // BUT BACKWARDS
  iter = storage_->GetIterator(25);
  iter->SeekToLast();  // Advance to first version.
  EXPECT_TRUE(iter->Valid());
  EXPECT_EQ(iter->key(), "key_c");
  EXPECT_EQ(iter->value(), value_one);
  iter->Prev();
  EXPECT_TRUE(iter->Valid());
  EXPECT_EQ(iter->key(), "key_b");
  EXPECT_EQ(iter->value(), value_two);
  iter->Prev();
  EXPECT_FALSE(iter->Valid());
  delete iter;
}

}  // namespace calvin

int main(int argc, char **argv) {
  google::InitGoogleLogging(argv[0]);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
