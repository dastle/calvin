/// @file
/// @author Alexander Thomson (thomson@cs.yale.edu)
/// @author Shu-chun Weng (scweng@cs.yale.edu)
/// @version 0.1
/// @since 5 April 2012
///
/// @section DESCRIPTION
///
/// Library for handling messaging between system nodes. Each node generally
/// owns a ConnectionMultiplexer object as well as a Configuration object.

#ifndef _DB_CONNECTION_CONNECTION_H_
#define _DB_CONNECTION_CONNECTION_H_

#include <pthread.h>

#include <map>
#include <set>
#include <string>
#include <vector>

#include "common/zmq.hpp"
#include "proto/message.pb.h"

using std::map;
using std::set;
using std::string;
using std::vector;

class Config;

/// @TODO(alex): What if multiplexer receives a message sent to a local channel
//               that doesn't exist (yet)?
class Connection;
class ConnectionMultiplexer {
 public:
  // Create a ConnectionMultiplexer that establishes two-way communication with
  // Connections for every other node specified by '*config' to exist.
  ConnectionMultiplexer();

  // Creates a multiplexer with the specified configuration file
  ConnectionMultiplexer(Config *config);
 

  /// @TODO(alex): The deconstructor currently closes all sockets. Connection
  //             objects, however, do not have a defined behavior for trying to
  //             send messages to the multiplexor after it has been destroyed.
  ~ConnectionMultiplexer();

  // Creates and registers a new connection with channel name 'channel', unless
  // the channel name is already in use, in which case NULL is returned. The
  // caller (not the multiplexer) owns of the newly created Connection object.
  Connection* NewConnection(const string& channel);

  zmq::context_t* context() { return context_; }

 private:
  friend class Connection;

  // Stores a reference to the configuration
  Config *config_;

  // This is used as a common function for constructors
  void Init(Config *config);

  // Runs the Multiplexer's main loop. Run() is called in a new thread by the
  // constructor.
  void Run();

  // Function to call multiplexer->Run() in a new pthread.
  static void* RunMultiplexer(void *multiplexer);

  /// @TODO(alex): Comments.
  void Send(const MessageProto& message);

  // Separate pthread context in which to run the multiplexer's main loop.
  pthread_t thread_;

  // Context shared by all Connection objects with channels to this
  // multiplexer.
  zmq::context_t* context_;

  // Port on which to listen for incoming messages from other nodes.
  int port_;

  // Socket listening for messages from other nodes. Type = ZMQ_PULL.
  zmq::socket_t* remote_in_;

  // Sockets for outgoing traffic to other nodes. Keyed by node_id.
  // Type = ZMQ_PUSH.
  map<int, zmq::socket_t*> remote_out_;

  // Socket listening for messages from Connections. Type = ZMQ_PULL.
  zmq::socket_t* inproc_in_;

  // Sockets for forwarding messages to Connections. Keyed by channel
  // name. Type = ZMQ_PUSH.
  map<string, zmq::socket_t*> inproc_out_;

  // Stores messages addressed to local channels that do not exist at the time
  // the message is received (so that they may be delivered if a connection is
  // ever created with the specified channel name).
  //
  /// @TODO(alex): Prune this occasionally?
  map<string, vector<MessageProto> > undelivered_messages_;

  // Protects concurrent calls to NewConnection().
  pthread_mutex_t new_connection_mutex_;

  // Specifies a requested channel. Null if there is no outstanding new
  // connection request.
  const string* new_connection_channel_;

  // Specifies channel requested to be deleted. Null if there is no outstanding
  // connection deletion request.
  const string* delete_connection_channel_;

  // Pointer to Connection objects recently created in the Run() thread.
  Connection* new_connection_;

  // False until the deconstructor is called. As soon as it is set to true, the
  // main loop sees it and stops.
  bool deconstructor_invoked_;

  // DISALLOW_COPY_AND_ASSIGN
  ConnectionMultiplexer(const ConnectionMultiplexer&);
  ConnectionMultiplexer& operator=(const ConnectionMultiplexer&);
};

class Connection {
 public:
  // Closes all sockets.
  ~Connection();

  // Sends 'message' to the Connection specified by
  // 'message.destination_node()' and 'message.destination_channel()'.
  void Send(const MessageProto& message);

  // Loads the next incoming MessageProto into 'message'. Returns true, unless
  // no message is queued up to be delivered, in which case false is returned.
  // 'message->Clear()' is NOT called. Non-blocking.
  bool GetMessage(MessageProto* message);

  // Loads the next incoming MessageProto into 'message'. If no message is
  // queued up to be delivered, GetMessageBlocking waits at most 'max_wait_time'
  // seconds for a message to arrive. If no message arrives, false is returned.
  // 'message->Clear()' is NOT called.
  bool GetMessageBlocking(MessageProto* message, double max_wait_time);

  // Links 'channel' to this Connection object so that messages sent to
  // 'channel' will be forwarded to this Connection.
  //
  // Requires: The requested channel name is not already in use.
  void LinkChannel(const string& channel);

  // Links 'channel' from this Connection object so that messages sent to
  // 'channel' will no longer be forwarded to this Connection.
  //
  // Requires: The requested channel name was previously linked to this
  // Connection by LinkChannel.
  void UnlinkChannel(const string& channel);

  // Returns a pointer to this Connection's multiplexer.
  ConnectionMultiplexer* multiplexer() { return multiplexer_; }

  // Return a const ref to this Connection's channel name.
  const string& channel() { return channel_; }

 private:
  friend class ConnectionMultiplexer;

  // Channel name that 'multiplexer_' uses to identify which messages to
  // forward to this Connection object.
  string channel_;

  // Additional channels currently linked to this Connection object.
  set<string> linked_channels_;

  // Pointer to the main ConnectionMultiplexer with which the Connection
  // communicates. Not owned by the Connection.
  ConnectionMultiplexer* multiplexer_;

  // Socket for sending messages to 'multiplexer_'. Type = ZMQ_PUSH.
  zmq::socket_t* socket_out_;

  // Socket for getting messages from 'multiplexer_'. Type = ZMQ_PUSH.
  zmq::socket_t* socket_in_;

  zmq::message_t msg_;
};

#endif  // _DB_CONNECTION_CONNECTION_H_
