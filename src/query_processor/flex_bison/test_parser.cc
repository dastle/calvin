/// @file
/// @author Anton Petrov <anton.petrov@yale.edu>
/// @author Michael Giuffrida <michael.giuffrida@yale.edu>
/// @version 0.1
/// @since 20 Apr 2012
///
/// @section DESCRIPTION
///
/// This file serves as a test binary used to see whether the lexer and parser
/// are all working correctly.

#include <stdio.h>
#include "query_processor/flex_bison/sqldriver.h"

int main(int argc, char* argv[]) {
  SQLDriver driver;
  for (++argv; argv[0]; ++argv) {
    if (*argv == std::string("-p"))
      driver.trace_parsing_ = true;
    else if (*argv == std::string("-s"))
      driver.trace_scanning_ = true;
    else if (*argv == std::string("-d"))
      driver.show_parsing_ = true;
    else if (!driver.ParseFile(*argv))
      printf("SQL parsing worked\n");
  }
  return 0;
}
