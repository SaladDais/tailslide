#include "determine_reachable.hh"

namespace Sling {


static bool allret(LLASTNode *p) {
  bool ret = false;
  if (p->get_node_type() == NODE_STATEMENT && p->get_node_sub_type() == NODE_RETURN_STATEMENT) {
    // TODO check next value here for unreachable code
    return true;
  } else if (p->get_node_type() == NODE_STATEMENT && p->get_node_sub_type() == NODE_IF_STATEMENT) {
    bool true_branch = p->get_child(1) && allret(p->get_child(1));
    bool false_branch = p->get_child(2) && allret(p->get_child(2));

    return (true_branch && false_branch);
  } else if (p->get_node_type() == NODE_STATEMENT && p->get_node_sub_type() == NODE_COMPOUND_STATEMENT) {
    for (LLASTNode *q = p->get_children(); q; q = q->get_next()) {
      ret |= allret(q);
    }
  } else {
#if 0
    if (p->get_next()) {
       ret |= allret(p->get_next());
    }
    if (p->get_children()) {
       ret |= allret(p->get_children());
    }
#endif
  }
  return ret;
}

bool DetermineReachableVisitor::visit(LLScriptGlobalFunction* node) {
  auto *id = (LLScriptIdentifier *) node->get_child(0);
  //LLScriptFunctionDec *decl = (LLScriptFunctionDec *) get_child(1);
  auto *statement = (LLScriptStatement *) node->get_child(2);

  if (id->get_symbol() != nullptr) {
    LLScriptType *tipe = id->get_symbol()->get_type();

    if (tipe->get_itype() != LST_NULL && !allret(statement)) {
      ERROR(IN(node->get_child(0)), E_NOT_ALL_PATHS_RETURN);
    }
  }
  return true;
};

bool DetermineReachableVisitor::visit(LLScriptIfStatement *node) {
  // see if expression is constant
  LLASTNode *cond = node->get_child(0);
  if (cond->get_constant_value() != nullptr) {
    // TODO: can conditions be something other than integer?
    // ^ Yep, `key`s for one, and probably a bunch of others.
    // `if (key_foo) {}` is the fastest way to validate UUIDs in LSL.
    if (cond->get_constant_value()->get_node_sub_type() == NODE_INTEGER_CONSTANT) {
      if (((LLScriptIntegerConstant *) cond->get_constant_value())->get_value()) {
        ERROR(IN(cond), W_CONDITION_ALWAYS_TRUE);
      } else {
        ERROR(IN(cond), W_CONDITION_ALWAYS_FALSE);
      }
    }
  }

  // set if expression is an assignment
  if (cond->get_node_type() == NODE_EXPRESSION) {
    auto *expr = (LLScriptExpression *) cond;
    if (expr->get_operation() == '=') {
      ERROR(IN(expr), W_ASSIGNMENT_IN_COMPARISON);
    }
  }
  return true;
}

}
