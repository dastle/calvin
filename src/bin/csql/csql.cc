/// @file
/// @author Thaddeus Diamond <diamond@cs.yale.edu>
/// @version 0.1
/// @since 26 Jan 2012
///
/// @section DESCRIPTION
///
/// This is a sample client application that uses calls to the calvin API
/// to connect to a Calvin instance and send SQL/SP calls

#define PRINT_BUFSIZE 4096

#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <setjmp.h>
#include <signal.h>
#include <string>

#include "include/calvin.h"

using std::string;

/// GLOBAL VARIABLES
///
/// Updated: 10 April 2012
/// Thaddeus Diamond
const char* DefaultHost = "localhost";
int DefaultPort = 50976;
int DefaultNodeID = 0;
const char* Host = DefaultHost;
int Port = DefaultPort;
int NodeID = DefaultNodeID;
CalvinAPI::NodeIdentifier* identifier;

/// TRIM LEADING AND TRAILING WHITESPACE
/// This copies the char* specified by buffer into a new array of characters
/// and returns the new array.  It does not free, you must explicitly manage
/// both copies of memory.
///
/// @param    buffer      A buffer of characters to be trimmed (null-terminated)
///
/// @returns  The beginning of the newly trimmed buffer
char* Trim(char* buffer) {
  if (!buffer)
    return NULL;
  int length = strlen(buffer);

  // Trailing
  for (int i = length - 1; i >= 0; i--) {
    if (isspace(buffer[i]))
      buffer[i] = '\0';
    else
      break;
  }

  // Leading
  int offset;
  for (offset = 0; offset < length; offset++) {
    if (!isspace(buffer[offset]))
      break;
  }

  // Copy to avoid mem management issues
  length = strlen(buffer + offset);
  char* copy = new char[length + 1];
  int i;
  for (i = 0; i < length; i++)
    copy[i] = buffer[offset + i];
  copy[i] = '\0';

  return copy;
}

/// @enum  Level
/// @brief What level of log message you want to print out
enum Level {
  OUTPUT = 0,
  ERROR,
  DEBUG,
  PAGER,
};

/// SIMPLISTIC LOG FUNCTION
/// This logs the arguments (specified by a specific format), to stdout.  If
/// the level is ERROR, the executable's name is printed out along with the
/// message.  If the level is DEBUG, the message is only printed out when there
/// is a macro defining DEBUG_MODE.  If the level is OUTPUT, it is printed out
/// normally.
///
/// @param    level     The level of output
/// @param    format    The format to print out to
/// @param    ...       Arguments of the format
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

  // PRINT ERROR
  if (level == ERROR) {
    // Get the current process name
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
    fprintf(stdout, "%s: %s", exe_name + last_backslash + 1, buffer);

  // PRINT TO PAGER
  } else if (level == PAGER) {
    const char* pager = getenv("PAGER");
    FILE* output = stdout;
    if (pager && strlen(pager) > 0)
      output = ((output = popen(pager, "w")) ? output : stdout);

    fprintf(output, "%s", buffer);
    if (output != stdout)
      pclose(output);

  // PRINT TO OUTPUT
  } else {
    fprintf(stdout, "%s", buffer);
  }

  va_end(arguments);
  return;
}

/// VERSION PRINTOUT
///
/// Updated: 30 March 2012
/// Thaddeus Diamond
void Version() {
  Log(OUTPUT, "csql (Calvin) 0.1  (c) Yale University 2012\n");
  exit(EXIT_SUCCESS);
}

void Copyright() {
  Log(OUTPUT, "csql (Calvin) 0.1  (c) Yale University 2012\n");
}

