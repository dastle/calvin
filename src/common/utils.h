/// @file
/// @author Alexander Thomson <thomson@cs.yale.edu>
/// @author Thaddeus Diamond <diamond@cs.yale.edu>
/// @version 0.1
/// @since 26 Jan 2012
///
/// @section DESCRIPTION
///
/// Utility library
///
/// @TODO(alex): MORE/BETTER UNIT TESTING!

#ifndef _DB_COMMON_UTILS_H_
#define _DB_COMMON_UTILS_H_

#include <glog/logging.h>
#include <dirent.h>
#include <assert.h>
#include <sys/time.h>
#include <sys/stat.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <utility>
#include <vector>
#include <algorithm>

#include "common/types.h"

using std::string;
using std::pair;
using std::vector;

#define ASSERTS_ON true

/// CVarint is a variable-length encoding of integers, depending on the size.
/// This is based on google Varint library, which I couldn't find, although I
/// found public references to it, and I think that protobufs uses it. See
/// https://developers.google.com/protocol-buffers/docs/encoding for google's
/// documentation on how Varints work.
///
/// The general idea here, though is this. Say you want to encode a sequence of
/// uint64s (most of which are actually pretty small) into a blob. Here's what
/// you do:
///
///   // Create a string to store the encoded sequence.
///   string s;
///   // Append some numbers.
///   CVarint::Append64(&s, 91);
///   CVarint::Append64(&s, 119);
///   CVarint::Append64(&s, 267);
///
///   // Now let's read these numbers back.
///   uint64 x, y, z;
///   const char* pos = s.data();  // Tracks our current offset into the string.
///   pos = CVarint::Parse64(pos, &x);  //  x now stores 91
///   pos = CVarint::Parse64(pos, &y);  //  y now stores 119
///   pos = CVarint::Parse64(pos, &z);  //  z now stores 267
///
/// @TODO(Alex): Find Google implementation of this to use instead; it's
///              probably way better!
class CVarint {
 public:
  /// Appends a variable-length encoding of x to *s.
  static void Append64(string* s, uint64 x);

  /// Reads the next var-length encoded value into *x.
  static const char* Parse64(const char* pos, uint64* x);
};

/// @TODO(Alex): Thorough doxygenation
// Status code for return values.
struct Status {
  // Represents overall status state.
  enum Code {
    ERROR = 0,
    OKAY = 1,
    DONE = 2,
  };
  Code code;

  // Optional explanation.
  string message;

  // Constructors.
  explicit Status(Code c) : code(c) {}
  Status(Code c, const string& s) : code(c), message(s) {}
  static Status Error() { return Status(ERROR); }
  static Status Error(const string& s) { return Status(ERROR, s); }
  static Status Okay() { return Status(OKAY); }
  static Status Done() { return Status(DONE); }

  // Pretty printing.
  string ToString() {
    string out;
    if (code == ERROR) out.append("Error");
    if (code == OKAY) out.append("Okay");
    if (code == DONE) out.append("Done");
    if (message.size()) {
      out.append(": ");
      out.append(message);
    }
    return out;
  }
};

/// Time accessor function
///
/// @returns    Double representing number of seconds since midnight according
///             to local system time, to the nearest microsecond.
double GetTime();

/// Setter for global start times
///
/// @returns    True if the time was successfully set from an unset state,
///             false o/w
bool SetGlobalStartTime();

/// Accessor for global start time
///
/// @returns    The global start time as initially set by SetGlobalStartTime
double GlobalStartTime();

/// We provide a custom function to sort the file names when we collapse
/// checkpoints
///
/// @param    one   The first of two file pairs to compare
/// @param    two   The second of two file pairs to compare
///
/// @returns  True or false based on comparison
bool SortFiles(pair<int, string> one, pair<int, string> two);

/// When accessing checkpoints in the checkpoint log we provide a quick way
/// to return a list of files
///
/// @param    directory   The directory to search for files in
/// @param    filter      A filter to put on files looked up
/// @param    files       The list of pair<int, string> to populate
///
/// @returns  True unless the directory does not exist
bool GetFileListing(const char* directory, const char* filter,
                    vector<pair<int, string> >* files);

