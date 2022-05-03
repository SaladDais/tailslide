#include "desugaring.hh"
#include <vector>

namespace Tailslide {

// any time an int appears in a binary expression with these it must be promoted
std::vector<LSLIType> SIBLINGS_CAUSING_INT_PROMOTION = {
    LST_FLOATINGPOINT,
    LST_VECTOR,
    LST_QUATERNION,
};

bool DeSugaringVisitor::visit(LSLBinaryExpression *node) {
  LSLOperator op = node->getOperation();
  LSLOperator decoupled_op = decouple_compound_operation(op);
  bool compound_assignment = op != decoupled_op;

  auto *left = node->getLHS();
  auto *right = node->getRHS();

  if (left->getIType() == LST_ERROR || right->getIType() == LST_ERROR)
    return true;

  // This is effectively NOT syntactic sugar and needs to be handled specially by backends.
  if (op == OP_MUL_ASSIGN && left->getIType() == LST_INTEGER && right->getIType() == LST_FLOATINGPOINT) {
    return true;
  }

  // plain '=', just promote to the given type
  if (decoupled_op == '=') {
    maybeInjectCast(right, left->getType());
    return true;
  }

  if (_mMonoSemantics) {
    // note that `int <op> float` and `float <op> int` are NOT just syntactic sugar in LSO, they
    // compile to different opcodes than if you de-sugared to `(float)int <op> float`!
    // but they ARE syntactic sugar in Mono. Mono has no int <op> float functions so we need to promote
    // in all places where the LSO VM was promoting internally in its opcode handlers.
    //
    // This should be fine since by this point there shouldn't be any promotable types on the left side
    // of an assignment expression.
    for (auto int_sibling : SIBLINGS_CAUSING_INT_PROMOTION) {
      if (left->getIType() == int_sibling && right->getIType() == LST_INTEGER) {
        maybeInjectCast(right, TYPE(LST_FLOATINGPOINT));
        break;
      } else if (left->getIType() == LST_INTEGER && right->getIType() == int_sibling) {
        assert(!compound_assignment || left->getNodeSubType() != NODE_LVALUE_EXPRESSION);
        maybeInjectCast(left, TYPE(LST_FLOATINGPOINT));
        break;
      }
    }
  }

  if (!compound_assignment)
    return true;

  // some kind of compound operator, desugar it

  // decouple the RHS from the existing expression
  // TODO: some kind of take() on the node itself that leaves a null node in its place
  LSLASTNode::replaceNode(right, node->newNullNode());

  // turn `lhs += rhs` into `lhs = lhs + rhs`
  auto *new_right_node = _mAllocator->newTracked<LSLBinaryExpression>(
      ((LSLLValueExpression *) left)->clone(),
      decoupled_op,
      right
  );
  new_right_node->setType(node->getType());
  new_right_node->setLoc(node->getLoc());
  node->setRHS(new_right_node);
  node->setOperation(OP_ASSIGN);
  return true;
}

bool DeSugaringVisitor::visit(LSLUnaryExpression *node) {
  LSLOperator new_op;
  if (node->getIType() == LST_ERROR)
    return true;

  // these compile to a different form than `foo = foo + 1` in mono,
  // don't desugar.
  if (_mMonoSemantics)
    return true;

  // the post operations are never syntactic sugar,
  // so no way to desugar those.
  switch(node->getOperation()) {
    case OP_PRE_INCR: new_op = OP_PLUS; break;
    case OP_PRE_DECR: new_op = OP_MINUS; break;
    default:
      return true;
  }

  auto *lvalue = (LSLLValueExpression*) node->takeChild(0);
  auto *lvalue_copy = lvalue->clone();
  // "1" for the given type
  LSLConstant *cv = lvalue->getType()->getOneValue();
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
      OP_ASSIGN,
      new_rvalue
  );
  assign_expr->setType(node->getType());
  LSLASTNode::replaceNode(node, assign_expr);
  return true;
}

