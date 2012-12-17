/// @file
/// @author Anton Petrov <anton.petrov@yale.edu>
/// @author Michael Giuffrida <michael.giuffrida@yale.edu>
/// @version 0.1
/// @since 16 Apr 2012
///
/// @section DESCRIPTION
///
/// Introduction to the Abstract Syntac Tree
/// ----------------------------------------
///
/// An Abstract Syntax Tree (henceforth AST) is a tree representation of the
/// abstract syntactic structure of any language. Each node in the tree
/// represents a construct occuring in the language. The AST is built during
/// the parsing process and is then traversed using a collection of rules
/// and heuristics in order to execute the parsed statement.
///
/// The files in the ast directory (src/query_processor/ast) contain the full
/// definition of the AST interface. The entire definition of the AST has been
/// split into different files based on the type of nodes that are being
/// defined. For example, the ast_select.h and ast_select.cc files declare and
/// define the node used to represent the SQL SELECT statement and all the
/// sub nodes that are referenced by the SELECT statement.
///
/// Each node in the AST is declared as a struct because all its members are
/// by default public. The various enum types in this file (ast.h) are used
/// identify the types of nodes in the AST. This information will be needed
/// during the traversal phase.
///
/// The parsing approach used in Calvin relies on a BNF (Backus-Naur Form)
/// grammar specification outlined in the ../flex_bison/sqlparser.ypp file.
/// Every time a rule in the grammar is matched, the action code to the right
/// of the rule is executed. This action code is used to create and link the
/// nodes in the AST together. When the final (top most) rule in the grammar is
/// matched, the root node of the AST is created.

#ifndef _DB_QUERY_PROCESSOR_AST_AST_H_
#define _DB_QUERY_PROCESSOR_AST_AST_H_

#include <vector>
#include <string>
#include <cstddef>
#include <cstdlib>

using std::vector;
using std::string;

namespace calvin {

/// Forward struct declarations needed so the developer need not worry about
/// the order in which he/she declares the classes in the file.
struct ASTExprOperand;
struct ASTSelect;
struct ASTDelete;
struct ASTUpdate;
struct ASTInsert;
struct ASTAtom;

/// Types of operands allowed.
enum ASTOperandType {
  NAME = 0,
  /// A string for SQL purposes in enclosed in single quotes.
  STRING,
  INTNUM,
  /// Float.
  APPROXNUM,
  BOOL,
};

/// List of comparisons allowed. The seemingly random mapping from comparison
/// types to integers is due to the lexer which is the entity that first
/// decided on these mappings.
enum ASTComparison {
  EQ = 4,          // =
  GREATER_EQ = 6,  // >=
  GREATER = 2,     // >
  LESS_EQ = 5,     // <=
  LESS = 1,        // <
  NOT_EQ = 3,      // != OR <>
};

/// List of built in SQL functions that our executor supports.
enum ASTBuiltInFunction {
  AVG = 0,
  COUNT,
  MAX,
  MIN,
  SUM,
};

/// @struct ASTExpr
/// @brief This ASTExpr node corresponds to an expression (expr symbol) as
/// defined in the BNF grammar.
struct ASTExpr {
  /// This enum type tells us the type of operator that needs to be executed
  /// in order to evaluate the ASTExpr.
  enum ASTExprType {
    OP = 0,
    ADD,
    SUB,
    MUL,
    DIV,
    MOD,
    NEG,
    AND,
    OR,
    XOR,
    NOT,
    CMP,
    FCN,
  } type_;

  virtual ~ASTExpr() { }

  /// Left expression, which can be a single ASTExprOperand or it can be an
  /// entire ASTExpr node. This recursive definition allows for arbitrarily
  /// long expressions to be represented.
  ASTExpr* l_expr_;

  /// The equivalent of the l_expr field, except this field logically sits on
  /// the right of the operator.
  ASTExpr* r_expr_;

