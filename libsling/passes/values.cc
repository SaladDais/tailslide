#include "lslmini.hh"
#include "values.hh"

namespace Sling {

bool ConstantDeterminingVisitor::before_descend(LLASTNode *node) {
  // invalidate any old constant value we had, it might not be valid anymore
  if (!node->is_static() && node->get_node_type() != NODE_CONSTANT)
    node->set_constant_value(nullptr);
  // need special iteration order for script nodes
  // don't automatically descend!
  if (node->get_node_type() == NODE_SCRIPT)
    return false;
  return true;
}

bool ConstantDeterminingVisitor::visit(LLScriptScript *node) {
  // need to iterate over global vars FIRST since expressions in
  // global functions may make use of them.
  LLASTNode *child = node->get_children();
  while (child != nullptr) {
    // passed the end of the list of globals
    if (child->get_node_type() != NODE_GLOBAL_STORAGE)
      break;
    if (LLASTNode *gs_child = child->get_child(0)) {
      if (gs_child->get_node_type() == NODE_GLOBAL_VARIABLE)
        gs_child->propogate_values();
    }
    child = child->get_next();
  }
  // safe to descend into functions and event handlers now
  visit_children(node);
  return false;
}

bool ConstantDeterminingVisitor::visit(LLScriptDeclaration *node) {
  auto *id = (LLScriptIdentifier *) node->get_child(0);
  LLASTNode *rvalue = node->get_child(1);
  if (rvalue == nullptr || rvalue->get_node_type() == NODE_NULL)
    return false;
  DEBUG(LOG_DEBUG_SPAM, NULL, "set %s const to %p\n", id->get_name(), rvalue->get_constant_value());
  id->get_symbol()->set_constant_value(rvalue->get_constant_value());
  return true;
}

bool ConstantDeterminingVisitor::visit(LLScriptExpression *node) {
  int operation = node->get_operation();
  LLScriptConstant *constant_value = node->get_constant_value();
  DEBUG(
      LOG_DEBUG_SPAM,
      NULL,
      "expression.determine_value() op=%d cv=%s st=%d\n",
      operation,
      constant_value ? constant_value->get_node_name() : NULL,
      get_node_sub_type()
  );

  // Can't really be avoided in cases where the tree has been modified since
  // the value was calculated, absent some way of finding out which constant values
  // depend on the dirtied expression.
  // if (constant_value != nullptr)
  //   return; // we already have a value

  // only check normal and lvalue expressions
  switch (node->get_node_sub_type()) {
    case NODE_NO_SUB_TYPE:
    case NODE_CONSTANT_EXPRESSION:
    case NODE_PARENTHESIS_EXPRESSION:
    case NODE_BINARY_EXPRESSION:
    case NODE_UNARY_EXPRESSION:
    case NODE_LVALUE_EXPRESSION:
      break;
    default:
      return true;
  }

  if (operation == 0 || operation == '(')
    constant_value = node->get_child(0)->get_constant_value();
  else if (operation == '=')
    constant_value = node->get_child(1)->get_constant_value();
  else {
    LLScriptConstant *left = node->get_child(0)->get_constant_value();
    LLScriptConstant *right = node->get_child(1) ? node->get_child(1)->get_constant_value() : nullptr;

    // we need a constant value from the left, and if we have a right side, it MUST have a constant value too
    if (left && (node->get_child(1) == nullptr || right != nullptr))
      constant_value = left->operation(operation, right, node->get_lloc());
    else
      constant_value = nullptr;
  }
  node->set_constant_value(constant_value);
  return true;
}

bool ConstantDeterminingVisitor::visit(LLScriptGlobalVariable *node) {
  // if it's initialized, set its constant value
  auto *identifier = (LLScriptIdentifier *) node->get_child(0);
  LLASTNode *rvalue = node->get_child(1);
  if (rvalue->get_node_type() == NODE_SIMPLE_ASSIGNABLE) {
    identifier->get_symbol()->set_constant_value(rvalue->get_child(0)->get_constant_value());
  }
  return true;
}

bool ConstantDeterminingVisitor::visit(LLScriptSimpleAssignable *node) {
  // SimpleAssignables take on the value of whatever they contain
  if (node->get_child(0))
    node->set_constant_value(node->get_child(0)->get_constant_value());
  return true;
}

bool ConstantDeterminingVisitor::visit(LLScriptVectorConstant *node) {
  float v[3];
  int cv = 0;

  for (LLASTNode *child = node->get_children(); child; child = child->get_next()) {
    // if we have too many children, make sure we don't overflow cv
    if (cv >= 3)
      return true;

    // all children must be constant
    if (!child->is_constant())
      return true;

    // all children must be float/int constants - get their val or bail if they're wrong
    switch (child->get_constant_value()->get_type()->get_itype()) {
      case LST_FLOATINGPOINT:
        v[cv++] = ((LLScriptFloatConstant *) child->get_constant_value())->get_value();
        break;
      case LST_INTEGER:
        v[cv++] = (F32) ((LLScriptIntegerConstant *) child->get_constant_value())->get_value();
        break;
      default:
        return true;
    }

  }

  if (cv < 3)  // not enough children
    return true;

  node->set_value(gAllocationManager->new_tracked<LLVector>(v[0], v[1], v[2]));
  return true;
}

bool ConstantDeterminingVisitor::visit(LLScriptQuaternionConstant *node) {
  float v[4];
  int cv = 0;

  for (LLASTNode *child = node->get_children(); child; child = child->get_next()) {
    // if we have too many children, make sure we don't overflow cv
    if (cv >= 4)
      return true;

    // all children must be constant
    if (!child->is_constant())
      return true;

    // all children must be float/int constants - get their val or bail if they're wrong
    switch (child->get_constant_value()->get_type()->get_itype()) {
      case LST_FLOATINGPOINT:
        v[cv++] = ((LLScriptFloatConstant *) child->get_constant_value())->get_value();
        break;
      case LST_INTEGER:
        v[cv++] = (F32) ((LLScriptIntegerConstant *) child->get_constant_value())->get_value();
        break;
      default:
        return true;
    }

  }

  if (cv < 4) // not enough children;
    return true;

  node->set_value(gAllocationManager->new_tracked<LLQuaternion>(v[0], v[1], v[2], v[3]));
  return true;
}

bool ConstantDeterminingVisitor::visit(LLScriptIdentifier *node) {
  LLScriptSymbol *symbol = node->get_symbol();
  LLScriptConstant *constant_value = nullptr;
  const char *member = node->get_member();
  // can't determine value if we don't have a symbol
  if (symbol == nullptr) {
    node->set_constant_value(nullptr);
    return true;
  }

  DEBUG(LOG_DEBUG_SPAM, NULL, "id %s assigned %d times\n", get_name(), symbol->get_assignments());
  if (symbol->get_assignments() == 0) {
    constant_value = symbol->get_constant_value();
    if (constant_value != nullptr && member != nullptr) { // getting a member
      switch (constant_value->get_type()->get_itype()) {
        case LST_VECTOR: {
          auto *c = (LLScriptVectorConstant *) constant_value;
          auto *v = (LLVector *) c->get_value();
          if (v == nullptr) {
            constant_value = nullptr;
            break;
          }
          switch (member[0]) {
            case 'x':
              constant_value = gAllocationManager->new_tracked<LLScriptFloatConstant>(v->x);
              break;
            case 'y':
              constant_value = gAllocationManager->new_tracked<LLScriptFloatConstant>(v->y);
              break;
            case 'z':
              constant_value = gAllocationManager->new_tracked<LLScriptFloatConstant>(v->z);
              break;
            default:
              constant_value = nullptr;
          }
          break;
        }
        case LST_QUATERNION: {
          auto *c = (LLScriptQuaternionConstant *) constant_value;
          auto *v = (LLQuaternion *) c->get_value();
          if (v == nullptr) {
            constant_value = nullptr;
            break;
          }
          switch (member[0]) {
            case 'x':
              constant_value = gAllocationManager->new_tracked<LLScriptFloatConstant>(v->x);
              break;
            case 'y':
              constant_value = gAllocationManager->new_tracked<LLScriptFloatConstant>(v->y);
              break;
            case 'z':
              constant_value = gAllocationManager->new_tracked<LLScriptFloatConstant>(v->z);
              break;
            case 's':
              constant_value = gAllocationManager->new_tracked<LLScriptFloatConstant>(v->s);
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

bool ConstantDeterminingVisitor::visit(LLScriptListExpression *node) {
  LLASTNode *child = node->get_children();
  LLScriptSimpleAssignable *assignable = nullptr;

  // if we have children
  if (child->get_node_type() != NODE_NULL) {
    // make sure they are all constant
    for (child = node->get_children(); child; child = child->get_next()) {
      if (!child->is_constant())
        return true;
    }

    // create assignables for them
    for (child = node->get_children(); child; child = child->get_next()) {
      if (assignable == nullptr) {
        assignable = gAllocationManager->new_tracked<LLScriptSimpleAssignable>(child->get_constant_value());
      } else {
        assignable->add_next_sibling(
            gAllocationManager->new_tracked<LLScriptSimpleAssignable>(child->get_constant_value()));
      }
    }
  }

  // create constant value
  node->set_constant_value(gAllocationManager->new_tracked<LLScriptListConstant>(assignable));
  return true;
}

bool ConstantDeterminingVisitor::visit(LLScriptVectorExpression *node) {
  float v[3];
  int cv = 0;

  for (LLASTNode *child = node->get_children(); child; child = child->get_next()) {
    // if we have too many children, make sure we don't overflow cv
    if (cv >= 3)
      return true;

    // all children must be constant
    if (!child->is_constant())
      return true;

    // all children must be float/int constants - get their val or bail if they're wrong
    switch (child->get_constant_value()->get_type()->get_itype()) {
      case LST_FLOATINGPOINT:
        v[cv++] = ((LLScriptFloatConstant *) child->get_constant_value())->get_value();
        break;
      case LST_INTEGER:
        v[cv++] = (F32) ((LLScriptIntegerConstant *) child->get_constant_value())->get_value();
        break;
      default:
        return true;
    }
  }

  // make sure we had enough children
  if (cv < 3)
    return true;

  // create constant value
  node->set_constant_value(gAllocationManager->new_tracked<LLScriptVectorConstant>(v[0], v[1], v[2]));
  return true;
}


bool ConstantDeterminingVisitor::visit(LLScriptQuaternionExpression *node) {
  float v[4];
  int cv = 0;

  for (LLASTNode *child = node->get_children(); child; child = child->get_next()) {
    // if we have too many children, make sure we don't overflow cv
    if (cv >= 4)
      return true;

    // all children must be constant
    if (!child->is_constant())
      return true;

    // all children must be float/int constants - get their val or bail if they're wrong
    switch (child->get_constant_value()->get_type()->get_itype()) {
      case LST_FLOATINGPOINT:
        v[cv++] = ((LLScriptFloatConstant *) child->get_constant_value())->get_value();
        break;
      case LST_INTEGER:
        v[cv++] = (F32) ((LLScriptIntegerConstant *) child->get_constant_value())->get_value();
        break;
      default:
        return true;
    }
  }

  // make sure we had enough children
  if (cv < 4)
    return true;

  // create constant value
  node->set_constant_value(gAllocationManager->new_tracked<LLScriptQuaternionConstant>(v[0], v[1], v[2], v[3]));
  return true;
}

bool ConstantDeterminingVisitor::visit(LLScriptTypecastExpression *node) {
  // what are we casting to
  auto *val = node->get_child(0)->get_constant_value();
  node->set_constant_value(nullptr);
  if (!val) {
    return true;
  }
  auto orig_type = val->get_type()->get_itype();
  auto to_type = node->get_type()->get_itype();

  if (orig_type == to_type) {
    // no-op case
    node->set_constant_value(val);
    return true;
  }

  LLScriptConstant *constant_value = nullptr;
  switch (orig_type) {
    case LST_KEY:
    case LST_STRING:
      constant_value = ((LLScriptStringConstant *) val)->cast(to_type);
      break;
    case LST_INTEGER:
      constant_value = ((LLScriptIntegerConstant *) val)->cast(to_type);
      break;
    case LST_FLOATINGPOINT:
      constant_value = ((LLScriptFloatConstant *) val)->cast(to_type);
      break;
    case LST_LIST:
      constant_value = ((LLScriptListConstant *) val)->cast(to_type);
      break;
    case LST_VECTOR:
      constant_value = ((LLScriptVectorConstant *) val)->cast(to_type);
      break;
    case LST_QUATERNION:
      constant_value = ((LLScriptQuaternionConstant *) val)->cast(to_type);
      break;
    case LST_MAX:
    case LST_NULL:
    case LST_ERROR:
      break;
  }
  node->set_constant_value(constant_value);
  return true;
}


LLScriptConstant *LLScriptGlobalVariable::get_constant_value() {
  // It's not really constant if it gets mutated more than once, is it?
  // note that initialization during declaration doesn't count.
  auto *id = (LLScriptIdentifier *) get_child(0);
  if (id->get_symbol()->get_assignments() == 0) {
    return constant_value;
  }
  return nullptr;
}

LLScriptConstant *LLScriptDeclaration::get_constant_value() {
  auto *id = (LLScriptIdentifier *) get_child(0);
  if (id->get_symbol()->get_assignments() == 0) {
    return constant_value;
  }
  return nullptr;
}

LLScriptConstant *LLScriptExpression::get_constant_value() {
  // replacing `foo = "bar"` with `"bar"` == no
  if (!operation_mutates(operation)) {
    return constant_value;
  }
  return nullptr;
}


LLScriptConstant *LLScriptLValueExpression::get_constant_value() {
  if (is_foldable) {
    // We have to be careful about folding lists
    if (this->type == TYPE(LST_LIST)) {
      LLASTNode *top_foldable = this;
      LLASTNode *current_node = this;

      // Don't fold this in if it's a list expression at the foldable level
      while (current_node != nullptr && current_node->node_allows_folding()) {
        top_foldable = current_node;
        current_node = current_node->get_parent();
      }
      if (top_foldable->get_type() == TYPE(LST_LIST))
        return nullptr;
    }

    return constant_value;
  }
  return nullptr;
}
}
