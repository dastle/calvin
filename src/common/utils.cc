/// @file
/// @author Alexander Thomson <thomson@cs.yale.edu>
/// @author Thaddeus Diamond <diamond@cs.yale.edu>
/// @version 0.1
/// @since 26 Jan 2012
///
/// @section DESCRIPTION
///
/// Utility library implementation

#include "common/utils.h"

#include <cstdio>

void CVarint::Append64(string* s, uint64 x) {
  for (int i = 0; x & ~127 && i < 8; i++) {
    // If x takes > 7 bits to encode. Append a 1 followed by the 7 lowest-
    // order bits of x.
    s->append(1, (1<<7) | static_cast<uint8>(x));
    // Then throw away those lowest order bits and repeat for the next 7 bits.
    x >>= 7;
  }
  // What remains of x takes <= 7 bits to encode. Append a 0 followed by the
  // 7 lowest-order bits of x.
  s->append(1, static_cast<uint8>(x));
}

const char* CVarint::Parse64(const char* pos, uint64* x) {
  *x = 0;
  int offset = 0;

  // First check if this encodes to only one byte (possibly a common case?).
  if (!(*pos & (1<<7))) {
    *x = *pos;
    return pos + 1;
  }

  // If not, do it the slow way.
  while ((*pos) & (1<<7) && offset < 56) {
    // Read this byte (except for the first bit into x).
    uint64 bits = *pos & 127;
    *x |= bits << offset;
    pos++;
    // This is NOT the last byte we're reading, so shift everything we've read
    // so far left.
    offset += 7;
  }
  // This is the last byte (thus it starts with 0, so we don't need to
  // bitwise-and it with 127---OR it starts with 1 because the highest order of
  // the top bit started with 1, so that's ok too).
  uint64 bits = *pos;
  *x |= bits << offset;

  // Return a pointer to the next char after the last one that was part of x.
  return pos + 1;
}

double GetTime() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec + tv.tv_usec/1e6;
}

double StartTime = -1;
bool SetGlobalStartTime() {
  if (StartTime < 0) {
    StartTime = GetTime();
    LOG(INFO) << "Set start time to " << StartTime;
    return true;
  }
  return false;
}

double GlobalStartTime() {
  return StartTime;
}

bool SortFiles(pair<int, string> one, pair<int, string> two) {
  return (one.first < two.first);
}

bool GetFileListing(const char* directory, const char* filter,
                                  vector<pair<int, string> >* files) {
  // Check if there is EVEN A DIRECTORY!
  DIR* dir = opendir(directory);
  if (!dir)
    return false;

  // Do the damn thing
  files->clear();
  struct dirent ent, *result;
  while (!readdir_r(dir, &ent, &result) && result != NULL) {
    string file = string(ent.d_name);
    if (filter == NULL || file.find(filter) != string::npos) {
      files->push_back(
        pair<int, string>(strtol(file.c_str(), NULL, 10), file));
    }
  }

  // Cleanup and sorting
  closedir(dir);
  sort(files->begin(), files->end(), SortFiles);
  return (files->size() > 0 ? true : false);
}

unsigned int FNVHash(const string& key) {
  unsigned int hash = 2166136261;                 // FNV Hash offset
  for (unsigned int i = 0; i < key.length(); i++)
    hash = (hash * 1099511628211) ^ key[i];       // H(x) = H(x-1) * FNV' XOR x

  return hash;
}

unsigned int FNVModHash(const string& key) {
  unsigned int hash = 2166136261;                 // FNV Hash offset
  for (unsigned int i = 0; i < key.length(); i++)
    hash = (hash ^ key[i]) * 1099511628211;       // H(x) = H(x-1) * FNV' XOR x

  return hash;
}

void* CreatePermBuffer(const void* buffer, int buffer_length) {
  char* perm_buf = new char[buffer_length + 1];
  memcpy(perm_buf, buffer, buffer_length);
  perm_buf[buffer_length] = '\0';
  return reinterpret_cast<void*>(perm_buf);
}

int ByteArrayToInt(const char* byte_array) {
  return static_cast<int>(
            ((static_cast<unsigned int>(byte_array[0]) & 255) << 24)
          + ((static_cast<unsigned int>(byte_array[1]) & 255) << 16)
          + ((static_cast<unsigned int>(byte_array[2]) & 255) << 8)
          + ((static_cast<unsigned int>(byte_array[3]) & 255) << 0));
}

unsigned int ByteArrayToUnsignedInt(const char* byte_array) {
  return static_cast<unsigned int>(ByteArrayToInt(byte_array));
}

int ReadFromFile(FILE* file, char* buf, unsigned int read_len) {
  if (fread(buf, sizeof(buf[0]), read_len, file) != read_len)
    return -1;
  buf[read_len] = '\0';
  return read_len;
}

void Spin(double duration) {
  usleep(1000000 * duration);
//  double start = GetTime();
//  while (GetTime() < start + duration) {}
}

void SpinUntil(double time) {
  while (GetTime() >= time) {}
}

string RandomString(int length) {
  string random_string;
  for (int i = 0; i < length; i++)
    random_string += rand() % 26 + 'A';

  return random_string;
}

string IntToString(int n) {
  char s[64];
  snprintf(s, sizeof(s), "%d", n);
  return string(s);
}

bool IntToBuffer(char* buffer, int value, int maxlength) {
  if (snprintf(buffer, maxlength, "%d", value) <= maxlength)
    return true;
  return false;
}

int StringToInt(const string& s) {
  return atoi(s.c_str());
}

int OffsetStringToInt(const string& s, int n) {
  return atoi(s.c_str() + n);
}

void DeleteString(void* data, void* hint) {
  delete reinterpret_cast<string*>(hint);
}
void Noop(void* data, void* hint) {}
