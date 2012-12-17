/// @file
/// @author Anton Petrov <anton.petrov@yale.edu>
/// @author Michael Giuffrida <michael.giuffrida@yale.edu>
/// @version 0.1
/// @since 20 Apr 2012
///
/// @section DESCRIPTION
///
/// Implementation file of the SQLDriver class.

#include "query_processor/flex_bison/sqldriver.h"

SQLDriver::SQLDriver()
  : trace_scanning_(false), trace_parsing_(false), show_parsing_(false) {
  result_ = NULL;
}

SQLDriver::~SQLDriver() {
  delete result_;
}

int SQLDriver::Parse(const std::string &stmt) {
  stmt_ = stmt;
  file_.clear();

  ScanBegin();

  yy::SQLParser parser(*this, show_parsing_);
  parser.set_debug_level(trace_parsing_);
  int res = parser.parse();

  ScanEnd();
  return res;
}

int SQLDriver::ParseFile(const std::string &file) {
  file_ = file;
  stmt_.clear();

  ScanBegin();

  yy::SQLParser parser(*this, show_parsing_);
  parser.set_debug_level(trace_parsing_);
  int res = parser.parse();

  ScanEnd();
  return res;
}

void SQLDriver::Error(const yy::location& loc, const std::string& msg) {
  LOG(ERROR) << loc << ": " << msg;
}

void SQLDriver::Error(const std::string& msg) {
  LOG(ERROR) << msg;
}

void SQLDriver::Error(const yy::location& loc, const char* msg, ...) {
  LOG(ERROR) << loc << ": " << msg << "\nInvalid query syntax";
  exit(1);
}
