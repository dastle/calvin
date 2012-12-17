/// @file
/// @author Alex Thomson <thomson@cs.yale.edu>
/// @version 0.1
/// @since 5 April 2012
///
/// @section DESCRIPTION
///
/// KeyFormat unit test suite

#include "query_processor/key_format.h"

#include <stdio.h>
#include <gtest/gtest.h>
#include <glog/logging.h>

namespace calvin {

class KeyFormatTest : public testing::Test {
 protected:
  KeyFormatTest() {}

  virtual ~KeyFormatTest() {}
};

/// @TODO(alex) Test error cases.
TEST_F(KeyFormatTest, CodeHRConversion) {
  AtomTable at;
  at.AddAtom("index",     1, AtomTable::NONE);
  at.AddAtom("warehouse", 2, AtomTable::UINT64);
  at.AddAtom("district",  3, AtomTable::UINT64);
  at.AddAtom("customer",  4, AtomTable::UINT64);
  at.AddAtom("name",      5, AtomTable::STRING);
  at.AddAtom("stock",     6, AtomTable::UINT64);

  string w("warehouse(1)");
  string d("warehouse(1).district(3)");
  string c("warehouse(1).district(3).customer(91)");
  string i("index.name('diamond').warehouse(1).district(3).customer(92)");
  string s("warehouse(1).stock(1337)");

  EXPECT_EQ(w, at.CodedToHR(at.HRToCoded(w)));
  EXPECT_EQ(d, at.CodedToHR(at.HRToCoded(d)));
  EXPECT_EQ(c, at.CodedToHR(at.HRToCoded(c)));
  EXPECT_EQ(i, at.CodedToHR(at.HRToCoded(i)));
  EXPECT_EQ(s, at.CodedToHR(at.HRToCoded(s)));
}

}  // namespace calvin

int main(int argc, char **argv) {
  google::InitGoogleLogging(argv[0]);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

