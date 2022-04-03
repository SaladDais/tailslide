#include "lslmini.hh"
#include "values.hh"

namespace Tailslide {

bool ConstantDeterminingVisitor::before_descend(LSLASTNode *node) {
  // invalidate any old constant value we had, it might not be valid anymore
  if (!node->is_static() && node->get_node_type() != NODE_CONSTANT) {
    node->set_constant_value(nullptr);
    node->set_constant_precluded(false);
  }

  if (node->get_type() == TYPE(LST_ERROR)) {
    // absolutely no chance of us figuring out a constant value
    // for this node, don't descend.
    node->set_constant_precluded(true);
    return false;
  }

  // need special iteration order for script nodes
  // don't automatically descend!
  if (node->get_node_type() == NODE_SCRIPT)
    return false;
  return true;
}

bool ConstantDeterminingVisitor::visit(LSLScript *node) {
  // need to iterate over global vars FIRST since expressions in
  // global functions may make use of them.
  LSLASTNode *child = node->get_children();
  while (child != nullptr) {
    // passed the end of the list of globals
    if (child->get_node_type() != NODE_GLOBAL_STORAGE)
      break;
    if (LSLASTNode *gs_child = child->get_child(0)) {
      if (gs_child->get_node_type() == NODE_GLOBAL_VARIABLE)
        gs_child->visit(this);
    }
    child = child->get_next();
  }
  // safe to descend into functions and event handlers now
  visit_children(node);
  return false;
}

bool ConstantDeterminingVisitor::visit(LSLDeclaration *node) {
  auto *id = (LSLIdentifier *) node->get_child(0);
  LSLASTNode *rvalue = node->get_child(1);
  LSLConstant *cv = nullptr;
  bool cv_precluded = false;
  if (rvalue && rvalue->get_node_type() != NODE_NULL) {
    cv = rvalue->get_constant_value();
    cv_precluded = rvalue->get_constant_precluded();
  }
  DEBUG(LOG_DEBUG_SPAM, nullptr,
      "set %s const to %p\n",
      id->get_name(),
      rvalue ? rvalue->get_constant_value(): nullptr
  );
  auto *sym = id->get_symbol();
  sym->set_constant_value(cv);
  sym->set_constant_precluded(cv_precluded);
  return false;
}

bool ConstantDeterminingVisitor::visit(LSLExpression *node) {
  int operation = node->get_operation();
  LSLConstant *constant_value = node->get_constant_value();
  DEBUG(
      LOG_DEBUG_SPAM,
      nullptr,
      "expression.determine_value() op=%d cv=%s st=%d\n",
      operation,
      constant_value ? constant_value->get_node_name() : nullptr,
      node->get_node_sub_type()
  );

  LSLASTNode *left = node->get_child(0);
  LSLASTNode *right = node->get_child(1);

  if (left->get_type() == TYPE(LST_ERROR) || (right && right->get_type() == TYPE(LST_ERROR))) {
    node->set_constant_precluded(true);
    return true;
  }

  // only check normal expressions
  switch (node->get_node_sub_type()) {
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
    constant_value = left->get_constant_value();
  else if (operation == '=') {
    assert(right);
    constant_value = right->get_constant_value();
  } else {
    LSLConstant *c_left = left->get_constant_value();
    LSLConstant *c_right = right ? right->get_constant_value() : nullptr;

    // we need a constant value from the c_left, and if we have a c_right side, it MUST have a constant value too
    if (c_left && (right == nullptr || c_right != nullptr))
      constant_value = operation_behavior->operation(operation, c_left, c_right, node->get_lloc());
    else
      constant_value = nullptr;
  }
  node->set_constant_value(constant_value);
  return true;
}

bool ConstantDeterminingVisitor::visit(LSLGlobalVariable *node) {
  // if it's initialized, set its constant value
  auto *identifier = (LSLIdentifier *) node->get_child(0);
  auto *sym = identifier->get_symbol();
  LSLASTNode *rvalue = node->get_child(1);
  if (rvalue) {
    sym->set_constant_value(rvalue->get_constant_value());
    sym->set_constant_precluded(rvalue->get_constant_precluded());
  }
  return true;
}

bool ConstantDeterminingVisitor::visit(LSLLValueExpression *node) {
  auto *id = (LSLIdentifier*)node->get_child(0);
  LSLSymbol *symbol = id->get_symbol();

  // can't determine value if we don't have a symbol
  if (symbol == nullptr) {
    node->set_constant_value(nullptr);
    node->set_constant_precluded(true);
    return true;
  }

  auto *member_id = (LSLIdentifier*)node->get_child(1);
  const char *member = nullptr;
  if (member_id && member_id->get_node_type() == NODE_IDENTIFIER)
    member = ((LSLIdentifier*)member_id)->get_name();

  LSLConstant *constant_value = nullptr;
  DEBUG(LOG_DEBUG_SPAM, nullptr, "id %s assigned %d times\n", id->get_name(), symbol->get_assignments());
  if (symbol->get_assignments() == 0) {
    constant_value = symbol->get_constant_value();
    if (constant_value != nullptr && member != nullptr) { // getting a member
      LST_TYPE c_type = constant_value->get_type()->get_itype();
      switch (c_type) {
        case LST_VECTOR: {
          auto *c = (LSLVectorConstant *) constant_value;
          auto *v = (LSLVector *) c->get_value();
          assert(v);
          switch (member[0]) {
            case 'x':
              constant_value = gAllocationManager->new_tracked<LSLFloatConstant>(v->x);
              break;
            case 'y':
              constant_value = gAllocationManager->new_tracked<LSLFloatConstant>(v->y);
              break;
            case 'z':
              constant_value = gAllocationManager->new_tracked<LSLFloatConstant>(v->z);
              break;
            default:
              constant_value = nullptr;
          }
          break;
        }
        case LST_QUATERNION: {
          auto *c = (LSLQuaternionConstant *) constant_value;
          auto *v = (LSLQuaternion *) c->get_value();
          assert(v);
          switch (member[0]) {
            case 'x':
              constant_value = gAllocationManager->new_tracked<LSLFloatConstant>(v->x);
              break;
            case 'y':
              constant_value = gAllocationManager->new_tracked<LSLFloatConstant>(v->y);
              break;
            case 'z':
              constant_value = gAllocationManager->new_tracked<LSLFloatConstant>(v->z);
              break;
            case 's':
              constant_value = gAllocationManager->new_tracked<LSLFloatConstant>(v->s);
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
  node->set_constant_value(constant_value);
  return true;
}

bool ConstantDeterminingVisitor::visit(LSLListExpression *node) {
  LSLASTNode *child = node->get_children();
  LSLConstant *new_child = nullptr;

  // if we have children
  if (child->get_node_type() != NODE_NULL) {
    // make sure they are all constant
    for (child = node->get_children(); child; child = child->get_next()) {
      if (!child->is_constant()) {
        node->set_constant_precluded(child->get_constant_precluded());
        return true;
      }
    }

    // create assignables for them
    for (child = node->get_children(); child; child = child->get_next()) {
      if (new_child == nullptr) {
        new_child = child->get_constant_value()->copy();
      } else {
        new_child->add_next_sibling(child->get_constant_value()->copy());
      }
    }
  }

  // create constant value
  node->set_constant_value(gAllocationManager->new_tracked<LSLListConstant>(new_child));
  return true;
}

bool ConstantDeterminingVisitor::visit(LSLVectorExpression *node) {
  float v[3];
  int cv = 0;

  for (LSLASTNode *child = node->get_children(); child; child = child->get_next()) {
    // if we have too many children, make sure we don't overflow cv
    if (cv >= 3)
      return true;

    // all children must be constant
    if (!child->is_constant()) {
      node->set_constant_precluded(child->get_constant_precluded());
      return true;
    }

    // all children must be float/int constants - get their val or bail if they're wrong
    switch (child->get_constant_value()->get_type()->get_itype()) {
      case LST_FLOATINGPOINT:
        v[cv++] = ((LSLFloatConstant *) child->get_constant_value())->get_value();
        break;
      case LST_INTEGER:
        v[cv++] = (F32) ((LSLIntegerConstant *) child->get_constant_value())->get_value();
        break;
      default:
        node->set_constant_precluded(true);
        return true;
    }
  }

  // make sure we had enough children
  if (cv < 3)
    return true;

  // create constant value
  node->set_constant_value(gAllocationManager->new_tracked<LSLVectorConstant>(v[0], v[1], v[2]));
  return true;
}


bool ConstantDeterminingVisitor::visit(LSLQuaternionExpression *node) {
  float v[4];
  int cv = 0;

  for (LSLASTNode *child = node->get_children(); child; child = child->get_next()) {
    // if we have too many children, make sure we don't overflow cv
    if (cv >= 4)
      return true;

    // all children must be constant
    if (!child->is_constant()) {
      node->set_constant_precluded(child->get_constant_precluded());
      return true;
    }

    // all children must be float/int constants - get their val or bail if they're wrong
    switch (child->get_constant_value()->get_type()->get_itype()) {
      case LST_FLOATINGPOINT:
        v[cv++] = ((LSLFloatConstant *) child->get_constant_value())->get_value();
        break;
      case LST_INTEGER:
        v[cv++] = (F32) ((LSLIntegerConstant *) child->get_constant_value())->get_value();
        break;
      default:
        node->set_constant_precluded(true);
        return true;
    }
  }

  // make sure we had enough children
  if (cv < 4)
    return true;

  // create constant value
  node->set_constant_value(gAllocationManager->new_tracked<LSLQuaternionConstant>(v[0], v[1], v[2], v[3]));
  return true;
}

bool ConstantDeterminingVisitor::visit(LSLTypecastExpression *node) {
  // what are we casting to
  auto *child = node->get_child(0);
  auto *val = child->get_constant_value();
  node->set_constant_value(nullptr);
  if (!val) {
    node->set_constant_precluded(child->get_constant_precluded());
    return true;
  }
  auto to_type = node->get_type();
  node->set_constant_value(operation_behavior->cast(to_type, val, val->get_lloc()));
  return true;
}


}
