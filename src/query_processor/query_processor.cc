/// @file
/// @author Anton Petrov <anton.petrov@yale.edu>
/// @author Michael Giuffrida <michael.giuffrida@yale.edu>
/// @version 0.1
/// @since 26 Jan 2012
///
/// @section DESCRIPTION
///
/// This is the implementation for the QueryProcessor class

#include <cstdlib>
#include <vector>
#include <sstream>
#include "query_processor/query_processor.h"
#include "common/types.h"

/// @TODO (Anton and Michael): Implement this!

namespace calvin {

bool QueryProcessor::ProcessQuery(TxnProto* sql_txn) {
  storage_ = Storage::GetStorageInstance();
  catalog_ = CatalogManager::GetCatalogManagerInstance();

  // Build operator tree from parsed statement
  Operator* op_root = NULL;

  SQLDriver driver;
  if (!driver.Parse(sql_txn->sql_code())) {
    /// @TODO: Implement insert, update, delete
    ASTSelect* select = static_cast<ASTSelect*>(driver.result_);

    // Table references
    OpTable* op_table = new OpTable();
    if (!select->table_references_->size()) {
      // No tables to select from
      LOG(WARNING) << "Select without table references is not implemented";
      return false;
    }

    /// @TODO: Add join support
    // For now, only select from one table
    op_table->table_ = (*(select->table_references_))[0]->table_factor_->
     key_->toString();

    if (!catalog_->TableExists(op_table->table_)) {
      LOG(WARNING) << "Table " << op_table->table_ << " not found";
      return false;
    }

    OpProject* op_project = new OpProject();
    op_project->next_ = op_table;
    op_root = op_project;

    // Find projection attributes in select stmt
    if (!select->select_all_) {
      vector<ASTSelectExpr*>::iterator it;
      for (it = select->select_expr_list_->begin();
           it < select->select_expr_list_->end(); it++) {
        switch ((*it)->expr_->type_) {
          case NAME: {
            // Append the attribute to the project
            string attribute((*it)->expr_->value_.svar);
            if (!catalog_->AttributeExists(op_table->table_, attribute)) {
              LOG(WARNING) << "Attribute " << attribute << " not found";
              return false;
            }
            op_project->attributes_.push_back(attribute);
            break;
          }
          case STRING:
            /// @TODO: Project non-attribute types
            break;
          case INTNUM:
            break;
          case APPROXNUM:
            break;
          case BOOL:
            break;
        }
      }
    }

    // Where expression
    if (select->where_ != NULL) {
      OpSelect* op = new OpSelect();
      op->expr_ = select->where_;
      op->next_ = op_root;
      op_root = op;
    }

    /// @TODO: Add other expressions to operator tree
  } else {
    // SQL parsing failed
    return false;
  }

  /// Execute operator tree
  Table* table = ExecuteQuery(op_root);
  delete op_root;

  if (table == NULL) {
    // Error
    return false;
  }

  // Output results
  LOG(INFO) << "Rows: " << table->values_.size();

  // Use stream here for output purposes
  std::ostringstream oss;

  // Log attribute names
  map<string, CatalogManager::Attribute>::const_iterator it_attrs;
  for (it_attrs = table->attributes_.begin();
       it_attrs != table->attributes_.end(); it_attrs++) {
    oss << (*it_attrs).first << '\t';
  }

  LOG(INFO) << oss.str();

  // Log each tuple
  map<Key, const char*>::const_iterator it_tuples;
  for (it_tuples = table->values_.begin();
       it_tuples != table->values_.end(); it_tuples++) {
    oss.str(string());

    // Log attribute name
    oss << catalog_->at().CodedToHR((*it_tuples).first) << ":\t";

    // Get value of attribute
    for (it_attrs = table->attributes_.begin();
         it_attrs != table->attributes_.end(); it_attrs++) {
      CatalogManager::Attribute attr = (*it_attrs).second;
      if (attr.type_ == CatalogManager::INT) {
        oss << Tuple::GetInt((*it_tuples).second, attr.offset_) << "\t";
      } else {
        oss << Tuple::GetString((*it_tuples).second, attr.offset_)
                  << "\t";
      }
    }
    LOG(INFO) << oss.str();
  }

  delete table;

  // Execution successful
  return true;
}

QueryProcessor::Table*
QueryProcessor::ExecuteQuery(Operator* root) {
  if (root->operator_ == OP_SELECT) {
    OpSelect* op = static_cast<OpSelect*>(root);

    // Get tuples from remainder of tree
    Table* table = ExecuteQuery(op->next_);

    // Apply select expression to tuples
    return ProcessSelect(op->expr_, table);
  } else if (root->operator_ == OP_PROJECT) {
    OpProject* op = static_cast<OpProject*>(root);

    // Get tuples from next operation
    Table* table = ExecuteQuery(op->next_);

    if (!op->attributes_.size()) {
      // Project all
      return table;
    }

    // Loop over attribute map, removing those not in projection
    map<string, CatalogManager::Attribute>::const_iterator it;
    for (it = table->attributes_.begin();
         it != table->attributes_.end(); ) {
      string attr = (*it).first;
      it++;

      // If table attribute is not in projection, remove
      if (find(op->attributes_.begin(), op->attributes_.end(), attr) ==
          op->attributes_.end()) {
        table->attributes_.erase(attr);
      }
    }

    /// @TODO: Re-form tuple value and update attribute offsets?

    return table;
  } else if (root->operator_ == OP_TABLE) {
    // Read tuples from a table
    OpTable* op = static_cast<OpTable*>(root);

    Table* table = new Table();

    // Copy list of attributes
    table->attributes_ = *catalog_->Attributes(op->table_);
    table->table_ = op->table_;

    // Extract tuples into table
    // To get first key, start at tablename(0)
    string key_start_name(table->table_.c_str());
    key_start_name.append("(0)");

    Key key_start = Key(catalog_->at().HRToCoded(key_start_name));
    const Key* key = &key_start;

    // Scan in key order
    while ((key = storage_->GetNextKey(*key, 1)) != NULL) {
      // Get value from storage
      char* value = reinterpret_cast<char*>(
          storage_->Get(*key, 1)->data());
      table->values_[*key] = value;
    }

    return table;
  }

  return NULL;
}

QueryProcessor::Table*
QueryProcessor::ProcessSelect(ASTExpr* expr, Table* table) {
  /// @TODO: Add smarter selection mechanisms

  // Iterate over tuples, testing predicate
  map<Key, const char*>::const_iterator it;
  for (it = table->values_.begin();
       it != table->values_.end(); ) {
    Key key = (*it).first;
    const char* value = (*it).second;
    it++;

    // Process selection expression into a single operand
    ASTExprOperand* op = ProcessExpr(expr, value, table->attributes_);
    if (op == NULL) {
      continue;
    }
    // Decide whether selection predicate matched
    bool match = false;
    switch (op->type_) {
      case STRING:
        match = strlen(op->value_.svar);
        break;
      case INTNUM:
        match = op->value_.ivar;
        break;
      case APPROXNUM:
        match = op->value_.dvar;
        break;
      case BOOL:
        match = op->value_.bvar;
        break;
      default:
        // Shouldn't reach this
        break;
    }
    if (!match) {
      // Remove tuple
      table->values_.erase(key);
    }
    delete op;
  }

  return table;
}

ASTExprOperand* QueryProcessor::ProcessExpr(ASTExpr* expr, const char* tuple,
  const map<string, CatalogManager::Attribute>& attributes) {
  if (expr == NULL) {
    return NULL;
  }

  ASTExprOperand* result = NULL;

  if (expr->type_ == ASTExpr::OP) {
    // Already an operand
    ASTExprOperand* op = static_cast<ASTExprOperand*>(expr);

    // If operand is of type NAME, reduce attribute name to its value in tuple
    if (op->type_ == NAME) {
      // Find attribute in attribute list
      map<string, CatalogManager::Attribute>::const_iterator it =
        attributes.find(op->value_.svar);
      // Populate result based on attribute type
      if ((*it).second.type_ == CatalogManager::INT) {
        result = new ASTExprOperand(Tuple::GetInt(tuple, (*it).second.offset_));
      } else {
        result = new ASTExprOperand(
          Tuple::GetString(tuple, (*it).second.offset_), STRING);
      }
    } else {
      // Copy the operand
      result = new ASTExprOperand(0);
      result->type_ = op->type_;
      memcpy(&result->value_, &op->value_, sizeof(op->value_));
    }
  } else if (expr->type_ == ASTExpr::AND) {
    // Conjunction of two expressions
    ASTExprOperand* l_expr = ProcessExpr(expr->l_expr_, tuple, attributes);
    ASTExprOperand* r_expr = ProcessExpr(expr->r_expr_, tuple, attributes);
    if (l_expr == NULL || r_expr == NULL) {
      return NULL;
    }

    bool value = true;
    // AND expressions based on their types
    if (l_expr->type_ == INTNUM) {
      value = l_expr->value_.ivar;
    } else if (l_expr->type_ == APPROXNUM) {
      value = l_expr->value_.dvar;
    } else if (l_expr->type_ == BOOL) {
      value = l_expr->value_.bvar;
    } else if (l_expr->type_ == STRING) {
      value = strlen(l_expr->value_.svar);
    }

    // If true, consider right value
    if (value) {
      if (r_expr->type_ == INTNUM) {
        value = r_expr->value_.ivar;
      } else if (r_expr->type_ == APPROXNUM) {
        value = r_expr->value_.dvar;
      } else if (r_expr->type_ == BOOL) {
        value = r_expr->value_.bvar;
      } else if (r_expr->type_ == STRING) {
        value = strlen(r_expr->value_.svar);
      }
    }

    // Copy result into l_expr and free r_expr
    l_expr->type_ = BOOL;
    memcpy(&l_expr->value_, &value, sizeof(value));
    delete r_expr;

    result = l_expr;
  } else if (expr->type_ == ASTExpr::CMP) {
    // Compare two expressions
    ASTExprComparison* cmp = static_cast<ASTExprComparison*>(expr);
    ASTExprOperand* l_expr = ProcessExpr(expr->l_expr_, tuple, attributes);
    ASTExprOperand* r_expr = ProcessExpr(expr->r_expr_, tuple, attributes);

    if (l_expr == NULL || r_expr == NULL) {
      return NULL;
    }

    // Get types and values, to help with ugliness
    ASTOperandType l_type = l_expr->type_;
    ASTOperandType r_type = r_expr->type_;
    ASTExprOperand::datatype l_value = l_expr->value_;
    ASTExprOperand::datatype r_value = r_expr->value_;

    bool value;
    switch (cmp->comparison_) {
      case EQ:
      case NOT_EQ:  // Invert the result later
        // Compare expressions based on their types
        if (l_type == STRING && r_type == STRING) {
          // Compare strings for equality
          value = (!strcmp(l_value.svar, r_value.svar));
        } else if (l_type != STRING &&
                   r_type != STRING) {
          if (l_type == INTNUM) {
            // Left hand side is an int
            if (r_type == INTNUM) {
              value = (l_value.ivar == r_value.ivar);
            } else if (r_type == APPROXNUM) {
              value = (l_value.ivar == r_value.dvar);
            } else if (r_type == BOOL) {
              value = (l_value.ivar && r_value.bvar);
            }
          } else if (l_type == APPROXNUM) {
            // Left hand side is a float
            if (r_type == INTNUM) {
              value = (l_value.dvar == r_value.ivar);
            } else if (r_type == APPROXNUM) {
              value = (l_value.dvar == r_value.dvar);
            } else if (r_type == BOOL) {
              value = (l_value.dvar && r_value.bvar);
            }
          } else if (l_type == BOOL) {
            // Left hand side is a bool
            if (r_type == INTNUM) {
              value = (l_value.bvar == r_value.ivar);
            } else if (r_type == APPROXNUM) {
              value = (l_value.bvar == r_value.dvar);
            } else if (r_type == BOOL) {
              value = (l_value.bvar == r_value.bvar);
            }
          }
        } else {
          // Cannot compare string and non-string
          LOG(WARNING) << "Attempting to compare string and non-string"
                       << "for equality";
          return NULL;
        }
        break;
      case GREATER:
      case LESS_EQ:  // inverse
        if (l_type == INTNUM) {
          if (r_type == INTNUM) {
            value = (l_value.ivar > r_value.ivar);
          } else if (r_type == APPROXNUM) {
            value = (l_value.ivar > r_value.dvar);
          } else {
            LOG(WARNING) << "Attempting to compare int and non-number";
            return NULL;
          }
        } else if (l_type == APPROXNUM) {
          if (r_type == INTNUM) {
            value = (l_value.dvar > r_value.ivar);
          } else if (r_type == APPROXNUM) {
            value = (l_value.dvar > r_value.dvar);
          } else {
            LOG(WARNING) << "Attempting to compare float and non-number";
            return NULL;
          }
        } else {
          // Cannot compare disparate types
          LOG(WARNING) << "Attempting to compare non-numbers";
          return NULL;
        }
        break;
      case LESS:
      case GREATER_EQ:  // inverse
        if (l_type == INTNUM) {
          if (r_type == INTNUM) {
            value = (l_value.ivar < r_value.ivar);
          } else if (r_type == APPROXNUM) {
            value = (l_value.ivar < r_value.dvar);
          } else {
            LOG(WARNING) << "Attempting to compare int and non-number";
            return NULL;
          }
        } else if (l_type == APPROXNUM) {
          if (r_type == INTNUM) {
            value = (l_value.dvar < r_value.ivar);
          } else if (r_type == APPROXNUM) {
            value = (l_value.dvar < r_value.dvar);
          } else {
            LOG(WARNING) << "Attempting to compare int and non-number";
            return NULL;
          }
        } else {
          // Cannot compare disparate types
          LOG(WARNING) << "Attempting to compare non-numbers";
          return NULL;
        }
        break;
    }
    // Apply inverse where necessary
    if (cmp->comparison_ == NOT_EQ ||
        cmp->comparison_ == LESS_EQ ||
        cmp->comparison_ == GREATER_EQ) {
      value = !value;
    }

    l_expr->type_ = BOOL;
    memcpy(&l_expr->value_, &value, sizeof(value));
    delete r_expr;

    result = l_expr;
  } else if (expr->type_ == ASTExpr::ADD ||
             expr->type_ == ASTExpr::SUB ||
             expr->type_ == ASTExpr::MUL ||
             expr->type_ == ASTExpr::DIV ||
             expr->type_ == ASTExpr::MOD) {
    // Arithmetic operations
    ASTExprOperand* l_expr = ProcessExpr(expr->l_expr_, tuple, attributes);
    ASTExprOperand* r_expr = ProcessExpr(expr->r_expr_, tuple, attributes);

    if (l_expr == NULL || r_expr == NULL) {
      return NULL;
    }

    // Check type constraints
    if (expr->type_ == ASTExpr::MOD &&
        (l_expr->type_ != INTNUM ||
         r_expr->type_ != INTNUM)) {
      LOG(WARNING) << "Attempting mod operation on non-integers";
      return NULL;
    } else if ((l_expr->type_ != INTNUM && l_expr->type_ != APPROXNUM) ||
               (r_expr->type_ != INTNUM && r_expr->type_ != APPROXNUM)) {
      LOG(WARNING) << "Attempting math operation on non-number";
      return NULL;
    }

    // Error on divide by zero
    if (expr->type_ == ASTExpr::DIV ||
        expr->type_ == ASTExpr::MOD) {
      if ((r_expr->type_ == INTNUM && r_expr->value_.ivar) ||
          (!r_expr->value_.dvar)) {
        LOG(WARNING) << "Attempting to divide by zero";
        return NULL;
      }
    }

    // Apply operation depending on left and right types
    if (l_expr->type_ == INTNUM) {
      if (r_expr->type_ == INTNUM) {
        // Ints
        l_expr->type_ = INTNUM;
        if (expr->type_ == ASTExpr::ADD) {
          l_expr->value_.ivar += r_expr->value_.ivar;
        } else if (expr->type_ == ASTExpr::SUB) {
          l_expr->value_.ivar -= r_expr->value_.ivar;
        } else if (expr->type_ == ASTExpr::MUL) {
          l_expr->value_.ivar *= r_expr->value_.ivar;
        } else if (expr->type_ == ASTExpr::DIV) {
          l_expr->value_.ivar /= r_expr->value_.ivar;
        } else if (expr->type_ == ASTExpr::DIV) {
          l_expr->value_.ivar %= r_expr->value_.ivar;
        }
      } else {
        // Int and float
        l_expr->type_ = APPROXNUM;
        if (expr->type_ == ASTExpr::ADD) {
          l_expr->value_.dvar = l_expr->value_.ivar + r_expr->value_.dvar;
        } else if (expr->type_ == ASTExpr::SUB) {
          l_expr->value_.dvar = l_expr->value_.ivar - r_expr->value_.dvar;
        } else if (expr->type_ == ASTExpr::MUL) {
          l_expr->value_.dvar = l_expr->value_.ivar * r_expr->value_.dvar;
        } else if (expr->type_ == ASTExpr::DIV) {
          l_expr->value_.dvar = l_expr->value_.ivar / r_expr->value_.dvar;
        }
      }
    } else if (l_expr->type_ == APPROXNUM) {
      l_expr->type_ = APPROXNUM;
      if (r_expr->type_ == INTNUM) {
        // Float and int
        if (expr->type_ == ASTExpr::ADD) {
          l_expr->value_.dvar += r_expr->value_.ivar;
        } else if (expr->type_ == ASTExpr::SUB) {
          l_expr->value_.dvar -= r_expr->value_.ivar;
        } else if (expr->type_ == ASTExpr::MUL) {
          l_expr->value_.dvar *= r_expr->value_.ivar;
        } else if (expr->type_ == ASTExpr::DIV) {
          l_expr->value_.dvar /= r_expr->value_.ivar;
        }
      } else {
        // Floats
        if (expr->type_ == ASTExpr::ADD) {
          l_expr->value_.dvar += r_expr->value_.dvar;
        } else if (expr->type_ == ASTExpr::SUB) {
          l_expr->value_.dvar -= r_expr->value_.dvar;
        } else if (expr->type_ == ASTExpr::MUL) {
          l_expr->value_.dvar *= r_expr->value_.dvar;
        } else if (expr->type_ == ASTExpr::DIV) {
          l_expr->value_.dvar /= r_expr->value_.dvar;
        }
      }
    }

    delete r_expr;

    result = l_expr;
  } else if (expr->type_ == ASTExpr::NEG) {
    // Negate the expression
    ASTExprOperand* l_expr = ProcessExpr(expr->l_expr_, tuple, attributes);

    if (l_expr == NULL) {
      return NULL;
    }

    // Negate value
    if (l_expr->type_ == INTNUM) {
      l_expr->value_.ivar *= -1;
    } else if (l_expr->type_ == APPROXNUM) {
      l_expr->value_.dvar *= -1;
    } else {
      LOG(WARNING) << "Attempting negation operation on non-number";
      return NULL;
    }

    result = l_expr;
  } else if (expr->type_ == ASTExpr::NOT) {
    // Invert the expression
    ASTExprOperand* l_expr = ProcessExpr(expr->l_expr_, tuple, attributes);

    if (l_expr == NULL) {
      return NULL;
    }

    if (l_expr->type_ == BOOL) {
      l_expr->value_.bvar = !l_expr->value_.bvar;
    } else if (l_expr->type_ == INTNUM) {
      l_expr->value_.bvar = !l_expr->value_.ivar;
    } else if (l_expr->type_ == APPROXNUM) {
      l_expr->value_.bvar = !l_expr->value_.dvar;
    } else {
      LOG(WARNING) << "Attempting not operation on non-nottable type";
      return NULL;
    }

    result = l_expr;
  }

  return result;
}

Tuple::Tuple(size_t size) : size_(size), offset_(0) {
  value_ = static_cast<char*>(malloc(size));
}

Tuple::Tuple(char* value, size_t size) : value_(value) {
  size_ = offset_ = size;
}

int Tuple::GetInt(size_t offset) const {
  // Cast the value as an int pointer
  return *(reinterpret_cast<int*>(&value_[offset]));
}

const char* Tuple::GetString(size_t offset) const {
  return &value_[offset];
}

int Tuple::GetInt(const char* value, size_t offset) {
  // Cast the value as an int pointer
  return *(reinterpret_cast<const int*>(&value[offset]));
}

const char* Tuple::GetString(const char* value, size_t offset) {
  return &value[offset];
}

void Tuple::add_value(uint32_t value) {
  // Copy the value into the byte array
  memcpy(&value_[offset_], reinterpret_cast<const void*>(&value),
         sizeof(value));
  offset_ += sizeof(value);
}

void Tuple::add_value(string value, size_t length) {
  size_t i;
  // Copy attribute value
  for (i = 0; i < value.length() && i < length; i++) {
    value_[offset_++] = value[i];
  }

  if (length > value.length()) {
    // Pad the value with 0s
    for (i = 0; i < length - value.length(); i++) {
      value_[offset_++] = '\0';
    }
  }

  // Add final null character
  value_[offset_++] = '\0';
}

const char* Tuple::value() const {
  return value_;
}

CatalogManager* QueryProcessor::catalog_ = NULL;
Storage* QueryProcessor::storage_ = NULL;

}  // namespace calvin
