/// @file
/// @author Anton Petrov <anton.petrov@yale.edu>
/// @author Michael Giuffrida <michael.giuffrida@yale.edu>
/// @version 0.1
/// @since 05 Apr 2012
///
/// @section DESCRIPTION
///
/// The CatalogManager maintains a list of table schemas

#include "query_processor/catalog_manager.h"
#include <glog/logging.h>
#include <cstdlib>

namespace calvin {

// The sole application instance is maintained as a private static
CatalogManager* CatalogManager::catalog_instance_ = NULL;

CatalogManager* CatalogManager::GetCatalogManagerInstance() {
  if (catalog_instance_ == NULL) {
    catalog_instance_ = new CatalogManager();
  }
  return catalog_instance_;
}

bool CatalogManager::TableExists(const string& table) const {
  return tables_.count(table);
}

bool CatalogManager::AttributeExists(const string& table,
                                     const string& attribute) const {
  map<string, Table*>::const_iterator it = tables_.find(table);
  if (it != tables_.end()) {
    return it->second->AttributeExists(attribute);
  }
  // Could not find table
  LOG(WARNING) << "Calling AttributeExists on invalid Table: " << table;
  return false;
}

size_t CatalogManager::AttributeSize(const string& table,
                                     const string& attribute) const {
  map<string, Table*>::const_iterator it = tables_.find(table);
  if (it != tables_.end()) {
    return it->second->AttributeSize(attribute);
  }
  // Could not find table
  LOG(WARNING) << "Calling AttributeSize on invalid Table: " << table;
  return 0;
}

size_t CatalogManager::AttributeOffset(const string& table,
                                       const string& attribute) const {
  map<string, Table*>::const_iterator it = tables_.find(table);
  if (it != tables_.end()) {
    return it->second->AttributeOffset(attribute);
  }
  // Could not find table
  LOG(WARNING) << "Calling AttributeOffset on invalid Table: " << table;
  return 0;
}

CatalogManager::AttrType
CatalogManager::AttributeType(const string& table,
                              const string& attribute) const {
  map<string, Table*>::const_iterator it = tables_.find(table);
  if (it != tables_.end()) {
    return it->second->AttributeType(attribute);
  }
  // Could not find table
  LOG(WARNING) << "Calling AttributeType on invalid Table: " << table;
  return INVALID;
}

bool CatalogManager::IndexExists(const string& table,
                                 const string& attribute) const {
  map<string, Table*>::const_iterator it = tables_.find(table);
  if (it != tables_.end()) {
    return it->second->IndexExists(attribute);
  }
  // Could not find table
  LOG(WARNING) << "Calling IndexExists on invalid Table: " << table;
  return false;
}

const map<string, CatalogManager::Attribute>*
CatalogManager::Attributes(const string& table) const {
  map<string, Table*>::const_iterator it = tables_.find(table);
  if (it != tables_.end()) {
    return it->second->Attributes();
  }
  // Could not find table
  LOG(WARNING) << "Calling Attributes on invalid Table: " << table;
  return NULL;
}


size_t CatalogManager::TupleSize(const string& table) const {
  map<string, Table*>::const_iterator it = tables_.find(table);
  if (it != tables_.end()) {
    return it->second->Size();
  }
  // Could not find table
  LOG(WARNING) << "Calling TupleSize on invalid Table: " << table;
  return 0;
}

bool CatalogManager::AddTable(const string& name, const string& schema) {
  if (TableExists(name)) {
      LOG(WARNING) << "Calling AddTable with table that already exists: "
                   << name;
    return false;
  }

  Table* table = new Table();
  if (table->SetSchema(schema)) {
    tables_[name] = table;
    return true;
  }

  // Table could not be initialized
  LOG(WARNING) << "Calling AddTable with invalid schema: " << schema;
  delete table;
  return false;
}

bool CatalogManager::DeleteTable(const string& name) {
  map<string, Table*>::iterator it = tables_.find(name);
  if (it != tables_.end()) {
    delete it->second;
    tables_.erase(it);
    return true;
  }
  LOG(WARNING) << "Calling DeleteTable on invalid Table: " << name;
  return false;
}

AtomTable& CatalogManager::at() {
  return at_;
}

CatalogManager::~CatalogManager() {
  // Delete the tables
  map<string, Table*>::iterator it;
  for (it = tables_.begin(); it != tables_.end(); it++) {
    delete (*it).second;
  }
}

bool CatalogManager::Table::SetSchema(const string& schema) {
  size_t sep = 0;
  size_t pos = 0;
  size_t offset = 0;

  while (sep < string::npos) {
    string name;
    string value;
    Attribute attr;

    sep = schema.find(':', pos);
    // Name should be at least one char
    if (sep - pos == 0 || sep >= schema.length()) {
      break;
    }

    name = schema.substr(pos, sep - pos);
    if (name.find_first_of(":;") < string::npos) {
      // Invalid character
      return false;
    } else if (attributes_.count(name)) {
      // Attribute already added
      return false;
    }

    // Increment position to char after separator
    pos = sep + 1;
    sep = schema.find(':', pos);

    // Size should be at least one char
    if (sep - pos == 0 || sep >= schema.length()) {
      break;
    }

    value = schema.substr(pos, sep - pos);
    if (!value.compare("int")) {
      attr.type_ = INT;
    } else if (!value.compare("string")) {
      attr.type_ = STRING;
    } else {
      // Unsupported type
      break;
    }

    // Increment position to char after separator
    pos = sep + 1;
    sep = schema.find_first_of(",;", pos);

    // Parse width
    value = schema.substr(pos, sep - pos);
    int width = atoi(value.c_str());

    // Require positive width
    if (width <= 0) {
      return false;
    }
    attr.size_ = width;

    attr.index_available_ = false;
    // Check for ",index"
    if (schema[sep] == ',') {
        attr.index_available_ = !(schema.compare(sep+1, 5, "index"));
        sep = schema.find(';', sep + 6);
    }

    pos = sep + 1;

    attr.offset_ = offset;
    attributes_[name] = attr;

    // Account for value
    offset += width;
    if (attr.type_ == STRING) {
      // Account for null character
      offset++;
    }
  }

  size_ = offset;

  return true;
}

bool CatalogManager::Table::AttributeExists(const string& attribute) const {
  return attributes_.count(attribute);
}

size_t CatalogManager::Table::AttributeSize(const string& attribute) const {
  map<string, Attribute>::const_iterator it = attributes_.find(attribute);
  if (it != attributes_.end()) {
    return it->second.size_;
  }
  LOG(WARNING) << "Calling AttributeSize on invalid Attribute: " << attribute;
  return 0;
}

size_t CatalogManager::Table::AttributeOffset(const string& attribute) const {
  map<string, Attribute>::const_iterator it = attributes_.find(attribute);
  if (it != attributes_.end()) {
    return it->second.offset_;
  }
  LOG(WARNING) << "Calling AttributeOffset on invalid Attribute: "
               << attribute;
  return 0;
}

CatalogManager::AttrType
CatalogManager::Table::AttributeType(const string& attribute) const {
  map<string, Attribute>::const_iterator it = attributes_.find(attribute);
  if (it != attributes_.end()) {
    return it->second.type_;
  }
  LOG(WARNING) << "Calling AttributeType on invalid Attribute: "
               << attribute;
  return INVALID;
}

bool CatalogManager::Table::IndexExists(const string& attribute) const {
  map<string, Attribute>::const_iterator it = attributes_.find(attribute);
  if (it != attributes_.end()) {
    return it->second.index_available_;
  }
  LOG(WARNING) << "Calling IndexExists on invalid Attribute: " << attribute;
  return false;
}

const map<string, CatalogManager::Attribute>*
CatalogManager::Table::Attributes() const {
  return &attributes_;
}

size_t CatalogManager::Table::Size() const {
  return size_;
}

}  // namespace calvin
