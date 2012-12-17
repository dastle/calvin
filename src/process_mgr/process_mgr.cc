/// @file
/// @author Thaddeus Diamond <diamond@cs.yale.edu>
/// @version 0.1
/// @since 26 Jan 2012
///
/// @section DESCRIPTION
///
/// The Process Manager module (depending on whether it's using an internal
/// or external loadgen) queues up the next task for the sequencer to run

/// @TODO: (Thad) Support internal loadgen

#include "process_mgr/process_mgr.h"

namespace calvin {

ProcessMgr* ProcessMgr::process_mgr_instance_ = NULL;
ProcessMgr* ProcessMgr::InitializeProcessMgr(Connection* app_socket) {
  process_mgr_instance_ = new ProcessMgr(app_socket);
  return process_mgr_instance_;
}

ProcessMgr* ProcessMgr::GetProcessMgrInstance() {
  return process_mgr_instance_;
}

ProcessMgr::ProcessMgr(Connection* app_socket) : app_socket_(app_socket) {}

bool ProcessMgr::GetQueryNonblocking() {
  MessageProto msg;
  if (app_socket_->GetMessage(&msg)) {
    DLOG(INFO) << "GOT MESSAGE IN PROCESS_MGR!";
    return true;
  }
  return false;
}

}  // namespace calvin
