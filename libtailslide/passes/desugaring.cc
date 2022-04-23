#include "desugaring.hh"

namespace Tailslide {

bool DeSugaringVisitor::visit(LSLBinaryExpression *node) {
  int decoupled_op = decouple_compound_operation(node->getOperation());

  auto *left = (LSLLValueExpression *) node->getChild(0);
  auto *right = (LSLExpression *) node->getChild(1);

  if (left->getIType() == LST_ERROR || right->getIType() == LST_ERROR)
    return true;

  if (decoupled_op == '=') {
    maybeInjectCast(right, left->getType());
    return true;
  }

  // note that `int <op> float` and `float <op> int` are NOT just syntactic sugar, they
  // compile to different opcodes than if you de-sugared to `(float)int <op> float`!
  if (node->getOperation() == decoupled_op)
    return true;
  // some kind of compound operator, desugar it
  // This is effectively NOT syntactic sugar and needs to be handled specially by backends.
  if (decoupled_op == '*' && left->getIType() == LST_INTEGER && right->getIType() == LST_FLOATINGPOINT) {
    return true;
  }
  // decouple the RHS from the existing expression
  right = (LSLExpression *) node->takeChild(1);
  // turn `lhs += rhs` into `lhs = lhs + rhs`
  auto *new_right_node = _mAllocator->newTracked<LSLBinaryExpression>(
      left->clone(),
      decoupled_op,
      right
  );
  new_right_node->setType(node->getType());
  new_right_node->setLoc(node->getLoc());
  LSLASTNode::replaceNode(node->getChild(1), new_right_node);
  node->setOperation('=');
  return true;
}

bool DeSugaringVisitor::visit(LSLUnaryExpression *node) {
  int new_op;
  if (node->getIType() == LST_ERROR)
    return true;
  // the post operations aren't actually syntactic sugar,
  // so no way to desugar those.
  switch(node->getOperation()) {
    case INC_PRE_OP: new_op = '+'; break;
    case DEC_PRE_OP: new_op = '-'; break;
    default:
      return true;
  }

  auto *lvalue = (LSLLValueExpression*) node->takeChild(0);
  auto *lvalue_copy = lvalue->clone();
  LSLConstant *cv = nullptr;
  if (node->getIType() == LST_INTEGER)
    cv = _mAllocator->newTracked<LSLIntegerConstant>(1);
  else
    cv = _mAllocator->newTracked<LSLFloatConstant>(1.f);
  auto *rhs_operand = _mAllocator->newTracked<LSLConstantExpression>(cv);

  // turn `++lhs` into `lhs = lhs + 1`
  // this dirties the node for constant value propagation purposes.
  auto *new_rvalue = _mAllocator->newTracked<LSLBinaryExpression>(
      lvalue_copy,
      new_op,
      rhs_operand
  );
  new_rvalue->setType(node->getType());
  auto *assign_expr = _mAllocator->newTracked<LSLBinaryExpression>(
      lvalue,
      '=',
      new_rvalue
  );
  assign_expr->setType(node->getType());
  LSLASTNode::replaceNode(node, assign_expr);
  return true;
}

bool DeSugaringVisitor::visit(LSLDeclaration *node) {
  auto expr = (LSLExpression *) node->getChild(1);
  if (expr->getNodeType() != NODE_NULL)
    maybeInjectCast(expr, node->getChild(0)->getType());
  return true;
}

void DeSugaringVisitor::maybeInjectCast(LSLExpression* expr, LSLType *to) {
  auto *expr_type = expr->getType();
  if (to == expr_type)
    return;
  if (!expr_type->canCoerce(to))
    return;
  // this dirties the node for constant value propagation purposes.
  auto *expr_placeholder = expr->newNullNode();
  LSLASTNode::replaceNode(expr, expr_placeholder);
  auto *typecast = _mAllocator->newTracked<LSLTypecastExpression>(
      to,
      expr
  );
  LSLASTNode::replaceNode(expr_placeholder, typecast);
  typecast->setLoc(expr->getLoc());
}

bool DeSugaringVisitor::visit(LSLQuaternionExpression *node) {
  handleCoordinateNode(node);
  return true;
}

bool DeSugaringVisitor::visit(LSLVectorExpression *node) {
  handleCoordinateNode(node);
  return true;
}

bool DeSugaringVisitor::visit(LSLFunctionExpression *node) {
  auto *sym = node->getSymbol();
  if (!sym || sym->getIType() == LST_ERROR)
    return true;
  auto *func_decl = sym->getFunctionDecl();
  if (!func_decl) {
    return true;
  }

  // we may replace nodes during iteration so we can't use `node->getNext()`
  // function exprs' children are identifier, [param, ...]
  auto num_children = node->getNumChildren() - 1;
  for(auto i=0; i<num_children; ++i) {
    auto expected_param = func_decl->getChild(i);
    auto param = node->getChild(i + 1);
    if (!param || !expected_param)
      return true;
    maybeInjectCast((LSLExpression *) param, expected_param->getType());
  }
  return true;
}

bool DeSugaringVisitor::visit(LSLReturnStatement *node) {
  auto *expr = (LSLExpression *) node->getChild(0);
  if (expr->getNodeType() == NODE_NULL)
    return true;
  // figure out what function we're in and conditionally cast to the return type
  for (LSLASTNode *parent = expr->getParent(); parent; parent = parent->getParent()) {
    if (parent->getNodeType() == NODE_GLOBAL_FUNCTION) {
      auto *id = parent->getChild(0);
      maybeInjectCast(expr, id->getType());
      return true;
    }
  }
  return true;
}

void DeSugaringVisitor::handleCoordinateNode(LSLASTNode *node) {
  // we may replace nodes during iteration so we can't use `node->getNext()`
  auto num_children = node->getNumChildren();
  for(auto i=0; i<num_children; ++i) {
    maybeInjectCast((LSLExpression *) node->getChild(i), TYPE(LST_FLOATINGPOINT));
  }
}

}
