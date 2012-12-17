/// @file
/// @author Anton Petrov <anton.petrov@yale.edu>
/// @author Michael Giuffrida <michael.giuffrida@yale.edu>
/// @version 0.1
/// @since 16 Apr 2012
///
/// @section DESCRIPTION
///
/// Implementation details of the functions declared in the ast_select.h file.

#include "query_processor/ast/ast_select.h"
#include <sstream>

namespace calvin {

ASTSelect::~ASTSelect() {
  if (opts_ != NULL) {
    free(const_cast<char*>(opts_));
  }
  size_t i;
  if (select_expr_list_ != NULL) {
    for (i = 0; i < select_expr_list_->size(); i++) {
      delete (*select_expr_list_)[i];
    }
  }
  delete select_expr_list_;

  if (table_references_ != NULL) {
    for (i = 0; i < table_references_->size(); i++) {
      delete (*table_references_)[i];
    }
  }
  delete table_references_;

  delete where_;
  delete group_by_;
  delete having_;
  delete order_by_;
  delete limit_;
}

ASTSelectExpr::~ASTSelectExpr() {
  if (opt_as_alias_ != NULL) {
    free(const_cast<char*>(opt_as_alias_));
  }
  delete expr_;
}

ASTSelectExpr* ASTSelectExpr::create(const char* opt,
                                     ASTExprOperand* expression) {
  ASTSelectExpr* node = new ASTSelectExpr();
  node->opt_as_alias_ = opt;
  node->expr_ = expression;
  return node;
}

ASTTableReference::~ASTTableReference() {
  if (opt_as_alias_ != NULL) {
    free(const_cast<char*>(opt_as_alias_));
  }
  delete table_factor_;
  delete join_table_;
}

ASTTableReference* ASTTableReference::create(ASTTableFactor* table_factor,
                                             ASTJoinTable* join_table) {
  ASTTableReference* node = new ASTTableReference();
  node->table_factor_ = table_factor;
  node->join_table_ = join_table;
  return node;
}

ASTTableFactor::~ASTTableFactor() {
  if (opt_as_alias_ != NULL) {
    free(const_cast<char*>(opt_as_alias_));
  }
  delete key_;
}

ASTTableFactor::ASTTableFactor(ASTKey* key_ptr, const char* as_alias)
  : key_(key_ptr), opt_as_alias_(as_alias) { }

}  // namespace calvin
