/// @file
/// @author Alexander Thomson (thomson@cs.yale.edu)
/// @version 0.1
/// @since 5 April 2012
///
/// @section DESCRIPTION
///
/// Pack/unpack tests.

#include "common/types.h"

#include <gtest/gtest.h>
#include <glog/logging.h>

TEST(TypesTest, PackSignedIntTest) {
  int8  i1 = 65;
  int16 i2 = -2551;
  int32 i3 = 0;
  int64 i4 = -2551255125512551;

  EXPECT_EQ(i1, UnpackInt8(PackInt8(i1)));
  EXPECT_EQ(i2, UnpackInt16(PackInt16(i2)));
  EXPECT_EQ(i3, UnpackInt32(PackInt32(i3)));
  EXPECT_EQ(i4, UnpackInt64(PackInt64(i4)));
}

TEST(TypesTest, PackUnsignedIntTest) {
  uint8  u1 = 251;
  uint16 u2 = 2551;
  uint32 u3 = 0;
  uint64 u4 = 2551255125512551;

  EXPECT_EQ(u1, UnpackUInt8(PackUInt8(u1)));
  EXPECT_EQ(u2, UnpackUInt16(PackUInt16(u2)));
  EXPECT_EQ(u3, UnpackUInt32(PackUInt32(u3)));
  EXPECT_EQ(u4, UnpackUInt64(PackUInt64(u4)));
}

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