/// HELP PRINTOUT
///
/// Updated: 30 March 2012
/// Thaddeus Diamond
void CmdLineHelp() {
  Log(OUTPUT,
        (string("csql is the interactive terminal for a Calvin deployment\n")
          + "\n"
          + "Usage: \n"
          + "  csql [OPTIONS]\n"
          + "\n"
          + "Option flags:\n"
          + "  -c COMMAND       run only a single SQL command, then exit\n"
          + "  -f FILENAME      read in SQL file and execute each, then exit\n"
          + "  --help           print this help message\n"
          + "  --version        print out current system version\n"
          + "\n"
          + "Connection options:\n"
          + "  -h HOSTNAME      database server host (default: %s)\n"
          + "  -p PORT          database server port (default: %d)\n"
          + "  -n NODE          server node id to connect to (default: %d)\n"
          + "\n"
          + "For more info, type \"help\" from within csql\n"
          + "\n"
          + "Report bugs to <calvin@cs.yale.edu>\n").c_str(),
      DefaultHost, DefaultPort, DefaultNodeID);
  exit(EXIT_SUCCESS);
}

void InProgramHelp() {
  Log(OUTPUT,
      (string("You are using csql, the interactive terminal for Calvin\n")
        + "Type: \\copyright for distribution terms\n"
        + "      \\h for help w/available SQL commands\t\t### ANTON/MICHAEL\n"
        + "      \\? for help w/csql internal commands\n"
        + "      ;  to terminate a SQL query\n"
        + "      \\q to quit\n").c_str());
}

void CSQLHelp() {
  Log(PAGER,
      (string("General usage:\n")
        + "\t\\copyright   show cSQL usage and distribution license\n"
        + "\t;            execute query (and send results to file or |pipe)\n"
        + "\t\\h [NAME]    help on syntax of SQL commands, * for all commands\n"
        + "\t\\q           quit cSQL\n"
        + "\nTODO: More here would be great...\n").c_str());
}

/// ILLEGAL OPTION SPECIFIED
///
/// Updated: 30 March 2012
/// Thaddeus Diamond
void IllegalArgument(const char* argument) {
  Log(ERROR, "invalid option or missing argument (%s)\n", argument);
  Log(OUTPUT, "Try \"calvin_ctl --help\" for information\n");
  exit(EXIT_SUCCESS);
}

/// CONNECT TO THE DATABASE WITH ERROR HANDLING
///
/// Updated: 31 March 2012
/// Thaddeus Diamond
void ConnectToDB() {
  if (!(identifier = CalvinAPI::ConnectToDatabase(Host, Port, NodeID))) {
    Log(ERROR, "can't connect to Calvin on specified host:port\n");
    exit(EXIT_FAILURE);
  }
}

/// PROCESS AN INDIVIDUAL LINE OF INPUT
///
/// Updated: 31 March 2012
/// Thaddeus Diamond
const char* ProcessLine(char* line) {
  // Null checking
  if (!line)
    return NULL;

  // Copy first part of command to buffer
  char cmd[PRINT_BUFSIZE];
  int i, length = strlen(line);
  for (i = 0; i < length; i++) {
    if (isspace(line[i]))
      break;
    cmd[i] = line[i];
  }
  cmd[i] = '\0';

  // INTERNAL COMMAND HELP
  if (!strcmp(line, "\\?"))
    CSQLHelp();

  // QUIT THE PROGRAM
  else if (!strcmp(line, "\\q"))
    exit(EXIT_SUCCESS);

  // COPYRIGHT
  else if (!strcmp(cmd, "\\copyright"))
    Copyright();

  // INVALID INTERNAL COMMAND
  else if (cmd[0] == '\\')
    Log(OUTPUT, "Invalid command %s. Try \\? for help\n", cmd);

  // GENERAL HELP
  else if (!CalvinAPI::SQLCommandStarted(*identifier) && !strcmp(cmd, "help"))
    InProgramHelp();

  // ADD TO SQL BUFFER
  else
    return CalvinAPI::BufferSQL(*identifier, line, ';', add_history);

  // Extra handling for internal Calvin commands
  if (cmd[0] == '\\') {
    CalvinAPI::ResetSQLBuffer(*identifier);
    if (strcmp(cmd, line))
      Log(OUTPUT, "Ignoring extra input after %s\n", cmd);
  }

  return NULL;
}

