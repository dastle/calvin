/// @file
/// @author Thaddeus Diamond <diamond@cs.yale.edu>
/// @version 0.1
/// @since 26 Jan 2012
///
/// @section DESCRIPTION
///
/// Compiled into a "Calvin" binary for launching an instance on any node
/// (called from CalvinCtl)

#define PRINT_BUFSIZE 1024

#include <glog/logging.h>
#include <ctype.h>
#include <fcntl.h>
#include <cstdlib>
#include <string>
#include "deploy/calvin_instance.h"
#include "checkpointing/checkpointer.h"
#include "checkpointing/hobbes.h"
#include "checkpointing/ping_pong.h"
#include "checkpointing/naive.h"
#include "stored_procedures/application.h"
#include "stored_procedures/microbenchmark/microbenchmark.h"
#include "stored_procedures/tpcc/tpcc.h"

using std::string;
using calvin::CalvinInstance;

int   NodeID;
char  Lock[PRINT_BUFSIZE];
char* Directory;

char   Application = 'm';
char*  RecoveryFile = NULL;
char   CPArg = 'x';
string CPOpts;
int    DirtyArg = 100;

/// PARSE ARGUMENTS
///
/// Updated: 28 March 2012
/// Thaddeus Diamond
int ParseArguments(int argc, char* argv[]) {
  for (int i = 0; i < argc; i++) {
    if (!strcmp(argv[i], "-D"))
      Directory = argv[++i];
    if (!strcmp(argv[i], "--application"))
      Application = argv[++i][0];
    if (!strcmp(argv[i], "--node"))
      NodeID = atoi(argv[++i]);
    if (!strcmp(argv[i], "--dirty"))
      DirtyArg = atoi(argv[++i]);
    if (!strcmp(argv[i], "-C")) {
      CPArg = argv[++i][0];
      CPOpts = string(argv[++i]);
    }
    if (!strcmp(argv[i], "--recovery"))
      RecoveryFile = argv[++i];
    if (!strcmp(argv[i], "-s"))
      FLAGS_minloglevel = 2;                      // 2 => ERROR (see GLOG)
  }

  return 0;
}

/// ACQUIRE EXCLUSIVE LOCK IN FILESYSTEM (APPLICATION SINGLETON)
///
/// Updated: 28 March 2012
/// Thaddeus Diamond
bool AcquireLock() {
  // Acquire the lock atomically using Linux O_EXCL flag
  char buffer[PRINT_BUFSIZE];
  snprintf(Lock, sizeof(Lock), "%s/calvinmaster_%d.pid", Directory, NodeID);
  int lock = open(Lock, O_RDWR | O_CREAT | O_EXCL,
                  S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

  // Error on opening file
  if (lock < 0) {
    // Duplicated lock
    if (errno == EEXIST) {
      LOG(ERROR) << "lock file '" << Lock << "' already exists";
      int pid = atoi(fgets(buffer, sizeof(buffer), fopen(Lock, "r")));
      if (kill(pid, 0)) {
        LOG(WARNING) << "process seems to be dead. Launch ahoy...";
        lock = open(Lock, O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
      } else {
        LOG(FATAL) << "is calvinmaster (PID " << pid << ") running w/data dir '"
                   << Directory << "'?";
        return false;
      }

    // Generic FS error
    } else {
      LOG(FATAL) << "error creating lock file '" << Lock << "'";
      return false;
    }
  }

  // Write out the contents of the lock file
  snprintf(buffer, sizeof(buffer), "%d\n%s\n", getpid(), Directory);
  write(lock, buffer, strlen(buffer));
  return true;
}

/// REMOVE EXCLUSIVE LOCK IN FILESYSTEM
///
/// Updated: 28 March 2012
/// Thaddeus Diamond
void RemoveLock() {
  remove(Lock);
}

/// MAIN METHOD
///
/// Updated: 28 March 2012
/// Thaddeus Diamond
int main(int argc, char* argv[]) {
  ParseArguments(argc, argv);
  FLAGS_logtostderr = true;
  google::InitGoogleLogging(argv[0]);

  if (!AcquireLock())
    exit(EXIT_FAILURE);

  // TODO make application depend on arg instead of hardcoding it
  if (Application == 'm') {
    calvin::Microbenchmark::InitializeApplication(
        1, calvin::Microbenchmark::kDBSize * DirtyArg / 100, 5, 5);
  } else {
    calvin::TPCC::InitializeApplication();
  }
  calvin::Application* app = calvin::Application::GetApplicationInstance();

  // Checkpointing options
  calvin::Checkpointer* decorator = NULL;
  if (CPArg != 'x') {
    if (CPArg == 'h') {
      calvin::Hobbes* hobbes;
      if (CPOpts == string("--dicthash"))
        hobbes = new calvin::Hobbes(calvin::Hobbes::DICTIONARY_HASH);
      else if (CPOpts == string("--bloom"))
        hobbes = new calvin::Hobbes(calvin::Hobbes::BLOOM_FILTER);
      else
        hobbes = new calvin::Hobbes();
      decorator = hobbes;
    } else if (CPArg == 'p') {
      calvin::PingPong* ping_pong;
      if (CPOpts == string("--interleaved"))
        ping_pong = new calvin::PingPong(app->DBSize(), calvin::PingPong::INTERLEAVED);
      else if (CPOpts == string("--hash_map"))
        ping_pong = new calvin::PingPong(app->DBSize(), calvin::PingPong::HASH_MAP);
      else if (CPOpts == string("--hash_map_ipp"))
        ping_pong = new calvin::PingPong(app->DBSize(), calvin::PingPong::INTERLEAVED_HASH_MAP);
      else
        ping_pong = new calvin::PingPong(app->DBSize());
      decorator = ping_pong;
    } else if (CPArg == 'n') {
      calvin::NaiveCheckpointer* naive = new calvin::NaiveCheckpointer();
      decorator = naive;
    } else if (CPArg == 'm') {
      calvin::MultiversionCheckpointer* multiversion = new calvin::MultiversionCheckpointer();
      decorator = multiversion;
    }
  }

  calvin::Storage::InitializeStorage(decorator, NULL);

  // Recover if need be
  if (RecoveryFile)
    decorator->RecoverFromCheckpoint(string(RecoveryFile), -1);

  if (CalvinInstance::SpawnCalvinInstanceInstance(Directory, NodeID))
    CalvinInstance::Run();

  RemoveLock();
  exit(EXIT_SUCCESS);
}
