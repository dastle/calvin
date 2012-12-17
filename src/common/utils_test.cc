/// @file
/// @author Alexander Thomson <thomson@cs.yale.edu>
/// @version 0.1
/// @since 26 Jan 2012
///
/// @section DESCRIPTION
///
/// Utility library unit test suite

#include "common/utils.h"

#include <gtest/gtest.h>
#include <glog/logging.h>
#include <vector>

#include "common/types.h"

using std::vector;

TEST(UtilsTest, Varint) {
  vector<uint64> unencoded;
  string s;
  for (int i = 0; i < 1000000; i++) {
    uint64 target = rand();
    unencoded.push_back(target);
    CVarint::Append64(&s, target);
  }

  const char* pos = s.data();
  for (int i = 0; i < 1000000; i++) {
    uint64 x;
    pos = CVarint::Parse64(pos, &x);
    EXPECT_EQ(unencoded[i], x);
  }
}

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
