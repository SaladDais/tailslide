#include "../lslmini.hh"

#include "values.hh"


namespace Tailslide {

bool ConstantDeterminingVisitor::beforeDescend(LSLASTNode *node) {
  // invalidate any old constant value we had, it might not be valid anymore
  if (!node->isStatic() && node->getNodeType() != NODE_CONSTANT) {
    node->setConstantValue(nullptr);
    node->setConstantPrecluded(false);
  }

  if (node->getType() == TYPE(LST_ERROR)) {
    // absolutely no chance of us figuring out a constant value
    // for this node, don't descend.
    node->setConstantPrecluded(true);
    return false;
  }

  // need special iteration order for script nodes
  // don't automatically descend!
  if (node->getNodeType() == NODE_SCRIPT)
    return false;
  return true;
}

bool ConstantDeterminingVisitor::visit(LSLScript *node) {
  // need to iterate over global vars FIRST since expressions in
  // global functions may make use of them.
  for (auto *child : *node->getChild(0)) {
    if (child->getNodeType() == NODE_GLOBAL_VARIABLE)
      child->visit(this);
  }
  // safe to descend into functions and event handlers now
  visitChildren(node);
  return false;
}

bool ConstantDeterminingVisitor::visit(LSLDeclaration *node) {
  handleDeclaration(node);
  return false;
}

void ConstantDeterminingVisitor::handleDeclaration(LSLASTNode *node) {
  // if it's initialized, set its constant value
  auto *sym = node->getSymbol();
  LSLASTNode *rvalue = node->getChild(1);
  if (rvalue && rvalue->getNodeType() != NODE_NULL) {
    sym->setConstantPrecluded(rvalue->getConstantPrecluded());
    if (rvalue->getConstantPrecluded()) {
      // Can't determine a value due to a type or symbol error somewhere
      sym->setConstantValue(nullptr);
    } else {
      auto *cv = rvalue->getConstantValue();
      // Must be a case where we're expecting automatic type promotion,
      // perform it on the rvalue's constant value before assigning it to the symbol
      if (cv && cv->getType() != sym->getType() && cv->getType()->canCoerce(sym->getType())) {
        cv = _mOperationBehavior->cast(sym->getType(), cv, cv->getLoc());
      }
      sym->setConstantValue(cv);
    }
  } else {
    sym->setConstantValue(sym->getType()->getDefaultValue());
  }
}

bool ConstantDeterminingVisitor::visit(LSLExpression *node) {
  LSLOperator operation = node->getOperation();
  LSLConstant *constant_value = node->getConstantValue();
  DEBUG(
      LOG_DEBUG_SPAM,
      nullptr,
      "expression.determine_value() op=%d cv=%s st=%d\n",
      operation,
      constant_value ? constant_value->getNodeName() : nullptr,
      node->getNodeSubType()
  );

  LSLASTNode *left = node->getChild(0);
  LSLASTNode *right = node->getChild(1);

  if (left->getType() == TYPE(LST_ERROR) || (right && right->getType() == TYPE(LST_ERROR))) {
    node->setConstantPrecluded(true);
    return true;
  }

  // only check normal expressions
  switch (node->getNodeSubType()) {
    case NODE_NO_SUB_TYPE:
    case NODE_CONSTANT_EXPRESSION:
    case NODE_PARENTHESIS_EXPRESSION:
    case NODE_BINARY_EXPRESSION:
    case NODE_UNARY_EXPRESSION:
      break;
    default:
      return true;
  }

  if (operation == 0 || operation == '(')
    constant_value = left->getConstantValue();
  else if (operation == '=') {
    assert(right);
    constant_value = right->getConstantValue();
  } else {
    LSLConstant *c_left = left->getConstantValue();
    LSLConstant *c_right = right ? right->getConstantValue() : nullptr;

    // we need a constant value from the c_left, and if we have a c_right side, it MUST have a constant value too
    if (c_left && (right == nullptr || c_right != nullptr))
      constant_value = _mOperationBehavior->operation(operation, c_left, c_right, node->getLoc());
    else
      constant_value = nullptr;
  }
  node->setConstantValue(constant_value);
  return true;
}

bool ConstantDeterminingVisitor::visit(LSLGlobalVariable *node) {
  handleDeclaration(node);
  return false;
}

bool ConstantDeterminingVisitor::visit(LSLLValueExpression *node) {
  LSLSymbol *symbol = node->getSymbol();

  // can't determine value if we don't have a symbol
  if (symbol == nullptr) {
    node->setConstantValue(nullptr);
    node->setConstantPrecluded(true);
    return true;
  }

  auto *member_id = (LSLIdentifier*) node->getChild(1);
  const char *member_name = nullptr;
  if (member_id && member_id->getNodeType() == NODE_IDENTIFIER)
    member_name = member_id->getName();

  LSLConstant *constant_value = nullptr;
  DEBUG(LOG_DEBUG_SPAM, nullptr, "id %s assigned %d times\n", symbol->getName(), symbol->getAssignments());
  if (symbol->getAssignments() == 0) {
    constant_value = symbol->getConstantValue();
    if (constant_value != nullptr && member_name != nullptr) { // getting a member_name
      LSLIType c_type = constant_value->getIType();
      switch (c_type) {
        case LST_VECTOR: {
          auto *c = (LSLVectorConstant *) constant_value;
          auto *v = (Vector3 *) c->getValue();
          assert(v);
          switch (member_name[0]) {
            case 'x':
              constant_value = _mAllocator->newTracked<LSLFloatConstant>(v->x);
              break;
            case 'y':
              constant_value = _mAllocator->newTracked<LSLFloatConstant>(v->y);
              break;
            case 'z':
              constant_value = _mAllocator->newTracked<LSLFloatConstant>(v->z);
              break;
            default:
              constant_value = nullptr;
          }
          break;
        }
        case LST_QUATERNION: {
          auto *c = (LSLQuaternionConstant *) constant_value;
          auto *v = (Quaternion *) c->getValue();
          assert(v);
          switch (member_name[0]) {
            case 'x':
              constant_value = _mAllocator->newTracked<LSLFloatConstant>(v->x);
              break;
            case 'y':
              constant_value = _mAllocator->newTracked<LSLFloatConstant>(v->y);
              break;
            case 'z':
              constant_value = _mAllocator->newTracked<LSLFloatConstant>(v->z);
              break;
            case 's':
              constant_value = _mAllocator->newTracked<LSLFloatConstant>(v->s);
              break;
            default:
              constant_value = nullptr;
          }
          break;
        }
        default:
          constant_value = nullptr;
          break;
      }
    }
  }
  node->setConstantValue(constant_value);
  return true;
}

bool ConstantDeterminingVisitor::visit(LSLListExpression *node) {
  auto *new_list_cv = _mAllocator->newTracked<LSLListConstant>(nullptr);

  // make sure they are all constant
  for (auto *child : *node) {
    if (!child->isConstant()) {
      node->setConstantPrecluded(child->getConstantPrecluded());
      return true;
    }
  }

  // create assignables for them
  for (auto *child : *node) {
    new_list_cv->pushChild(child->getConstantValue()->copy(_mAllocator));
  }

  // create constant value
  node->setConstantValue(new_list_cv);
  return true;
}

bool ConstantDeterminingVisitor::visit(LSLVectorExpression *node) {
  float v[3];
  int cv = 0;

  for (auto *child : *node) {
    // if we have too many children, make sure we don't overflow cv
    if (cv >= 3)
      return true;

    // all children must be constant
    if (!child->isConstant()) {
      node->setConstantPrecluded(child->getConstantPrecluded());
      return true;
    }

    // all children must be float/int constants - get their val or bail if they're wrong
    switch (child->getConstantValue()->getIType()) {
      case LST_FLOATINGPOINT:
        v[cv++] = ((LSLFloatConstant *) child->getConstantValue())->getValue();
        break;
      case LST_INTEGER:
        v[cv++] = (F32) ((LSLIntegerConstant *) child->getConstantValue())->getValue();
        break;
      default:
        node->setConstantPrecluded(true);
        return true;
    }
  }

  // make sure we had enough children
  if (cv < 3)
    return true;

  // create constant value
  node->setConstantValue(_mAllocator->newTracked<LSLVectorConstant>(v[0], v[1], v[2]));
  return true;
}


bool ConstantDeterminingVisitor::visit(LSLQuaternionExpression *node) {
  float v[4];
  int cv = 0;

  for (auto *child : *node) {
    // if we have too many children, make sure we don't overflow cv
    if (cv >= 4)
      return true;

    // all children must be constant
    if (!child->isConstant()) {
      node->setConstantPrecluded(child->getConstantPrecluded());
      return true;
    }

    // all children must be float/int constants - get their val or bail if they're wrong
    switch (child->getConstantValue()->getIType()) {
      case LST_FLOATINGPOINT:
        v[cv++] = ((LSLFloatConstant *) child->getConstantValue())->getValue();
        break;
      case LST_INTEGER:
        v[cv++] = (F32) ((LSLIntegerConstant *) child->getConstantValue())->getValue();
        break;
      default:
        node->setConstantPrecluded(true);
        return true;
    }
  }

  // make sure we had enough children
  if (cv < 4)
    return true;

  // create constant value
  node->setConstantValue(_mAllocator->newTracked<LSLQuaternionConstant>(v[0], v[1], v[2], v[3]));
  return true;
}

bool ConstantDeterminingVisitor::visit(LSLTypecastExpression *node) {
  // what are we casting to
  auto *child = node->getChild(0);
  auto *val = child->getConstantValue();
  node->setConstantValue(nullptr);
  if (!val) {
    node->setConstantPrecluded(child->getConstantPrecluded());
    return true;
  }
  auto to_type = node->getType();
  node->setConstantValue(_mOperationBehavior->cast(to_type, val, val->getLoc()));
  return true;
}


}
