/// @file
/// @author Alexander Thomson <thomson@cs.yale.edu>
/// @author Thaddeus Diamond <diamond@cs.yale.edu>
/// @version 0.1
/// @since 26 Nov 2011
///
/// @section DESCRIPTION
///
/// This file defines the Value type, which is to be stored in the database

#ifndef _DB_BACKEND_VALUE_H_
#define _DB_BACKEND_VALUE_H_

#include <google/protobuf/message.h>
#include <cstring>
#include <cstdio>
#include <string>

using std::string;
using google::protobuf::Message;

namespace calvin {

/// @class Value
/// @brief Storage layer values (i.e. records in the table)
///
/// Only a class of type CalvinStorage will have access to the use of a Val
/// class because this is an internal means for accessing the data layer.
/// All retrievals by an application should go through the
/// TransactionManager, and all communications from the partner classes
/// (i.e. Checkpoint, DiskStorage, Connection, etc.) need to go through the
/// actual database
class Value {
 public:
  /// The primary constructor for val takes a pointer to a buffer of data and
  /// the size of that data.  This makes the assumption that it is in
  /// serialized form (packed).
  ///
  /// @param       data    A pointer to a buffer for storing in the Val object
  /// @param       size    The size of the buffer pointed to in data
  ///
  /// @returns     A new Val object that points to a serialized buffer
  Value(void* data, int size) : data_(data), size_(size) {}

  /// We overload the val constructor to take a Value type because this
  /// paradigm occurs a lot and otherwise we're going to have to do work
  /// every time to make the conversion work.
  ///
  /// @param        data     The serialized version of the record
  ///
  /// @returns      A new Val object that points to a serialized buffer
  explicit Value(string data) : size_(data.length()) {
    const char* serialized_chars = data.c_str();
    char* mutable_chars = new char[MAX(11, data.length() + 1)];
    memcpy(mutable_chars, serialized_chars, data.length() + 1);

    data_ = reinterpret_cast<void*>(mutable_chars);
  }

  /// The destructor for Value frees a la C b/c C++ doesn't know how for void*
  ~Value() {}

  /// Accessor for the actual data
  inline void* data() { return data_; }

  /// Accessor for the size in bytes (if serialized, 0 otherwise)
  inline int size() { return size_; }

  /// Return a string containing the Value's data (for debugging purposes only)
  inline string ToString() {
    string s(reinterpret_cast<char*>(data_), size_);
    return s;
  }

 private:
  /// The constructor for the Val class is private, b/c we want to disallow
  /// a default constructor
  Value() {}

  /// The copy constructor for the Val class is disallowed, explicit copies
  /// must be done
  Value(const Value& v) {}

  /// We represent the actual data, the size of that data buffer, and whether
  /// or not the data is being stored in a serialized form via member vars
  void* data_;
  int   size_;
};

}  // namespace

#endif  // _DB_BACKEND_VALUE_H_
