/// @file
/// @author Alex Thomson <thomson@cs.yale.edu>
/// @version 0.1
/// @since 5 April 2012
///
/// @section DESCRIPTION
///
/// KeyFormat/AtomTable implementation

#include "query_processor/key_format.h"

#include <assert.h>
#include <glog/logging.h>

#include "common/utils.h"

namespace calvin {

/// @TODO(alex): Currently using IntToString and StringToInt, with static casts
///              for uint64s. This is SUPERDUPER BAD. Fix it please.

void AtomTable::AddAtom(const string& hr, uint64 code, IDType id_type) {
  hr_to_code_[hr] = code;
  code_to_hr_[code] = hr;
  code_to_id_type_[code] = id_type;
}

/// @TODO(alex): Do we need better error checking to detect bad keys?
string AtomTable::HRToCoded(const string& hr) {
  if (hr.size() == 0) {
    LOG(WARNING) << "Coding empty human-readable key.";
    return 0;
  }
  string buffer;       // Current HR atom that we're reading.
  string coded_atoms;  // Atom portion of coded key.
  string coded_ids;    // ID portion of coded key.

  const char* pos = hr.data();
  const char* limit = hr.data() + hr.size();
  while (pos < limit) {
    // Parse atom.
    for (; *pos != '(' && *pos != '.' && pos < limit; pos++) {
      buffer.append(1, *pos);
    }

    LOG(WARNING) << "Parsed atom: " << buffer;

    // Check that atom appears in AtomTable.
    if (hr_to_code_.count(buffer) == 0) {
      LOG(WARNING) << "Bad human-readable atom: " << buffer;
      return "";
    }

    // Encode and append.
    uint64 atom_code = hr_to_code_[buffer];
    CVarint::Append64(&coded_atoms, atom_code);

    // Parse ID (depending on atom's IDType).
    buffer.clear();
    switch (code_to_id_type_[atom_code]) {
      case UINT64:
        // Read in a uint64 id from the hr key. Should terminate with ')'.
        assert(*pos == '(');
        for (pos++; pos < limit && *pos != ')'; pos++) {
          buffer.append(1, *pos);
        }
        CVarint::Append64(&coded_ids, static_cast<uint64>(StringToInt(buffer)));
        buffer.clear();
        pos++;
        break;

      case STRING:
        // Read in a string id from the hr key. Should terminate with ')'.
        assert(*pos == '(');
        for (pos++; pos < limit && *pos != ')'; pos++) {
          coded_ids.append(1, *pos);
        }
        // Null-terminate string.
        coded_ids.append(1, '\0');
        pos++;
        break;

      case NONE:
      default:
        break;
    }
    // skip '.'
    pos++;
  }
  // Construct and return final coded key.
  CVarint::Append64(&coded_atoms, 0);  // Special end-of-atoms token.
  coded_atoms.append(coded_ids);
  return coded_atoms;
}

/// @TODO(alex): Do we need better error checking to detect bad keys?
string AtomTable::CodedToHR(const string& coded) {
  string hr_key;

  // Search for special end-of-atoms token in coded string.
  uint64 code = 1;  // arbitrary non-zero initial value
  const char* id_pos = coded.data();
  const char* limit = coded.data() + coded.size();
  while (code != 0) {
    // Read the next atom code.
    id_pos = CVarint::Parse64(id_pos, &code);
  }

  // Therefore pos points to the first id (or the end of the string if all atoms
  // had id type NONE. Add atom, id pairs to HR key.
  const char* atom_pos = coded.data();
  // Read the first atom code.
  atom_pos = CVarint::Parse64(atom_pos, &code);
  while (code != 0) {
    // Check that atom appears in AtomTable.
    if (code_to_hr_.count(code) == 0) {
      LOG(WARNING) << "Bad atom code.";
      return "";
    }

    // Append HR version of atom to the HR key.
    hr_key.append(code_to_hr_[code]);

    // Handle the associated id (if applicable), depending on the atom's IDType.
    uint64 x;  // Variable to hold uint64 ids.
    size_t s;  // Variable to hold string ids' sizes.
    switch (code_to_id_type_[code]) {
      case UINT64:
        hr_key.append(1, '(');
        id_pos = CVarint::Parse64(id_pos, &x);
        hr_key.append(IntToString(static_cast<int>(x)));
        hr_key.append(1, ')');
        break;

      case STRING:
        // The id string is null-terminated within the coded key.
        hr_key.append(1, '(');
        s = strlen(id_pos);
        hr_key.append(id_pos, s);
        id_pos += s+1;  // strlen plus the null char
        hr_key.append(1, ')');
        break;

      case NONE:
      default:
        break;
    }

    // Append separating '.'.
    hr_key.append(1, '.');

    // Read the next atom code.
    atom_pos = CVarint::Parse64(atom_pos, &code);
  }

  // Resize to 'unappend' the last '.'
  hr_key.resize(hr_key.size() - 1);

  // Check that we ended in the right place.
  assert(id_pos == limit);

  return hr_key;
}

}  // namespace calvin

