/// @file
/// @author Alexander Thomson (thomson@cs.yale.edu)
/// @version 0.1
/// @since 5 April 2012
///
/// @section DESCRIPTION
///
/// Configuration test suite

#include "common/configuration.h"

#include <gtest/gtest.h>
#include <glog/logging.h>

// common/configuration_test.conf:
//  # Node<id>=<replica>:<partition>:<cores>:<host>:<port>
//  node1=0:1:16:128.36.232.50:50001
//  node2=0:2:16:128.36.232.50:50002
TEST(ConfigurationTest, ReadFromFile) {
  Config* config =
      Config::LoadConfiguration(1, "common/configuration_test.conf");
  EXPECT_EQ(1, config->this_node_id);
  EXPECT_EQ(static_cast<uint32>(2),               // 2 Nodes, node13 and node23.
            config->all_nodes.size());
  EXPECT_EQ(1, config->all_nodes[1]->node_id);
  EXPECT_EQ(50001, config->all_nodes[1]->port);
  EXPECT_EQ(2, config->all_nodes[2]->node_id);
  EXPECT_EQ(string("128.36.232.50"), config->all_nodes[2]->host);
}

/// @TODO(alex): Write proper test once partitioning is implemented.
TEST(ConfigurationTest, LookupPartition) {
  Config* config =
      Config::LoadConfiguration(1, "common/configuration_test.conf");
  EXPECT_EQ(0, config->LookupPartition(Key("0")));
}

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

