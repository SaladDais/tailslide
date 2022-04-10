#include "best_practices.hh"

namespace Tailslide {

class AllPathsReturnVisitor: public ASTVisitor {
  protected:
    virtual bool visit(LSLCompoundStatement *node) {
      all_return = false;
      return true;
    };
    virtual bool visit(LSLStatement *node) {
      all_return = false;
      return false;
    };
    virtual bool visit(LSLReturnStatement *node) {
      all_return = true;
      return false;
    };
    virtual bool visit(LSLIfStatement *node) {
      auto* true_branch = node->get_child(1);
      auto* false_branch = node->get_child(2);

      // no false path, so all paths within `if` _couldn't_ return.
      if (false_branch->get_node_type() == NODE_NULL) {
        all_return = false;
        return false;
      }

      true_branch->visit(this);
      bool true_ret = all_return;
      false_branch->visit(this);
      bool false_ret = all_return;
      all_return = true_ret && false_ret;
      return false;
    }
  public:
    bool all_return = false;
};

bool BestPracticesVisitor::visit(LSLGlobalFunction* node) {
  auto *id = (LSLIdentifier *) node->get_child(0);
  auto *statement = (LSLStatement *) node->get_child(2);

  // this function has a non-null return type
  if (id->get_itype() != LST_NULL) {
    AllPathsReturnVisitor visitor;
    statement->visit(&visitor);
    if (!visitor.all_return) {
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
