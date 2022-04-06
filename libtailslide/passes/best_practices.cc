#include "best_practices.hh"

namespace Tailslide {


static bool allret(LSLASTNode *p) {
  bool ret = false;
  if (p->get_node_type() == NODE_STATEMENT && p->get_node_sub_type() == NODE_RETURN_STATEMENT) {
    // TODO check next value here for unreachable code
    return true;
  } else if (p->get_node_type() == NODE_STATEMENT && p->get_node_sub_type() == NODE_IF_STATEMENT) {
    bool true_branch = p->get_child(1) && allret(p->get_child(1));
    bool false_branch = p->get_child(2) && allret(p->get_child(2));

    return (true_branch && false_branch);
  } else if (p->get_node_type() == NODE_STATEMENT && p->get_node_sub_type() == NODE_COMPOUND_STATEMENT) {
    for (LSLASTNode *q = p->get_children(); q; q = q->get_next()) {
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

bool BestPracticesVisitor::visit(LSLGlobalFunction* node) {
  auto *id = (LSLIdentifier *) node->get_child(0);
  //LSLFunctionDec *decl = (LSLFunctionDec *) get_child(1);
  auto *statement = (LSLStatement *) node->get_child(2);

  if (id->get_symbol() != nullptr) {
    LSLType *tipe = id->get_symbol()->get_type();

    if (tipe->get_itype() != LST_NULL && !allret(statement)) {
      NODE_ERROR(node->get_child(0), E_NOT_ALL_PATHS_RETURN);
    }
  }
  return true;
}

bool BestPracticesVisitor::visit(LSLIfStatement *node) {
  // see if expression is constant
  LSLASTNode *cond = node->get_child(0);
  if (cond->get_constant_value() != nullptr) {
    // TODO: can conditions be something other than integer?
    // ^ Yep, `key`s for one, and probably a bunch of others.
    // `if (key_foo) {}` is the fastest way to validate UUIDs in LSL.
    if (cond->get_constant_value()->get_node_sub_type() == NODE_INTEGER_CONSTANT) {
      if (((LSLIntegerConstant *) cond->get_constant_value())->get_value()) {
        NODE_ERROR(cond, W_CONDITION_ALWAYS_TRUE);
      } else {
        NODE_ERROR(cond, W_CONDITION_ALWAYS_FALSE);
      }
    }
  }

  // set if expression is an assignment
  if (cond->get_node_type() == NODE_EXPRESSION) {
    auto *expr = (LSLExpression *) cond;
    if (expr->get_operation() == '=') {
      NODE_ERROR(expr, W_ASSIGNMENT_IN_COMPARISON);
    }
  }
  return true;
}

bool BestPracticesVisitor::visit(LSLBinaryExpression *node) {
  LSLConstant *left_cv = node->get_child(0)->get_constant_value();
  LSLConstant *right_cv = node->get_child(1)->get_constant_value();

  if (!left_cv || !right_cv)
    return true;

  if (left_cv->get_type() != TYPE(LST_LIST) || left_cv->get_type() != TYPE(LST_LIST))
    return true;

  auto *left_lcv = (LSLListConstant *)left_cv;
  auto *right_lcv = (LSLListConstant *)right_cv;

  switch (node->get_operation()) {
    case NEQ:
    case EQ: {
      // warn on list == list unless it's against an empty list constant,
      // integer len = (list_val != []) is a common pattern for getting the length of a list.
      if (left_lcv->get_length() != 0 && right_lcv->get_length() != 0) {
        NODE_ERROR(node, W_LIST_COMPARE);
      }
    }
  }
  return true;
}

}
