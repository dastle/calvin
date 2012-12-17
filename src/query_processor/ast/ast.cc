/// @file
/// @author Anton Petrov <anton.petrov@yale.edu>
/// @author Michael Giuffrida <michael.giuffrida@yale.edu>
/// @version 0.1
/// @since 16 Apr 2012
///
/// @section DESCRIPTION
///
/// Implementation details of the functions declared in the ast.h file.

#include <cstdio>
#include <cstring>
#include "query_processor/ast/ast.h"

namespace calvin {
ASTExprFunction::ASTExprFunction(ASTExpr* l_expr, ASTExpr* r_expr,
    const char* function, const char* operand) :
  ASTExpr(FCN, l_expr, r_expr), operand_(operand) {
    // Decide what type of function this node represents
    if (strcmp (function, "AVG"))
      function_ = AVG;
    else if (strcmp (function, "COUNT"))
      function_ = COUNT;
    else if (strcmp (function, "MAX"))
      function_ = MAX;
    else if (strcmp (function, "MIN"))
      function_ = MIN;
    else if (strcmp (function, "SUM"))
      function_ = SUM;
  }

// ----------------------------------------------------------------------------
// ASTKey Definitions

ASTKey::ASTKey(ASTAtom* atom) {
  AddAtom(atom);
}

void ASTKey::AddAtom(ASTAtom* atom) {
  atoms_.push_back(atom);
}

ASTKey::~ASTKey() {
  for (size_t i = 0; i < atoms_.size(); i++) {
    delete atoms_[i];
  }
}

string ASTKey::toString() {
  string s;

  char digits[21];  // can fit decimal form of 64-bit unsigned int
  vector<ASTAtom*>::iterator it;
  for (it = atoms_.begin(); it < atoms_.end();) {
    s.append((*it)->atom_);
    // Append digits
    if ((*it)->int_val_) {
      std::snprintf(digits, sizeof(digits), "%d", (*it)->int_val_);
      s.append("(").append(digits).append(")");
    }

    it++;
    if (it != atoms_.end()) {
      s.append(".");
    }
  }

  return s;
}

// -----------------------------------------------------------------------------
// ASTAtom Definitions

ASTAtom::ASTAtom(const char* atom_name, int val)
  : atom_(atom_name), int_val_(val) {
}

// -----------------------------------------------------------------------------
// ASTGroupBy Definitions

void ASTGroupBy::add_asc_or_desc(int opt) {
  asc_or_desc_->push_back(opt);
}

void ASTGroupBy::add_attributes(ASTExprOperand* attribute) {
  attributes_->push_back(attribute);
}

}  // namespace calvin
