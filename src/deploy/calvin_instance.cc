/// @file
/// @author Thaddeus Diamond <diamond@cs.yale.edu>
/// @version 0.1
/// @since 26 Jan 2012
///
/// @section DESCRIPTION
///
/// Implementation of an instance of the CalvinInstance database

#include "deploy/calvin_instance.h"

/// @TODO: Actually take incoming requests

namespace calvin {

Scheduler* CalvinInstance::scheduler_ = NULL;
Sequencer* CalvinInstance::sequencer_ = NULL;
ConnectionMultiplexer* CalvinInstance::mltplx_ = NULL;

CalvinInstance* CalvinInstance::node_instance_ = NULL;
CalvinInstance* CalvinInstance::SpawnCalvinInstanceInstance(const char* dir,
                                                            int id) {
  new CalvinInstance(dir, id);
  return node_instance_;
}

CalvinInstance::CalvinInstance(const char* dir, int id) {
  if (!Config::LoadConfiguration(id, (string(dir) + "/calvin.conf").c_str()))
    node_instance_ = NULL;
  else
    node_instance_ = this;
}

void CalvinInstance::ShutDown(int signal) {
  LOG(INFO) << "Shutting down calvin instance...";
  delete mltplx_;
  delete scheduler_;
  delete sequencer_;
  LOG(INFO) << "...shutdown complete.";
}

void CalvinInstance::Run() {
  // Handle trap interruptions and start the network connections
  signal(SIGTERM, CalvinInstance::ShutDown);
  mltplx_ = new ConnectionMultiplexer();

  // Initialize sequencer component and start sequencer/scheduler threads up
  ProcessMgr::InitializeProcessMgr(mltplx_->NewConnection("external"));
  sequencer_ = new Sequencer(mltplx_->NewConnection("sequencer"));
  scheduler_ = new Scheduler(mltplx_->NewConnection("scheduler_"));

  // Run the database main loop (scheduler w/workers)
  scheduler_->Run(Application::GetApplicationInstance());
}

}  // namespace calvin
