/// @file
/// @author Thaddeus Diamond  <diamond@cs.yale.edu>
/// @version 0.1
/// @since 26 Jan 2012
///
/// @section DESCRIPTION
///
/// Interface to the ProcessManager, which uses a pull mechanism to scrape the
/// application Connection for new queries

#ifndef _DB_PROCESS_MGR_PROCESS_MGR_H_
#define _DB_PROCESS_MGR_PROCESS_MGR_H_

#include <glog/logging.h>
#include "connection/connection.h"

namespace calvin {

/// @class ProcessMgr
/// @brief The ProcessMgr is responsible for employing admission control
///
/// By scraping and queuing on demand, the ProcessMgr is able to employ
/// admission control for incoming requests
///
/// @attention  The ProcessMgr class follows the Singleton DP
class ProcessMgr {
 public:
  /// The ProcessMgr destructor must free all aggregated memory.
  virtual ~ProcessMgr() {}

  /// The following initializes the process manager
  ///
  /// @param      app_socket    The Connection object receiving app msgs
  ///
  /// @returns    The sole instance of the ProcessMgr
  static ProcessMgr* InitializeProcessMgr(Connection* app_socket);

  /// Create a way for the program to return the instance
  ///
  /// @returns    The sole instance of the ProcessMgr
  static ProcessMgr* GetProcessMgrInstance();

  /// Simple mechanism for the sequencer to get back the next query for
  /// sending to the Scheduler
  bool GetQueryNonblocking();

 protected:
  /// Following the Singleton DP, we have a static private var
  static ProcessMgr* process_mgr_instance_;

  /// The constructor for ProcessMgr is protected so that child classes
  /// can initialize it
  ///
  /// @param      app_socket    The socket to receive app msgs on
  explicit ProcessMgr(Connection* app_socket);

  /// Keep track of the connection to receive app msgs on
  Connection* app_socket_;
};

}  // namespace calvin

#endif  // _DB_PROCESS_MGR_PROCESS_MGR_H_