  /// Create a 'regular' ASTExpr node by specifying its type and providing
  /// a l_expr and r_expr argument.
  ///
  /// @param type Type of expression node.
  /// @param l_expr Pointer to the ASTExpr node that logically sits to the left
  ///              of the operator in the expression.
  /// @param r_expr Same as l_expr, except that rExp is on the right of the
  ///              operator.
  ASTExpr(ASTExprType type, ASTExpr* l_expr, ASTExpr* r_expr)
    : type_(type), l_expr_(l_expr), r_expr_(r_expr) { }
};

/// @struct ASTExprComparison
/// @brief ASTExprComparison represents an ASTExpr that is a comparison. The
/// type of comparison that is being made is specified using the ASTComparison
/// enum type. This information will be necessary when traversing the AST.
struct ASTExprComparison : public ASTExpr {
  /// The type of comparison that is being done.
  ASTComparison comparison_;

  /// Create an ASTExprComparison node and set the type of comparison we are
  /// doing
  ///
  /// @param l_expr Pointer to the ASTExpr node that logically sits on the left
  ///              of the operator in the ASTExpr.
  /// @param r_expr Same as l_expr except that this ASTExpr sits on the right.
  /// @param comparison Type of comparison performed on the expression operands
  ASTExprComparison(ASTExpr* l_expr, ASTExpr* r_expr, ASTComparison comparison)
    : ASTExpr(CMP, l_expr, r_expr), comparison_(comparison) { }
};

/// @struct ASTExprFunction
/// @brief ASTExprFunction represents an ASTExpr that is a function. The
/// ASTBuiltInFunction enum type is used to tell us what function this
/// function represents.
struct ASTExprFunction : public ASTExpr {
  /// Type of built-in function
  ASTBuiltInFunction function_;

  /// What is the operand on which the function will act? This is usually
  /// an attribute name. For example, the function might be AVG and the
  /// operand can be the sales attribute of the table. All the functions we
  /// chose to support accept a single argument and so we just need a single
  /// const char* field to represent the column name.
  const char* operand_;

  /// Create an ASTExprFunction node, set the type of function the node
  /// represents using the ASTComparison enum type and attach an
  /// ASTExprOperand to serve as the operand of the function.
  ///
  /// @param l_expr Pointer to the ASTExpr node to the left of the expression
  ///              operator.
  /// @param r_expr Same as l_expr, except that r_expr is on the right of the
  ///              expression operator.
  /// @param function The type of function that this ASTExpr represents.
  /// @parma operand The operand of the function.
  ASTExprFunction(ASTExpr* l_expr, ASTExpr* r_expr,
                  const char* function, const char* operand);
};

/// @struct ASTExprOperand
/// @brief This struct represents the lowest rule in the BNF rule hierarchy.
/// An ASTExprOperand serves as the operand node of an ASTExpr.
struct ASTExprOperand : public ASTExpr {
  /// The value field stores the value of the operand using a union data type.
  union datatype {
    const char* svar;
    int ivar;
    double dvar;
    bool bvar;
  } value_;

  /// This field specifies the type of operand using the ASTOperandType enum.
  ASTOperandType type_;

  /// Create an ASTExprOperand node of type either NAME or STRING.
  ///
  /// @param value A pointer to a string that is the operand.
  /// @param type The type of the operand (NAME, STRING, INTNUM, etc.).
  ASTExprOperand(const char* value, ASTOperandType type)
    : ASTExpr(OP, NULL, NULL),  type_(type) {
    value_.svar = value;
  }

  /// Create an ASTExprOperand node of type INTVAL.
  ///
  /// @param value The value of the operand.
  explicit ASTExprOperand(int value) : ASTExpr(OP, NULL, NULL), type_(INTNUM) {
    value_.ivar = value;
  }

  /// Create an ASTExprOperand node of type APPROXNUM.
  ///
  /// @param value The value of the operand.
  explicit ASTExprOperand(double value) : ASTExpr(OP, NULL, NULL),
                                          type_(APPROXNUM) {
    value_.dvar = value;
  }

