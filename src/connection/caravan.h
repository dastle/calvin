#include "leveldb/db.h"
#include <map>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include "stdlib.h"
#include "pthread.h"
#include <fcntl.h>
#include "common/configuration.h"

namespace caravan {

using std::string;
using leveldb::Slice;
using leveldb::Status;

typedef unsigned long long MachineID;


// Representation of cluster status (for tracking all (other) nodes in cluster)
struct ClusterState {
  struct NodeRep {
    MachineID node_id_;  // node id
//    zmq::socket_t outgoing_socket_;  // socket to node
    Status node_state_;
  };
  std::map<MachineID, NodeRep> nodes_;
};


// Deploys/monitors/repairs/destroys a collection of Machines.
//
// This object should exist in only ONE place throughout the entire cluster,
// probably in its own separate cluster-management process. A distributed
// application using Caravan should therefore be comprised of two separate
// binaries: (a) a deployment/management program that makes use of the
// 'ClusterManager' class, and (b) the actual application program that runs
// on each machine, making use of the 'Machine' class. The application can then
// be started up by invoking the deployment binary, which SSHs into a set of
// machines (specified in the config file) and starts the application running
// on each of those machines.
//
// Possible future extension: maybe several copies of this object should exist
// running on several machines in the cluster, synchronized via paxos?
//
class ClusterManager {

 public:
  // Reads a config-file.
  // TODO: document config-file options/format here
  ClusterManager(const Slice *config_filename);
  ClusterManager();

  // SSHs to each machine specified by the config file. Spawns a background
  // thread locally that periodically polls each machine to monitor cluster
  // status.
  Status StartApplication(const Slice *application_path);

  // End application
  Status KillApplication();

  // Returns Status::OK() if all the specified machine is running normally,
  // else returns an informative error.
  Status MachineStatus(MachineID machine);

  // Returns Status::OK() if all machines are running normally, else returns
  // an informative error.
  Status ClusterStatus();

 private:
  // Internal representation of cluster status.
  ClusterState cluster_state_;


};

struct Socket {
  sockaddr_in RemoteEndPoint;
  int Handle;
  string IPAddress;
  bool Connected;
  
public:
  Socket() {
    Handle = 0;
    Connected = false;
    IPAddress = string(""); 
  }
};

// TODO: Maybe this should be a protobuf?
class MachineOptions {
 public:
  bool log_outgoing_;  // True iff outgoing messages should be logged.
  FILE *log_file_;  // File to which outgoing messages are logged.
  //... other options?
};



class Message {
  public:
    Message(char* body, int size, int sourceID, char* channel, int channel_size) {
      data_ = body;
      body_size_ = size;
      source_id_ = sourceID;
      channel_ = channel;
      channel_size_ = channel_size;
    }
  
    int size() {
      return body_size_;
    }

    int channel_size() {
      return channel_size_;
    }

    char *channel() {
      return channel_;
    }

    MachineID GetSourceID(){
      return source_id_;
    }
    
    char *data() {
      return data_;
    }

    ~Message() {
      delete data_;
    }

  private:
    char* data_;
    int body_size_;
    char* channel_;
    int channel_size_;
    MachineID source_id_;
};

class Machine {
 public:
    
  Machine(MachineID id, unsigned int port);
  Machine(MachineID id, const MachineConfig& config);

  ~Machine();

  // Returns this machine instance's id.
  MachineID machine_id();

  void AddMachine(MachineID otherID, const std::string& ipAddress, unsigned int port);
  bool Connect(MachineID machineID);
  void Listen();
  Message *ReceiveMessage();

  // Sends a message that can be retrieved by calling GetMessage('channel', ...)
  // on the destination machine.
  void SendMessage(
      MachineID destination,
      const Slice& channel,
      const Slice& message);

  // Dequeue a message from the local queue for the specified channel. If such
  // a message exists, sets '*message' to point to it (the caller takes
  // ownership of '**message') and returns true; else returns false.
  bool GetMessage(const Slice& channel, string** message);

  void StartListening();
  void StopListening();

  bool IsConnectedTo(MachineID machineID);
  bool IsConnectedFrom(MachineID machineID);

private:


  void Initialize(MachineID id, unsigned int port);
  void InitializeListenerSocket();

  static void *ListenerLoop(void *arg) {
    Machine *machine = (Machine *)arg;
    if (!machine->isSocketInitialized_) {
      machine->InitializeListenerSocket();
      machine->isSocketInitialized_ = true;
    }
    while (!machine->stopListening_) {
      machine->Listen();
    }
    return NULL;
  }

  // This needs to be a ptr because if it isn't, the sockets won't close correctly for some reason
  fd_set *socket_fd_set_;
  MachineID machine_id_;
  unsigned int port_;
  std::map<MachineID,Socket *> sockets_out_;
  std::map<MachineID,Socket *> sockets_in_;
  std::map<string,MachineID> ip_id_map_;
  int inSocketHandle_;
  pthread_t listenerThread_;
  bool stopListening_;
  bool isSocketInitialized_;
  char send_buf_[255];
};


}
