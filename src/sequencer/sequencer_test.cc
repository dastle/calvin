/// @file
/// @author Alexander Thomson (thomson@cs.yale.edu)
/// @version 0.1
/// @since 5 April 2012
///
/// @section DESCRIPTION
///
/// Sequencer testing

#include "sequencer/sequencer.h"

#include <gtest/gtest.h>
#include <glog/logging.h>

/// @TODO sequencer testing

namespace calvin {

}  // 	namespace calvin

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