/// We provide a few useful hash functions that can be used throughout. The
/// first one is the FNV-1 hash as described by Fowler-Noll-Vo.
///
/// @param      key     The key to be hashed into a resultant int
///
/// @returns    The hashed int value
unsigned int FNVHash(const string& key);

/// The second one is the FNV-1a hash as described by Fowler-Noll-Vo.
///
/// @param      key     The key to be hashed into a resultant int
///
/// @returns    The hashed int value
unsigned int FNVModHash(const string& key);

/// When recovering from checkpoints we want to be able to move memory around
/// to a new, permanent allocation.
///
/// @param      buffer  An arbitrary byte buffer to be copied
/// @param      length  Length of the buffer to be copied
///
/// @returns    A pointer to the newly copied, heap-allocated buffer.  NULL
///             if unsuccessful
void* CreatePermBuffer(const void* buffer, int length);

/// We provide a utility for turning 4 character bits (in order) into an int
///
/// @param      byte_array    Byte array to have four bytes converted to int
///
/// @returns    Signed integer representation of four bytes
int ByteArrayToInt(const char* byte_array);

/// We provide a wrapper in case we want an unsigned int i/o a signed int
///
/// @param      byte_array    Byte array to have four bytes converted to u_int
///
/// @returns    Unsigned integer representation of four bytes
unsigned int ByteArrayToUnsignedInt(const char* byte_array);
/// We provide a convenient utility to read in a value from a file and null
/// terminate the buffer
///
/// @param      file          FILE handle to be read in
/// @param      buf           Byte buffer to store values in
/// @param      read_len      Length of the read to be performed
///
/// @returns    read_len bytes if the read was successful, false o/w
int ReadFromFile(FILE* file, char* buf, unsigned int read_len);

/// Busy-wait for set 'duration'
///
/// @param        duration      How many seconds (can be decimal) to wait
void Spin(double duration);

/// Busy-wait until GetTime() >= time
///
/// @param        time          What time in future to stop spinning
void SpinUntil(double time);

/// Produces a random alphabet string of the specified length
///
/// @param        length        Length of the string to be produced
///
/// @returns      A string object of specified length
string RandomString(int length);

/// Returns a human-readable string representation of an int.
///
/// @param        n             The int to be converted
///
/// @returns      A string object in human-readable form of said int
string IntToString(int n);

/// Returns a human-readable  representation of an int
///
/// @param        buffer        The byte array to print the value into
/// @param        value         The value (int) to be converted
/// @param        maxlength     The maximum length of the buffer passed in
///
/// @returns      True if the conversion was succesful, false o/w
bool IntToBuffer(char* buffer, int value, int maxlength);

/// Converts a human-readable numeric string to an int.
///
/// @param        s             The string to convert into an int
///
/// @returns      An integer representing the contents of string s
int StringToInt(const string& s);

/// Converts a human-readable numeric sub-string (starting at the 'n'th position
/// of 's') to an int.
///
/// @param        s             The string to convert into an int
/// @param        n             Offset from the beginning at which to convert
///
/// @returns      An integer representing the contents of string s with offset
int OffsetStringToInt(const string& s, int n);

/// Function for deleting a heap-allocated string after it has been sent on a
/// zmq socket connection. E.g., if you want to send a heap-allocated
/// string '*s' on a socket 'sock':
///
///  zmq::message_t msg((void*) s->data(), s->size(), DeleteString, (void*) s);
///  sock.send(msg);
///
/// @param        data          Byte array being sent over the wire
/// @param        hint          Hint buffer as specified by ZeroMQ API
void DeleteString(void* data, void* hint);

/// Do nothing
///
/// @param        data          Byte array, irrelevant
/// @param        hint          Byte array, irrelevant
void Noop(void* data, void* hint);

#endif  // _DB_COMMON_UTILS_H_

