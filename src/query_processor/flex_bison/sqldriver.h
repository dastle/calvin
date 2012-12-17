/// @file
/// @author Anton Petrov <anton.petrov@yale.edu>
/// @author Michael Giuffrida <michael.giuffrida@yale.edu>
/// @version 0.1
/// @since 20 Apr 2012
///
/// @section DESCRIPTION
///
/// Parser driver class used for handling the parsing context and acting as a
/// wrapper on the automatically generated SQLParser class and SQLParser()
/// method.

#ifndef _DB_QUERY_PROCESSOR_FLEX_BISON_SQLDRIVER_H_
#define _DB_QUERY_PROCESSOR_FLEX_BISON_SQLDRIVER_H_

#include <glog/logging.h>
#include <stdio.h>
#include <string>
#include "query_processor/flex_bison/sqlparser.tab.hpp"

using calvin::ASTRoot;

using std::string;
using yy::location;

// Tell Flex the lexer's prototype ...
#define YY_DECL                                        \
  yy::SQLParser::token_type                  \
  yylex(yy::SQLParser::semantic_type* yylval,      \
         yy::SQLParser::location_type* yylloc,      \
         SQLDriver& driver)
// ... and declare it for the parser's sake.
YY_DECL;

/// @class SQLDriver
/// @brief SQLDriver handles the parsing context and servers as a wrapper
///        for the automatically generated SQLParser class
class SQLDriver {
 public:
  /// Constructor for SQLDriver initializes the SQLDriver
  SQLDriver();

  /// Virtual destructor for SQLDriver frees result
  virtual ~SQLDriver();

  /// Initialize the scanner
  void ScanBegin();

  /// Finish scanning
  void ScanEnd();

  /// Run the parser on the input.
  ///
  /// @param      stmt    SQL statement to parse
  ///
  /// @returns    0 on success, non-zero otherwise
  int Parse(const string& stmt);

  /// Run the parser on the contents of a file.
  ///
  /// @param      file    Filename to read in, or "-" for stdin
  ///
  /// @returns    0 on success, non-zero otherwise
  int ParseFile(const string& file);

  /// Whether to output debugging messages during scanning
  bool trace_scanning_;

  /// Whether to output debugging messages during parsing
  bool trace_parsing_;

  /// Whether to output parsing results during parsing
  bool show_parsing_;

  /// Filename to read input from, or "-" for stdin
  string file_;

  /// Statement to parse
  string stmt_;

  /// Result of parsing
  ASTRoot* result_;

  /// Handle errors
  ///
  /// @param      loc     Location identifier
  /// @param      msg     Message to output
  void Error(const location& loc, const string& msg);

  /// Handle errors
  ///
  /// @param      loc     Location identifier
  /// @param      msg     Message to output
  void Error(const location& loc, const char* msg, ...);

  /// Handle errors
  ///
  /// @param      msg     Message to output
  void Error(const string& msg);
};

#endif  // _DB_QUERY_PROCESSOR_FLEX_BISON_SQLDRIVER_H_
