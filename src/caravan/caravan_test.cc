/// @file
/// @author Alexander Thomson (thomson@cs.yale.edu)
/// @version 0.1
/// @since 5 April 2012
///
/// @section DESCRIPTION
///
/// Connection unit tests.
///
/// @TODO(alex): Write some tests spanning multiple physical machines.
/// @TODO(alex): All tests are commented out! Add them back in plz kthks.


#include <gtest/gtest.h>
#include <glog/logging.h>

TEST(NULL, ChannelNotCreatedYetTest) {
 printf("hei");
}

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

