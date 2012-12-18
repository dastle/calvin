#!/usr/bin/python
"""
    @file
    @author     Thaddeus Diamond <diamond@cs.yale.edu>
    @version    0.1
    @since      9 April 2012

    @section    DESCRIPTION

    This program launches a set of Calvin nodes based on the configuration file
    specified.  It assumes a shared filesystem where the configuration file
    can be accessed in the same place (and data can be written to) each time.
"""

import sys
import os

"""
  GLOBAL CONFIGURATION VARIABLES
"""
EXECUTABLE  = "bin/bin/calvin_ctl/calvin_ctl"
ROOT_DIR    = "../../.."                         # Relative to executable
CURRENT_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), ROOT_DIR)

"""
  Print out help messages, version messages, and error messages
"""
def Help():
    print """calvin_cluster is a utility to deploy Calvin onto an entire cluster

Usage:
  ./calvin_cluster <cmd> <calvin_dir>

Option Meanings:
  cmd \t\t\tAction to be performed (start, stop, restart, status, or kill)
  calvin_dir\t\tShared directory containing config and data files for cluster

Report bugs to <calvin@cs.yale.edu>"""

def Version():
    print "calvin_cluster (Calvin) 0.1 (c) Yale University 2012"

def ErrorProcessing():
    print "calvin_cluster: Invalid argument use\nTry './calvin_cluster --help'"

"""
  Process lines in a file and generate the actual command
"""
def ProcLine(line):
    line = line.strip()
    parts = line[0:line.find('#')].split(':')
    return parts

def FormCmds(nodes, cmd, dir):
    good_nodes = []
    for node in nodes:
        # Empty node
        if len(node) == 1 and len(node[0]) == 0:
            continue

        # Well-formed node
        elif len(node) == 5:
            host = node[3]
            if not node[0].startswith("node") or node[0].find('=') == -1:
              raise ValueError
            node_id = node[0].split('=')[0][4:]

            if cmd == "kill":
                shell = 'ssh {} "killall -9 calvinmaster"'.format(host)
            else:
                shell = 'ssh {} "cd {}; {} {} {} -D \'{}\'; sleep 60" &'.format(
                        host, CURRENT_DIR, EXECUTABLE, cmd, node_id, dir)
            good_nodes.append(shell)

        # Poorly formed node
        else:
            raise ValueError
    return good_nodes

"""
  Form commands for each node in the configuration file and ssh/execute
"""
def DeployNodes(cmd, dir):
    fname = dir + "/calvin.conf"
    try:
        shells = FormCmds(map(ProcLine, open(fname, "r").readlines()), cmd, dir)
        for shell in shells:
            os.system(shell)
    except ValueError:
        print "calvin_cluster: Badly formed config file '" + filename + "'"
    except IOError:
        print "calvin_cluster: Error with file I/O on '" + filename + "'"

"""
  Process command line arguments
"""
def ProcessArgs():
    if len(sys.argv) == 2 and sys.argv[1] == "--help":
        Help()
    elif len(sys.argv) == 2 and sys.argv[1] == "--version":
        Version()
    elif len(sys.argv) != 3:
        ErrorProcessing()
    else:
        DeployNodes(sys.argv[1], sys.argv[2])

"""
  MAIN METHOD
"""
if __name__ == "__main__":
    ProcessArgs()
