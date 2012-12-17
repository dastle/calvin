#include <gtest/gtest.h>
#include <glog/logging.h>
#include <python/Python.h>

#include <string>

using std::string;

TEST(PythonTest, SimpleString) {
  // Initialize the Python interpreter.  Required.
  Py_Initialize();

  // Execute some Python statements (in module __main__)
  string command("print \"hello\"\n");
  PyRun_SimpleString(command.c_str());
}

int main(int argc, char **argv) {
  google::InitGoogleLogging(argv[0]);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