  /// Create an ASTExprOperand node of type BOOL.
  ///
  /// @param value The value of the operand.
  explicit ASTExprOperand(bool value) : ASTExpr(OP, NULL, NULL), type_(BOOL) {
    value_.bvar = value;
  }
};

/// @struct ASTRoot
/// @brief This is the root node of the AST. It also serves as a base class for
/// the following other classes: ASTSelect, ASTInsert, ASTUpdate and ASTDelete.
/// When the top rule in the BNF grammar is executed (that will either be a
/// select, insert, update or delete rule) the appropriate node is created,
/// cast as an ASTRoot and then returned.
struct ASTRoot {
  /// The ASTRootType enum is used to indicate what type of root node we are
  /// dealing with. This information will be needed when traversing the AST
  /// and we will need to cast the ASTRoot node pointer to the appropriate
  /// derived type (ASTSelect, ASTUpdate, ASTInsert or ASTDelete) based on the
  /// valye of the type enum variable.
  enum ASTRootType {
    SELECT = 0,
    UPDATE,
    INSERT,
    DELETE,
  } type_;

  explicit ASTRoot(ASTRootType type) : type_(type) { }
};

/// @struct ASTKey
/// @brief ASTKey is a key in the underlying storage as specified by the
/// key_format.h specification.
struct ASTKey {
  /// Create a new ASTKey object and add the corresponding atom object.
  ///
  /// @param atom   Pointer to atom object.
  explicit ASTKey(ASTAtom* atom);

  /// Free associated memory
  ~ASTKey();

  /// Given an ASTKey object, this method adds a pointer to another ASTAtom
  /// object.
  ///
  /// @param atom   Pointer to ASTAtom that will be added to the vector.
  ///               keeping track of ASTAtoms in the ASTKey object.
  void AddAtom(ASTAtom* atom);

  /// Vector that keeps track of the atom objects that comprise the
  /// key.
  vector<ASTAtom*> atoms_;

  /// Return a human-readable key
  ///
  /// @returns      A human-readable key
  string toString();
};

/// @struct ASTAtom
/// @brief ASTAtin is an atom that comprises a key. For more information please
/// refer to the key specification in the key_format.h file.
struct ASTAtom {
  /// Create a new ASTAtom object.
  ///
  /// @param atomName   Pointer to string representing atom.
  /// @param val        Which atom are we specifically referring to.
  ASTAtom(const char* atom_name, int val = 0);

  /// Free associated memory
  ~ASTAtom() {
    free(const_cast<char*>(atom_));
  }

  /// String representing the atom.
  const char* atom_;

  /// The integer indicating which atom in the data model hierarchy we are
  /// referencing, 0 if not specified.
  int int_val_;
};

/// @struct ASTGroupBy
/// @brief ASTGroupBy is a representation of the GROUP BY keyword. It contains
/// a list of attributes by which to group the tuples and various other
/// options.
struct ASTGroupBy {
  /// Constructor -- create an ASTGroupBy object.
  ASTGroupBy() {}

  /// Add another ASC or DESC option to the asc_or_desc_ vector of options.
  ///
  /// @param opt  Integer specifying whether we want to sort in ASC (0), or
  ///             DESC (1) or that such an option has not beed specified (-1).
  void add_asc_or_desc(int opt);

  /// Add another attribute to the attributes list
  ///
  /// @param attribute    The attribute name to be added to the list.
  void add_attributes(ASTExprOperand* attribute);

  /// Pointer to a vector which aggregates the ASC or DESC optional arguments
  /// to a GROUP BY list of columns. Note that there is a one to one mapping
  /// from the entries in this vector and the ones in the attributes_ vector.
  /// In other words, the third entry in asc_or_desc_ corresponds to the ASC
  /// or DESC option for the third attribute specified in attributes_.
  ///
  /// -1 -- no ASC or DESC option specified.
  /// 0  -- ASC
  /// 1  -- DESC
  vector<int>* asc_or_desc_;

  /// Pointer to a vector aggregating all the attributes specified in the
  /// GROUP BY clause.
  vector<ASTExprOperand*>* attributes_;
};

}  // namespace calvin

#endif  // _DB_QUERY_PROCESSOR_AST_AST_H_
