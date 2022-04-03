#include "globalexpr_validator.hh"

namespace Tailslide {

bool GlobalExprValidatingVisitor::visit(LSLGlobalVariable *node) {
  valid_rvalue = true;
  LSLASTNode *rvalue = node->get_child(1);
  if (rvalue && rvalue->get_node_type() != NODE_NULL && !rvalue->is_constant()) {
    // don't bother complaining if the rvalue isn't constant due to a type or symbol
    // resolution error that we've already reported.
    if (rvalue->get_constant_precluded()) {
      valid_rvalue = false;
      return false;
    }
    valid_rvalue = false;
  }
  // Worth checking the children specifically to see if there's anything other
  // than non-constness that'd make the rvalue invalid.
  if (valid_rvalue)
    rvalue->visit(this);

  if (!valid_rvalue) {
    assert(rvalue);
    ERROR(IN(rvalue), E_GLOBAL_INITIALIZER_NOT_CONSTANT);
  }

  return false;
}

bool GlobalExprValidatingVisitor::visit(LSLFunctionExpression *node) {
  valid_rvalue = false;
  return false;
}

bool SimpleAssignableValidatingVisitor::visit(LSLExpression *node) {
  // other expression types that don't really need their own visitors, but
  // may not be valid in SA context
  switch(node->get_node_sub_type()) {
    case NODE_CONSTANT_EXPRESSION:
    case NODE_VECTOR_EXPRESSION:
    case NODE_QUATERNION_EXPRESSION:
    case NODE_LIST_EXPRESSION:
      return true;
    default:
      valid_rvalue = false;
      return false;
  }
}

bool SimpleAssignableValidatingVisitor::visit(LSLUnaryExpression *node) {
  // only unary minus is allowed
  if (node->get_operation() != '-') {
    valid_rvalue = false;
    return false;
  }
  // and only for certain builtins, by virtue of them _actually_ being lexer tokens
  LSLASTNode *rvalue = node->get_child(0);
  if (rvalue->get_node_sub_type() != NODE_LVALUE_EXPRESSION) {
    valid_rvalue = false;
    return false;
  }
  auto *id = (LSLIdentifier*)rvalue->get_child(0);
  auto *sym = id->get_symbol();
  // exprs not checked if type or symbol error occurred earlier
  assert(sym);
  if (sym->get_sub_type() != SYM_BUILTIN) {
    valid_rvalue = false;
    return false;
  }
  auto *sym_type = sym->get_type();
  if (sym_type != TYPE(LST_INTEGER) && sym_type != TYPE(LST_FLOATINGPOINT)) {
    valid_rvalue = false;
    return false;
  }
  // For some reason TRUE and FALSE are lexed a special way that doesn't allow unary minus
  // in the global context.
  if (!strcmp(sym->get_name(), "FALSE") || !strcmp(sym->get_name(), "TRUE")) {
    valid_rvalue = false;
    return false;
  }
  return true;
}

bool SimpleAssignableValidatingVisitor::visit(LSLLValueExpression *node) {
  // no member accessors allowed!
  auto *member = node->get_child(1);
  if (member && member->get_node_type() != NODE_NULL) {
    valid_rvalue = false;
    return false;
  }
  return true;
}

}