/// SINGLE COMMAND OPERATION
///
/// Updated: 30 March 2012
/// Thaddeus Diamond
void RunSingleCommand(char* line) {
  ConnectToDB();

  const char* first_result = ProcessLine(Trim(line));
  if (first_result)
    Log(OUTPUT, "%s", first_result);

  const char* second_result = CalvinAPI::FlushSQL(*identifier, true, NULL);
  if (second_result)
    Log(OUTPUT, "%s", second_result);

  exit(EXIT_SUCCESS);
}

/// SQL FILE OPERATION
///
/// Updated: 30 March 2012
/// Thaddeus Diamond
void RunFromFile(const char* filename) {
  FILE* file = fopen(filename, "r");
  if (!file) {
    Log(ERROR, "error reading file '%s'\n", filename);
    exit(EXIT_FAILURE);
  }

  ConnectToDB();
  char buffer[PRINT_BUFSIZE];
  while (fgets(buffer, PRINT_BUFSIZE, file)) {
    const char* result = ProcessLine(Trim(buffer));
    if (result)
      Log(OUTPUT, "%s", result);
  }
}

/// COMMAND PROMPT
///
/// Updated: 30 March 2012
/// Thaddeus Diamond

// Be able to interrupt mid-type
sigjmp_buf interrupt_jump;
void ResetCommandPrompt(int signal) {
  siglongjmp(interrupt_jump, 1);
}

void RunCommandPrompt() {
  // Set up data structures and basic output
  ConnectToDB();
  Log(OUTPUT, "csql (0.1)\nType \"help\" for help.\n\n");

  // Got here via jump, reset the state
  if (sigsetjmp(interrupt_jump, 1)) {
    Log(OUTPUT, "\n");
    CalvinAPI::ResetSQLBuffer(*identifier);
  }

  // PROMPT USER
  do {
    char* line = Trim(readline("==calvin== "));
    if (!line) {
      Log(OUTPUT, "\n");
      break;
    }

    const char* result = ProcessLine(line);
    delete[] line;
    if (result)
      Log(OUTPUT, "%s", result);
  } while (true);
}

/// PARSE ARGUMENTS
///
/// Updated: 28 March 2012
/// Thaddeus Diamond
int ParseArguments(int argc, char* argv[]) {
  char* command_or_filename = NULL;
  bool is_command = false;
  for (int i = 1; i < argc; i++) {
    // HELP
    if (!strcmp(argv[i], "--help")) {
      CmdLineHelp();

    // VERSION
    } else if (!strcmp(argv[i], "--version")) {
      Version();

    // SINGLE COMMAND
    } else if (i == argc - 1) {
      IllegalArgument(argv[i]);

    // SINGLE COMMAND
    } else if (!strcmp(argv[i], "-c")) {
      is_command = true;
      command_or_filename = argv[++i];

    // COMMANDS FROM FILE
    } else if (!strcmp(argv[i], "-f")) {
      is_command = false;
      command_or_filename = argv[++i];

    // CHANGE HOSTNAME
    } else if (!strcmp(argv[i], "-h")) {
      Host = argv[++i];

    // CHANGE PORT
    } else if (!strcmp(argv[i], "-p")) {
      Port = atoi(argv[++i]);

    // NODE TO CONNECT TO
    } else if (!strcmp(argv[i], "-n")) {
      NodeID = atoi(argv[++i]);

    // ILLEGAL OPTION
    } else {
      IllegalArgument(argv[i]);
    }
  }

  // Special modes of operation
  if (command_or_filename && is_command)
    RunSingleCommand(command_or_filename);
  else if (command_or_filename)
    RunFromFile(command_or_filename);

  return 0;
}

/// MAIN METHOD
///
/// Updated: 30 March 2012
/// Thaddeus Diamond
int main(int argc, char* argv[]) {
  ParseArguments(argc, argv);

  signal(SIGINT, ResetCommandPrompt);
  RunCommandPrompt();

  exit(EXIT_SUCCESS);
}
