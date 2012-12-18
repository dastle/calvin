/// @file
/// @author Thaddeus Diamond  <diamond@cs.yale.edu>
/// @version 0.1
/// @since 26 Jan 2012
///
/// @section DESCRIPTION
///
/// This program launches a durable instance of the Calvin system on the
/// current node.

#define PRINT_BUFSIZE 1024
#define THREAD_WAIT 100000
#define OUTPUT(...) \
  do { \
    fprintf(stdout, __VA_ARGS__); \
    fprintf(stdout, "\n"); \
  } while (0)
#define ERROR(...) \
  do { \
    fprintf(stderr, "calvin_ctl: "); \
    fprintf(stderr, __VA_ARGS__); \
    fprintf(stderr, "\n"); \
  } while (0)

#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <string>

using std::string;

/// VERSION PRINTOUT
///
/// Updated: 28 March 2012
/// Thaddeus Diamond
void Version() {
  OUTPUT("calvin_ctl (Calvin) 0.1  (c) Yale University 2012");
  exit(EXIT_SUCCESS);
}

/// HELP PRINTOUT
///
/// Updated: 28 March 2012
/// Thaddeus Diamond
void Help() {
  OUTPUT(
    (string("calvin_ctl is a utility to start, stop, and report on ")
     + "the status of a Calvin server. \n"
     + "\n"
     + "Usage: \n"
     + "  calvin_ctl start    node [-w] [-D data_dir] [-s]\n"
     + "  calvin_ctl stop     node [-W] [-D data_dir] [-s]\n"
     + "  calvin_ctl restart  node [-w] [-D data_dir] [-s]\n"
     + "  calvin_ctl status   node [-D data_dir]\t\t### JOHN'S FINAL PROJECT\n"
     + "  calvin_ctl kill     SIGNALNAME PID\n"
     + "\n"
     + "Option Meanings: \n"
     + "  node                         ID of node in cluster being started\n"
     + "  -D, --calvindata DATADIR     config and data file location\n"
     + "  -s, --silent                 only print errors, no info msgs\n"
     + "  -w                           wait until op completes\n"
     + "  -W                           don't wait until op completes\n"
     + "  --help                       print this help message\n"
     + "  --version                    print out current system version\n"
     + "(Defaults: wait on stop, don't wait on start/restart, not silent)\n"
     + "\n"
     + "Allowed signal names for kill:\n"
     + "  HUP INT QUIT ABRT TERM USR1 USR2\n"
     + "\n"
     + "Report bugs to <calvin@cs.yale.edu>").c_str());
  exit(EXIT_SUCCESS);
}


/// ARGUMENT PARSER
///
/// Updated: 28 March 2012
/// Thaddeus Diamond
enum {
  NONE = 0,
  NULL_OPT,
  AMBIGUOUS_OPS,
  BAD_FLAG,
  HELP,
  VERSION,
  KILL,
  START,
  STOP,
  RESTART,
  STATUS,
  NUMBER_OF_OPTIONS,
};

enum {
  UNSET = 0,
  TRUE,
  FALSE,
};

int ShouldWait = UNSET;
char* Directory = getenv("CALVINDATA");
int Node = -1;

int ParseArguments(int argc, char* argv[], char** arg_buffer) {
  int arg_index = 0;

  // Help option used
  if (argc > 1 && !strcmp(argv[1], "--help")) {
    return HELP;

  // Version option used
  } else if (argc > 1 && !strcmp(argv[1], "--version")) {
    return VERSION;

  // Kill command used
  } else if (argc > 1 && !strcmp(argv[1], "kill")) {
    if (argc == 4)
      return KILL;
    ERROR("wrong number of arguments for kill mode");
    return AMBIGUOUS_OPS;
  }

  // Iterate through all cmd line args
  int option = NONE;
  int index = 0;
  while (++index < argc) {
    // Run-time flag
    if (argv[index][0] == '-') {
      int length = strlen(argv[index]);
      for (int i = 1; i < length; i++) {
        // Secondary specification
        char flag = argv[index][i];
        if (!strcmp(argv[index], "--calvindata"))
          (flag = 'D') && (i = length);
        else if (!strcmp(argv[index], "--silent"))
          (flag = 's') && (i = length);

        // Waits and silents are handled simplest (last always has precedence)
        if (flag == 'w' || flag == 'W') {
          if (flag == 'w' || flag == 'W')
            ShouldWait = (flag == 'w' ? TRUE : FALSE);
          continue;
        }

        // Add to argument buffer
        char* temp = new char[3];
        (temp[0] = '-') && (temp[1] = flag) && (temp[2] = '\0');
        arg_buffer[arg_index++] = temp;

        // Option flag w/secondary value
        if (flag == 'D') {
          if (index >= argc - 1) {
            ERROR("flag with no parameter (%c)", flag);
            return BAD_FLAG;
          }

          // Directory is special
          if (flag == 'D')
            Directory = argv[index + 1];
          arg_buffer[arg_index++] = argv[++index];

        // Invalid flag
        } else if (flag != 's') {
          ERROR("bad flag (%c)", flag);
          return BAD_FLAG;
        }
      }

    // Operation argument
    } else {
      // Multiple options
      if (option != NONE) {
        ERROR("too many operations (second is %s)", argv[index]);
        return AMBIGUOUS_OPS;

      // Start
      } else if (!strcmp(argv[index], "start")) {
        ShouldWait = (ShouldWait == UNSET ? FALSE : ShouldWait);
        option = START;

      // Stop
      } else if (!strcmp(argv[index], "stop")) {
        ShouldWait = (ShouldWait == UNSET ? TRUE : ShouldWait);
        option = STOP;

      // Restart
      } else if (!strcmp(argv[index], "restart")) {
        ShouldWait = (ShouldWait == UNSET ? FALSE : ShouldWait);
        option = RESTART;

      // Status
      } else if (!strcmp(argv[index], "status")) {
        option = STATUS;

      // Invalid option
      } else {
        ERROR("invalid option (%s)", argv[index]);
        return NULL_OPT;
      }

      // No node specified
      if (index >= argc - 1) {
        ERROR("this is a parallel database, which node is it?");
        return BAD_FLAG;
      }

      arg_buffer[arg_index++] = const_cast<char*>("--node");
      arg_buffer[arg_index++] = argv[++index];

      // Check if node ID even valid
      char* extra = NULL;
      Node = strtol(argv[index], &extra, 10);
      if (extra && extra[0]) {
        ERROR("invalid [non-numeric] node id (%s)", argv[index]);
        return BAD_FLAG;
      }
    }
  }

  // No arguments specified
  if (option == NONE) {
    ERROR("no operation specified");
    return NONE;
  }

  // Successfully specified an option
  return option;
}


