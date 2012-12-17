/// @file
/// @author Thaddeus Diamond <diamond@cs.yale.edu>
/// @version 0.1
/// @since 26 Jan 2012
///
/// @section DESCRIPTION
///
/// This is the API for Calvin.  It should be included in any program that
/// is making calls to send SQL to the database
///
/// @attention ZeroMQ is a dependency (zguide.zeromq.org)
/// @attention Protocol Buffers is a dependency (code.google.com/p/protobuf)

#ifndef _DB_INCLUDE_CALVIN_H_
#define _DB_INCLUDE_CALVIN_H_

#define DEBUG_MODE

#include <zmq.hpp>
#include <ctype.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <cstring>
#include <string>
#include <utility>
#include <map>

#include "proto/message.pb.h"

namespace CalvinAPI {

using std::string;
using std::pair;
using std::map;

typedef pair<string, int> NodeIdentifier;

/// @enum  Level
/// @brief What level of log message you want to print out
enum Level {
  OUTPUT = 0,
  ERROR,
  DEBUG,
};

/// SIMPLISTIC LOG FUNCTION
/// This logs the arguments (specified by a specific format), to stdout.  If
/// the level is ERROR, the executable's name is printed out along with the
/// message.  If the level is DEBUG, the message is only printed out when there
/// is a macro defining DEBUG_MODE.  If the level is OUTPUT, it is printed out
/// normally.
///
/// THIS IS FOR USE ONLY BY OTHER FUNCTIONS IN THE CALVIN API
///
/// @param    level         The level of output
/// @param    format        The format to print out to
/// @param    ...           Arguments of the format
void Log(Level level, const char* format, ...);

/// OPEN COMMUNICATION LINE TO CALVIN
/// This opens a communication to Calvin to a specific host/port/node.  If this
/// connection already exists, the function returns true and exits.  Otherwise,
/// it attempts to make the connection and reports back the status.
///
/// @param      host        The host to connect to
/// @param      port        The port number to open the connection on
/// @param      node        Which node to send the message to
///
/// @returns    Pointer to ID of the node connected to (null if no connection)
NodeIdentifier* ConnectToDatabase(const char* host, int port, int node);

/// BUFFER SQL COMMAND TO CALVIN
/// This buffers a SQL command to Calvin, but does not necessarily send it.
/// Sending occurs via FlushSQL, which is called implicitly when a flusher char
/// is seen in the text.
///
/// @param      identifier  The unique ID for the node to buffer SQL to
/// @param      line        The character array to be buffered in outgoing SQL
/// @param      flusher     The character to flush the buffer on
/// @param      callback    The function to be called when the SQL is flushed
///
/// @returns    String response from Calvin (if any)
const char* BufferSQL(NodeIdentifier identifier, char* line, char flusher,
                      void (*callback)(const char*));

/// SEND SQL COMMAND TO CALVIN
///
/// @param      identifier  The unique ID for the node to buffer SQL to
/// @param      blocking    Should the program block waiting for a response
/// @param      callback    The function to be called right after flush
///
/// @returns    String response from Calvin (blocking)
const char* FlushSQL(NodeIdentifier identifier, bool blocking,
                     void (*callback)(const char*));

/// CHECK IF SQL COMMAND IS STARTED
///
/// @param      identifier  The unique ID for the node to buffer SQL to
///
/// @returns    True if there is anything in the SQL buffer, false o/w
bool SQLCommandStarted(NodeIdentifier identifier);

/// RESET SQL BUFFER
///
/// @param      identifier  The unique ID for the node to buffer SQL to
void ResetSQLBuffer(NodeIdentifier identifier);

}  // namespace CalvinAPI

#endif  // _DB_INCLUDE_CALVIN_H_
