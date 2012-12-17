/// @file
/// @author Thaddeus Diamond  <diamond@cs.yale.edu>
/// @version 0.1
/// @since 26 Jan 2012
///
/// @section DESCRIPTION
///
/// This defines an instance of the Calvin database.  It is instantiated
/// exactly once for interaction with the calvinmaster program running on a node

#ifndef _DB_DEPLOY_CALVIN_INSTANCE_H_
#define _DB_DEPLOY_CALVIN_INSTANCE_H_

#include <glog/logging.h>
#include <signal.h>
#include <string>
#include "stored_procedures/application.h"
#include "common/types.h"
#include "common/configuration.h"
#include "connection/connection.h"
#include "process_mgr/process_mgr.h"
#include "scheduler/scheduler.h"
#include "sequencer/sequencer.h"

using std::string;

namespace calvin {

/// @class CalvinInstance
/// @brief This defines an instance of the Calvin DB (replacing deployment/main)
///
/// We want to allow any arbitrary executable to be able to simply and easily
/// create an instance of the Calvin database system.
///
/// @attention  The CalvinInstance class follows the Singleton DP
class CalvinInstance {
 public:
  /// The CalvinInstance destructor must free all aggregated memory.
  virtual ~CalvinInstance() {}

  /// The following initializes the system with several configuration
  /// parameters.
  ///
  /// @returns    The sole instance of Calvin (NULL if not correctly init)
  static CalvinInstance* SpawnCalvinInstanceInstance(const char* dir, int id);

  /// A Calvin instance is simply "Run", which sets it off to accept
  /// incoming transactions and process them ad infinitum
  static void Run();

  /// In order to properly clean up a calvin instance must handle TRAP
  /// signals gracefully and shut itself down
  static void ShutDown(int signal);

 protected:
  /// Following the Singleton DP, we have a static private var
  static CalvinInstance* node_instance_;

  /// The constructor for calvin is protected so that child classes
  /// can initialize it
  CalvinInstance(const char* directory, int node_id);

  /// So that we can shut it down gracefully, we keep track of the database
  /// scheduler, sequencer, and multiplexer
  static Scheduler* scheduler_;
  static Sequencer* sequencer_;
  static ConnectionMultiplexer* mltplx_;
};

}  // namespace calvin

#endif  // _DB_DEPLOY_CALVIN_INSTANCE_H_