/// MAIN METHOD
///
/// Updated: 28 March 2012
/// Thaddeus Diamond
int main(int argc, char* argv[]) {
  // Create an argument buffer and parse out all possible options (non-trivial)
  char* arg_buffer[2 * argc];
  for (int i = 0; i < 2 * argc; i++)
    arg_buffer[i] = NULL;
  int prog_op = ParseArguments(argc, argv, arg_buffer);

  // HELP
  if (prog_op == HELP) {
    Help();

  // VERSION
  } else if (prog_op == VERSION) {
    Version();

  // KILL SIGNAL
  } else if (prog_op == KILL) {
    int signal = SIGCONT;
    char* sig_name = argv[2];
    if (!strcmp(sig_name, "HUP"))
      signal = SIGHUP;
    else if (!strcmp(sig_name, "INT"))
      signal = SIGINT;
    else if (!strcmp(sig_name, "QUIT"))
      signal = SIGQUIT;
    else if (!strcmp(sig_name, "ABRT"))
      signal = SIGABRT;
    else if (!strcmp(sig_name, "TERM"))
      signal = SIGTERM;
    else if (!strcmp(sig_name, "USR1"))
      signal = SIGUSR1;
    else if (!strcmp(sig_name, "USR2"))
      signal = SIGUSR2;

    char* extra = NULL;
    int proc_id = strtol(argv[3], &extra, 10);
    if (signal == SIGCONT) {
      ERROR("unknown signal (%s) specified", sig_name);
    } else if (proc_id <= 0 || (extra && extra[0])) {
      ERROR("process id (%s) is invalid", argv[3]);
    } else if (kill(proc_id, signal) < 0) {
      char buf[PRINT_BUFSIZE];
      snprintf(buf, sizeof(buf), "%s couldn't send signal %d (PID: %d)\n",
               "calvin_ctl:", signal, proc_id);
      perror(buf);
    } else {
      exit(EXIT_SUCCESS);
    }
  }

  // ERROR HANDLING
  if (prog_op == BAD_FLAG || prog_op == NONE || prog_op == NULL_OPT
      || prog_op == AMBIGUOUS_OPS || prog_op == KILL || !Directory) {
    if (!Directory && prog_op >= START)
      ERROR("no DB dir specified and env var CALVINDATA null");
    OUTPUT("Try \"calvin_ctl --help\" for information");
    exit(EXIT_FAILURE);
  }

  // MAIN PROCESS FORK
  int status, pid = fork();
  if (pid < 0) {
    exit(EXIT_FAILURE);

  // PARENT PROCESS
  } else if (pid > 0) {
    if (ShouldWait == TRUE)
      waitpid(pid, &status, 0);
    exit(EXIT_SUCCESS);

  // CHILD PROCESS
  } else {
    // CURRENT CALVINMASTER
    char buf[PRINT_BUFSIZE];
    int proc_id = -1;
    if (prog_op != START) {
      snprintf(buf, sizeof(buf), "%s/calvinmaster_%d.pid", Directory, Node);

      FILE* lock_file = fopen(buf, "r");
      if (!lock_file)
        ERROR("can't open file '%s'\nIs calvin running?", buf);
      else
        proc_id = atoi(fgets(buf, sizeof(buf), lock_file));
    }

    // STOP (RESTART)
    if (proc_id > 0 && (prog_op == STOP || prog_op == RESTART)) {
      if (kill(proc_id, SIGTERM) < 0) {
        snprintf(buf, sizeof(buf), "%s couldn't send signal %d (PID: %d)\n",
                 "calvin_ctl:", SIGTERM, proc_id);
        perror(buf);
      }
    }

    // RESTART
    if (proc_id > 0 && prog_op == RESTART) {
      while (!kill(proc_id, 0))
        usleep(THREAD_WAIT);
    }

    // START (RESTART)
    if (prog_op == START || prog_op == RESTART) {
      char proc_dir[PRINT_BUFSIZE], new_exe[PRINT_BUFSIZE],
           exe_name[PRINT_BUFSIZE] = "master";
      snprintf(proc_dir, sizeof(proc_dir), "/proc/%d/exe", getpid());
      int bytes = readlink(proc_dir, new_exe, sizeof(new_exe));
      for (unsigned int i = 0; i <= strlen(exe_name); i++)
        new_exe[bytes + i - 4] = exe_name[i];

      OUTPUT("calvinmaster starting");
      execv(new_exe, arg_buffer);
    }

    // STATUS
    if (prog_op == STATUS) {
      /// @TODO: IMPLEMENT THIS!!!
      ///   Plug into John Langhauser's final project
    }
  }

  exit(prog_op);
}
