/// @file
/// @author Thaddeus Diamond <diamond@cs.yale.edu>
/// @version 0.1
/// @since 26 Jan 2012
///
/// @section DESCRIPTION
///
/// ProcessMgr unit test suite

#include <gtest/gtest.h>
#include <glog/logging.h>
#include "process_mgr/process_mgr.h"
#include "common/black_box_factory.h"

namespace calvin {

class ProcessMgrTest : public testing::Test {
 protected:
  ProcessMgrTest() {}

  virtual ~ProcessMgrTest() {}
};

/// @TODO (Thad): Implement a unit test
TEST_F(ProcessMgrTest, FirstTest) {
}

}  // namespace calvin

int main(int argc, char **argv) {
  google::InitGoogleLogging(argv[0]);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

