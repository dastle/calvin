/// @file
/// @author Alexander Thomson (thomson@cs.yale.edu)
/// @author Shu-chun Weng (scweng@cs.yale.edu)
/// @version 0.1
/// @since 5 April 2012
///
/// @section DESCRIPTION
///
/// Each node in the system has a Config, which stores the identity of
/// that node, the system's current execution mode, and the set of all currently
/// active nodes in the system.
///
/// Config file format:
///  # (Lines starting with '#' are comments.)
///  # List all nodes in the system.
///  # Node\<id\>=\<replica\>:\<partition\>:\<cores\>:\<host\>:\<port\>
///  node13=1:3:16:4.8.15.16:1001:1002
///  node23=2:3:16:4.8.15.16:1004:1005
///
/// Note: Epoch duration, application and other global global options are
///       specified as command line options at invocation time (see
///       deployment/main.cc).

#ifndef _DB_COMMON_CONFIGURATION_H_
#define _DB_COMMON_CONFIGURATION_H_

#include <stdint.h>
#include <glog/logging.h>
#include <map>
#include <string>

#include "common/types.h"
#include "common/dynamic_defines.h"

/// @todo I HATE THESE DEFINEs!  Let's find a way to get rid of dem...
#define DISTRICTS_PER_WAREHOUSE 100
#define DISTRICTS_PER_NODE (WAREHOUSES_PER_NODE * DISTRICTS_PER_WAREHOUSE)
#define CUSTOMERS_PER_DISTRICT 300
#define CUSTOMERS_PER_NODE (DISTRICTS_PER_NODE * CUSTOMERS_PER_DISTRICT)
#define NUMBER_OF_ITEMS 100000
#define NUMBER_OF_TXNS 2000000                       // 2.5M of each txn type
#define TOTAL_RECORDS \
        WAREHOUSES_PER_NODE + DISTRICTS_PER_NODE + CUSTOMERS_PER_NODE + \
        NUMBER_OF_ITEMS + (NUMBER_OF_ITEMS * WAREHOUSES_PER_NODE) + \
        (13 * NUMBER_OF_TXNS * WAREHOUSES_PER_NODE)

using std::map;
using std::string;
using std::vector;

struct Node {
  // Globally unique node identifier.
  int node_id;
  int replica_id;
  int partition_id;

  // IP address of this node's machine.
  string host;

  // Port on which to listen for messages from other nodes.
  int port;

  // Total number of cores available for use by this node.
  // Note: Is this needed?
  int cores;
};

class MachineConfig {
 public:
  MachineConfig(const string& filename); 
  void ProcessConfigLine(char key[], char value[]);

  // Tracks the set of current active nodes in the system.
  vector<Node*> all_nodes;
};


class Config {
 public:
  /// The following initializes the configuration we want
  ///
  /// @param        node_id     The current node ID
  /// @param        filename    Name of the configuration file
  static Config* LoadConfiguration(int node_id, const string& filename);

  /// The following returns the current configuration singleton
  ///
  /// @returns      The current configuration instance
  static Config* GetConfigInstance();

  // Returns the node_id of the partition at which 'key' is stored.
  int LookupPartition(const Key& key) const;

  // Dump the current config into the file in key=value format.
  // Returns true when success.
  bool WriteToFile(const string& filename) const;

  // This node's node_id.
  int this_node_id;

  // Tracks the set of current active nodes in the system.
  map<int, Node*> all_nodes;

 private:
  /// Following the Singleton DP, we have a static private var
  static Config* configuration_instance_;

  /// Singleton DP constructor
  Config(int node_id, const string& filename);

  /// @TODO(alex): Comments.
  void ProcessConfigLine(char key[], char value[]);
  int ReadFromFile(const string& filename);
};

#endif  // _DB_COMMON_CONFIGURATION_H_

