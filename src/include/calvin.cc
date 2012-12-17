/// @file
/// @author Thaddeus Diamond <diamond@cs.yale.edu>
/// @version 0.1
/// @since 26 Jan 2012
///
/// @section DESCRIPTION
///
/// This is the implementation file for the Calvin API.

#include "include/calvin.h"

#define PRINT_BUFSIZE 1024

namespace CalvinAPI {

void Log(Level level, const char* format, ...) {
#ifndef DEBUG_MODE
  if (level == DEBUG)
    return;
#endif

  // Print formatted output into a buffer
  va_list arguments;
  va_start(arguments, format);
  char buffer[PRINT_BUFSIZE];
  memset(buffer, 0, sizeof(buffer));
  vsprintf(buffer, format, arguments);

  // Get the current process name (if error)
  char proc_dir[PRINT_BUFSIZE], exe_name[PRINT_BUFSIZE];
  snprintf(proc_dir, sizeof(proc_dir), "/proc/%d/exe", getpid());
  int bytes = readlink(proc_dir, exe_name, sizeof(exe_name));
  exe_name[bytes] = '\0';

  // Find only binary name (remove backslashes)
  int last_backslash = 0;
  for (int i = 0; i < bytes; i++) {
    if (exe_name[i] == '/')
      last_backslash = i;
  }

  // Write output
  if (level == ERROR)
    fprintf(stdout, "%s: %s", exe_name + last_backslash + 1, buffer);
  else
    fprintf(stdout, "%s", buffer);
  va_end(arguments);
}

/// Keep track of the state of ZeroMQ instance
map<NodeIdentifier, zmq::context_t*> calvin_contexts;
map<NodeIdentifier, zmq::socket_t*> calvin_sockets;
int node_id;

/// Maintain a buffer to SQL command building up
int MAX_LINE_LENGTH = 64;
int MAX_SQL_BUFFER = 2;
map<NodeIdentifier, string> SQLBuffer;

NodeIdentifier* ConnectToDatabase(const char* host, int port, int node) {
  // Check if we're already connected
  NodeIdentifier* identifier = new NodeIdentifier(string(host), port);
  if (calvin_sockets.count(*identifier) > 0) {
    Log(ERROR, "Already connected to that host and port (%s:%d)\n", host, port);
    return identifier;
  }

  // Connect via ZeroMQ
  try {
    node_id = node;
    calvin_contexts[*identifier] = new zmq::context_t(1);
    calvin_sockets[*identifier]
      = new zmq::socket_t(*calvin_contexts[*identifier], ZMQ_PUSH);

    char* buffer = new char[MAX_LINE_LENGTH];
    snprintf(buffer, MAX_LINE_LENGTH, "tcp://%s:%d", host, port);
    calvin_sockets[*identifier]->connect(buffer);
    delete[] buffer;

  // Catch all vanilla errors
  } catch(...) {
    return NULL;
  }

  return identifier;
}

const char* BufferSQL(NodeIdentifier identifier, char* line, char flusher,
                      void (*callback)(const char*)) {
  if (SQLBuffer.count(identifier) == 0)
    SQLBuffer[identifier] = "";

  const char* response = NULL;
  int length = strlen(line);
  if (SQLBuffer[identifier].length() > 0 && length > 0)
    SQLBuffer[identifier] += "\n";

  for (int i = 0; i < length; i++) {
    // SQL Command Terminated
    SQLBuffer[identifier].append(1, line[i]);
    if (line[i] == flusher) {
      response = FlushSQL(identifier, false, callback);
      continue;
    }
  }

  return response;
}

const char* FlushSQL(NodeIdentifier identifier, bool blocking,
                     void (*callback)(const char*)) {
  if (SQLBuffer.count(identifier) == 0)
    SQLBuffer[identifier] = "";

  int len = SQLBuffer[identifier].length();
  if (len > 1) {
    MessageProto* proto_msg = new MessageProto();
    proto_msg->set_destination_node(node_id);
    proto_msg->set_destination_channel("external");
    proto_msg->set_type(MessageProto::CLIENT_QUERY);
    proto_msg->add_data(SQLBuffer[identifier]);

    string* msg_str = new string();
    proto_msg->SerializeToString(msg_str);
    zmq::message_t
      msg(const_cast<char*>(msg_str->c_str()), msg_str->length(), NULL, NULL);
    try {
      calvin_sockets[identifier]->send(msg);
    } catch(...) {
      Log(ERROR, "could not send the message along app socket\n");
    }
  }

  if (callback)
    callback(SQLBuffer[identifier].c_str());
  ResetSQLBuffer(identifier);
  return NULL;
}

bool SQLCommandStarted(NodeIdentifier identifier) {
  return (SQLBuffer[identifier].length() != 0);
}

void ResetSQLBuffer(NodeIdentifier identifier) {
  SQLBuffer[identifier] = "";
}

}  // namespace CalvinAPI