bool DeSugaringVisitor::visit(LSLDeclaration *node) {
  if (auto expr = node->getInitializer())
    maybeInjectCast(expr, node->getIdentifier()->getType());
  return true;
}

bool DeSugaringVisitor::visit(LSLGlobalVariable *node) {
  // LSO has its own special handling for global var casting.
  if (!_mMonoSemantics)
    return true;
  if (auto expr = node->getInitializer())
    maybeInjectCast(expr, node->getIdentifier()->getType());
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

void DeSugaringVisitor::maybeInjectBoolConversion(LSLExpression* expr) {
  // LSO has typed branching instructions already, they do this internally!
  if (!_mMonoSemantics)
    return;
  // this dirties the node for constant value propagation purposes.
  auto *expr_placeholder = expr->newNullNode();
  LSLASTNode::replaceNode(expr, expr_placeholder);
  auto *bool_convert = _mAllocator->newTracked<LSLBoolConversionExpression>(expr);
  LSLASTNode::replaceNode(expr_placeholder, bool_convert);
  bool_convert->setLoc(expr->getLoc());
}

bool DeSugaringVisitor::visit(LSLQuaternionExpression *node) {
  handleCoordinateExpression(node);
  return true;
}

bool DeSugaringVisitor::visit(LSLVectorExpression *node) {
  handleCoordinateExpression(node);
  return true;
}

bool DeSugaringVisitor::visit(LSLFunctionExpression *node) {
  auto *sym = node->getSymbol();
  if (!sym || sym->getIType() == LST_ERROR)
    return true;
  auto *func_decl = sym->getFunctionDecl();
  if (!func_decl || !func_decl->hasChildren()) {
    return true;
  }

  auto *params = node->getArguments();
  // we may replace nodes during iteration so we can't use `node->getNext()`
  auto num_params = params->getNumChildren();
  for(auto i=0; i<num_params; ++i) {
    auto expected_param = func_decl->getChild(i);
    auto param = params->getChild(i);
    if (!param || !expected_param)
      return true;
    maybeInjectCast((LSLExpression *) param, expected_param->getType());
  }
  return true;
}

/// Replace any builtin references with their actual values.
/// These are lexer tokens in LL's compiler, they aren't "real" globals or locals.
bool DeSugaringVisitor::visit(LSLLValueExpression *node) {
  auto *sym = node->getSymbol();
  if (sym->getSymbolType() != SYM_VARIABLE)
    return true;
  if (sym->getSubType() != SYM_BUILTIN)
    return true;
  // should always have a value for builtin lvalues by this point
  assert(node->getConstantValue());

  auto *new_expr = rewriteBuiltinLValue(node);
  new_expr->setLoc(node->getLoc());
  LSLASTNode::replaceNode(
      node,
      new_expr
  );
  return false;
}

bool DeSugaringVisitor::visit(LSLReturnStatement *node) {
  auto *expr = node->getExpr();
  if (!expr)
    return true;
  // figure out what function we're in and conditionally cast to the return type
  for (LSLASTNode *parent = expr->getParent(); parent; parent = parent->getParent()) {
    if (parent->getNodeType() == NODE_GLOBAL_FUNCTION) {
      auto *id = ((LSLGlobalVariable *) parent)->getIdentifier();
      maybeInjectCast(expr, id->getType());
      return true;
    }
  }
  return true;
}

bool DeSugaringVisitor::visit(LSLForStatement *node) {
  maybeInjectBoolConversion(node->getCheckExpr());
  return true;
}

bool DeSugaringVisitor::visit(LSLWhileStatement *node) {
  maybeInjectBoolConversion(node->getCheckExpr());
  return true;
}

bool DeSugaringVisitor::visit(LSLDoStatement *node) {
  maybeInjectBoolConversion(node->getCheckExpr());
  return true;
}

bool DeSugaringVisitor::visit(LSLIfStatement *node) {
  maybeInjectBoolConversion(node->getCheckExpr());
  return true;
}

LSLASTNode *DeSugaringVisitor::rewriteBuiltinLValue(LSLASTNode *node) {
  return _mAllocator->newTracked<LSLConstantExpression>(node->getConstantValue());
}

void DeSugaringVisitor::handleCoordinateExpression(LSLASTNode *node) {
  // we may replace nodes during iteration so we can't use `node->getNext()`
  auto num_children = node->getNumChildren();
  for(auto i=0; i<num_children; ++i) {
    maybeInjectCast((LSLExpression *) node->getChild(i), TYPE(LST_FLOATINGPOINT));
  }
}



/// turns -1 literals into -(1). Necessary for matching LL's compiler exactly.
bool LLConformantDeSugaringVisitor::visit(LSLConstantExpression *node) {
  auto *cv = node->getConstantValue();
  // Only do this to values that were _parsed_ as '-' INTEGER_CONSTANT.
  // it shouldn't be done to 0xFFffFFff or ALL_SIDES.
  if (!cv->wasNegated())
    return false;
  LSLConstant *new_cv;
  switch (cv->getIType()) {
    case LST_INTEGER:
      new_cv = _mAllocator->newTracked<LSLIntegerConstant>(-((LSLIntegerConstant *) cv)->getValue());
      break;
    case LST_FLOATINGPOINT:
      new_cv = _mAllocator->newTracked<LSLFloatConstant>(-((LSLFloatConstant *) cv)->getValue());
      break;
    default:
      return false;
  }
  new_cv->setLoc(cv->getLoc());
  auto *new_constexpr = _mAllocator->newTracked<LSLConstantExpression>(new_cv);
  new_constexpr->setLoc(node->getLoc());
  auto *neg_expr = _mAllocator->newTracked<LSLUnaryExpression>(new_constexpr, OP_MINUS);
  neg_expr->setLoc(node->getLoc());
  neg_expr->setConstantValue(cv);
  neg_expr->setType(node->getType());
  LSLASTNode::replaceNode(node, neg_expr);
  return false;
}

LSLASTNode *LLConformantDeSugaringVisitor::rewriteBuiltinLValue(LSLASTNode *node) {
  auto cv = node->getConstantValue();
  auto itype = cv->getIType();

  // this case can use the default desugaring logic
  if (itype != LST_VECTOR && itype != LST_QUATERNION)
    return DeSugaringVisitor::rewriteBuiltinLValue(node);

  // vector and quaternion builtin constants are a little special in that they'd normally
  // be parsed as vector expressions within a function context. We don't want to desugar
  // them as constantexpressions since they get serialized differently from
  // (potentially non-constant) vectorexpressions.
  std::vector<float> children;
  std::vector<LSLConstantExpression *> new_expr_children;
  if (itype == LST_VECTOR) {
    auto *vec_val = ((LSLVectorConstant *) cv)->getValue();
    children.assign({vec_val->x, vec_val->y, vec_val->z});
  } else {
    auto *quat_val = ((LSLQuaternionConstant *) cv)->getValue();
    children.assign({quat_val->x, quat_val->y, quat_val->z, quat_val->s});
  }

  for (float axis: children) {
    auto *expr_child = _mAllocator->newTracked<LSLConstantExpression>(
        _mAllocator->newTracked<LSLFloatConstant>(axis));
    expr_child->setLoc(node->getLoc());
    new_expr_children.push_back(expr_child);
  }

  LSLASTNode *new_expr;
  if (itype == LST_VECTOR) {
    new_expr = _mAllocator->newTracked<LSLVectorExpression>(
        new_expr_children[0], new_expr_children[1], new_expr_children[2]);
  } else {
    new_expr = _mAllocator->newTracked<LSLQuaternionExpression>(
        new_expr_children[0], new_expr_children[1], new_expr_children[2], new_expr_children[3]);
  }
  new_expr->setConstantValue(cv);
  return new_expr;
}

}
