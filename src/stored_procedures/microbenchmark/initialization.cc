/// @file
/// @author Thaddeus Diamond <diamond@cs.yale.edu>
/// @version 0.1
/// @since 26 Jan 2012
///
/// @section DESCRIPTION
///
/// This file initializes storage for use in the microbenchmark application

#include "stored_procedures/microbenchmark/microbenchmark.h"

namespace calvin {

void Microbenchmark::InitializeApplication(int nodecount, int hotcount,
                                           int RWSetSize, int numHotKeys) {
  if (application_instance_ == NULL)
    application_instance_
      = new Microbenchmark(nodecount, hotcount, RWSetSize, numHotKeys);
}

void Microbenchmark::InitializeStorage() const {
  Storage* storage = Storage::GetStorageInstance();
  Config* conf = Config::GetConfigInstance();

  srand(static_cast<int>(GetTime()));
  for (int i = 0; i < nparts*kDBSize; i++) {
    if (conf->LookupPartition(IntToString(i)) == conf->this_node_id) {
      char* int_buffer = new char[kValueSize];
      for (int j = 0; j < kValueSize - 1; j++)
        int_buffer[j] = (rand() % 26 + 'a');
      int_buffer[kValueSize - 1] = '\0';
      storage->Put(IntToString(i), new Value(int_buffer, kValueSize), -1);
    }
  }
}

}  // namespace calvin
