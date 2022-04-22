#include "best_practices.hh"

namespace Tailslide {

class AllPathsReturnVisitor: public ASTVisitor {
  protected:
    virtual bool visit(LSLCompoundStatement *node) {
      mAllReturn = false;
      return true;
    };
    virtual bool visit(LSLStatement *node) {
      mAllReturn = false;
      return false;
    };
    virtual bool visit(LSLReturnStatement *node) {
      mAllReturn = true;
      return false;
    };
    virtual bool visit(LSLIfStatement *node) {
      auto* true_branch = node->getChild(1);
      auto* false_branch = node->getChild(2);

      // no false path, so all paths within `if` _couldn't_ return.
      if (false_branch->getNodeType() == NODE_NULL) {
        mAllReturn = false;
        return false;
      }

      true_branch->visit(this);
      bool true_ret = mAllReturn;
      false_branch->visit(this);
      bool false_ret = mAllReturn;
      mAllReturn = true_ret && false_ret;
      return false;
    }
  public:
    bool mAllReturn = false;
};

bool BestPracticesVisitor::visit(LSLGlobalFunction* node) {
  auto *id = (LSLIdentifier *) node->getChild(0);
  auto *statement = (LSLStatement *) node->getChild(2);

  AllPathsReturnVisitor visitor;
  statement->visit(&visitor);
  // this function has a non-null return type, it requires explicit returns.
  if (id->getIType() != LST_NULL && !visitor.mAllReturn) {
    NODE_ERROR(id, E_NOT_ALL_PATHS_RETURN);
  }
  if (auto *sym = node->getSymbol()) {
    sym->setAllPathsReturn(visitor.mAllReturn);
  }
  return true;
}

bool BestPracticesVisitor::visit(LSLEventHandler *node) {
  auto *statement = (LSLStatement *) node->getChild(2);
  AllPathsReturnVisitor visitor;
  statement->visit(&visitor);
  if (auto *sym = node->getSymbol()) {
    sym->setAllPathsReturn(visitor.mAllReturn);
  }
  return true;
}

bool BestPracticesVisitor::visit(LSLIfStatement *node) {
  // see if expression is constant
  LSLASTNode *cond = node->getChild(0);
  if (cond->getConstantValue() != nullptr) {
    // TODO: can conditions be something other than integer?
    // ^ Yep, `key`s for one, and probably a bunch of others.
    // `if (key_foo) {}` is the fastest way to validate UUIDs in LSL.
    if (cond->getConstantValue()->getNodeSubType() == NODE_INTEGER_CONSTANT) {
      if (((LSLIntegerConstant *) cond->getConstantValue())->getValue()) {
        NODE_ERROR(cond, W_CONDITION_ALWAYS_TRUE);
      } else {
        NODE_ERROR(cond, W_CONDITION_ALWAYS_FALSE);
      }
    }
  }

  // set if expression is an assignment
  if (cond->getNodeType() == NODE_EXPRESSION) {
    auto *expr = (LSLExpression *) cond;
    if (expr->getOperation() == '=') {
      NODE_ERROR(expr, W_ASSIGNMENT_IN_COMPARISON);
    }
  }
  return true;
}

bool BestPracticesVisitor::visit(LSLBinaryExpression *node) {
  LSLConstant *left_cv = node->getChild(0)->getConstantValue();
  LSLConstant *right_cv = node->getChild(1)->getConstantValue();

  if (!left_cv || !right_cv)
    return true;

  if (left_cv->getType() != TYPE(LST_LIST) || left_cv->getType() != TYPE(LST_LIST))
    return true;

  auto *left_lcv = (LSLListConstant *)left_cv;
  auto *right_lcv = (LSLListConstant *)right_cv;

  switch (node->getOperation()) {
    case NEQ:
    case EQ: {
      // warn on list == list unless it's against an empty list constant,
      // integer len = (list_val != []) is a common pattern for getting the length of a list.
      if (left_lcv->getLength() != 0 && right_lcv->getLength() != 0) {
        NODE_ERROR(node, W_LIST_COMPARE);
      }
    }
  }
  return true;
}

bool BestPracticesVisitor::visit(LSLExpressionStatement *node) {
  auto *expr = (LSLExpression *)node->getChild(0);
  if (expr->getOperation() == EQ) {
    NODE_ERROR(node, W_EQ_AS_STATEMENT);
  }
  return true;
}

}
