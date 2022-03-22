#include <string>

#include "lslmini.hh"

void LLASTNode::propogate_values() {
  LLASTNode *node = get_children();
  if (node != nullptr) {
    /*
       while ( node->get_next() )
       node = node->get_next(); // start with last node

       while ( node )  {
       node->propogate_values();
       node = node->get_prev();
       }
       */
    while (node) {
      node->propogate_values();
      node = node->get_next();
    }
  }

  determine_value();
}

void LLScriptListConstant::propogate_values() {
  LLASTNode *node = get_children();
  while (node) {
    node->propogate_values();
    node = node->get_next();
  }

  node = get_value();
  while (node) {
    node->propogate_values();
    node = node->get_next();
  }
  determine_value();
}

void LLScriptScript::propogate_values() {
  // need to iterate over global vars FIRST since expressions in
  // global functions may make use of them.
  LLASTNode *node = get_children();
  while(node != nullptr) {
    // passed the end of the list of globals
    if (node->get_node_type() != NODE_GLOBAL_STORAGE)
      break;
    if (LLASTNode* child = node->get_child(0)) {
      if (child->get_node_type() == NODE_GLOBAL_VARIABLE)
        child->propogate_values();
    }
    node = node->get_next();
  }
  // continue calculating values as normal.
  LLASTNode::propogate_values();
}

void LLASTNode::determine_value() {
  // none
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

void LLScriptDeclaration::determine_value() {
  auto *id = (LLScriptIdentifier *) get_child(0);
  LLASTNode *node = get_child(1);
  if (node == nullptr || node->get_node_type() == NODE_NULL) return;
  DEBUG(LOG_DEBUG_SPAM, NULL, "set %s const to %p\n", id->get_name(), node->get_constant_value());
  id->get_symbol()->set_constant_value(node->get_constant_value());
}

void LLScriptExpression::determine_value() {
  DEBUG(LOG_DEBUG_SPAM, NULL, "expression.determine_value() op=%d cv=%s st=%d\n", operation, constant_value
                                                                                             ? constant_value->get_node_name()
                                                                                             : NULL, get_node_sub_type());
  // Can't really be avoided in cases where the tree has been modified since
  // the value was calculated, absent some way of finding out which constant values
  // depend on the dirtied expression.
  // if (constant_value != nullptr)
  //   return; // we already have a value

  // only check normal and lvalue expressions
  switch(get_node_sub_type()) {
    case NODE_NO_SUB_TYPE:
    case NODE_CONSTANT_EXPRESSION:
    case NODE_PARENTHESIS_EXPRESSION:
    case NODE_BINARY_EXPRESSION:
    case NODE_UNARY_EXPRESSION:
    case NODE_LVALUE_EXPRESSION:
      break;
    default:
      return;
  }

  if (operation == 0 || operation == '(')
    constant_value = get_child(0)->get_constant_value();
  else if (operation == '=')
    constant_value = get_child(1)->get_constant_value();
  else {
    LLScriptConstant *left = get_child(0)->get_constant_value();
    LLScriptConstant *right = get_child(1) ? get_child(1)->get_constant_value() : nullptr;

    // we need a constant value from the left, and if we have a right side, it MUST have a constant value too
    if (left && (get_child(1) == nullptr || right != nullptr))
      constant_value = left->operation(operation, right, get_lloc());
    else
      constant_value = nullptr;
  }
}

void LLScriptGlobalVariable::determine_value() {
  // if it's initialized, set its constant value
  if (get_child(1)->get_node_type() == NODE_SIMPLE_ASSIGNABLE) {
    auto *identifier = (LLScriptIdentifier*) get_child(0);
    identifier->get_symbol()->set_constant_value(get_child(1)->get_child(0)->get_constant_value());
  }
}


void LLScriptSimpleAssignable::determine_value() {
  if (get_child(0))
    constant_value = get_child(0)->get_constant_value();
}

void LLScriptVectorConstant::determine_value() {
  if (get_value() != nullptr)
    return;

  float v[3];
  int cv = 0;

  for (LLASTNode *node = get_children(); node; node = node->get_next()) {
    // if we have too many children, make sure we don't overflow cv
    if (cv >= 3)
      return;

    // all children must be constant
    if (!node->is_constant())
      return;

    // all children must be float/int constants - get their val or bail if they're wrong
    switch (node->get_constant_value()->get_type()->get_itype()) {
      case LST_FLOATINGPOINT:
        v[cv++] = ((LLScriptFloatConstant *) node->get_constant_value())->get_value();
        break;
      case LST_INTEGER:
        v[cv++] = (F32)((LLScriptIntegerConstant *) node->get_constant_value())->get_value();
        break;
      default:
        return;
    }

  }

  if (cv < 3)  // not enough children
    return;

  value = gAllocationManager->new_tracked<LLVector>(v[0], v[1], v[2]);

}

void LLScriptQuaternionConstant::determine_value() {
  if (get_value() != nullptr)
    return;

  float v[4];
  int cv = 0;

  for (LLASTNode *node = get_children(); node; node = node->get_next()) {
    // if we have too many children, make sure we don't overflow cv
    if (cv >= 4)
      return;

    // all children must be constant
    if (!node->is_constant())
      return;

    // all children must be float/int constants - get their val or bail if they're wrong
    switch (node->get_constant_value()->get_type()->get_itype()) {
      case LST_FLOATINGPOINT:
        v[cv++] = ((LLScriptFloatConstant *) node->get_constant_value())->get_value();
        break;
      case LST_INTEGER:
        v[cv++] = (F32)((LLScriptIntegerConstant *) node->get_constant_value())->get_value();
        break;
      default:
        return;
    }

  }

  if (cv < 4) // not enough children;
    return;

  value = gAllocationManager->new_tracked<LLQuaternion>(v[0], v[1], v[2], v[3]);

}


void LLScriptIdentifier::determine_value() {
  // can't determine value if we don't have a symbol
  if (symbol == nullptr)
    return;

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
}

void LLScriptListExpression::determine_value() {
  LLASTNode *node = get_children();
  LLScriptSimpleAssignable *assignable = nullptr;

  // if we have children
  if (node->get_node_type() != NODE_NULL) {
    // make sure they are all constant
    for (node = get_children(); node; node = node->get_next()) {
      if (!node->is_constant())
        return;
    }

    // create assignables for them
    for (node = get_children(); node; node = node->get_next()) {
      if (assignable == nullptr) {
        assignable = gAllocationManager->new_tracked<LLScriptSimpleAssignable>(node->get_constant_value());
      } else {
        assignable->add_next_sibling(gAllocationManager->new_tracked<LLScriptSimpleAssignable>(node->get_constant_value()));
      }
    }
  }

  // create constant value
  constant_value = gAllocationManager->new_tracked<LLScriptListConstant>(assignable);

}

void LLScriptVectorExpression::determine_value() {
  float v[3];
  int cv = 0;

  // don't need to figure out a value if we already have one
  if (constant_value)
    return;

  for (LLASTNode *node = get_children(); node; node = node->get_next()) {
    // if we have too many children, make sure we don't overflow cv
    if (cv >= 3)
      return;

    // all children must be constant
    if (!node->is_constant())
      return;

    // all children must be float/int constants - get their val or bail if they're wrong
    switch (node->get_constant_value()->get_type()->get_itype()) {
      case LST_FLOATINGPOINT:
        v[cv++] = ((LLScriptFloatConstant *) node->get_constant_value())->get_value();
        break;
      case LST_INTEGER:
        v[cv++] = (F32)((LLScriptIntegerConstant *) node->get_constant_value())->get_value();
        break;
      default:
        return;
    }
  }

  // make sure we had enough children
  if (cv < 3)
    return;

  // create constant value
  constant_value = gAllocationManager->new_tracked<LLScriptVectorConstant>(v[0], v[1], v[2]);

}

void LLScriptQuaternionExpression::determine_value() {
  float v[4];
  int cv = 0;

  if (constant_value)
    return;

  for (LLASTNode *node = get_children(); node; node = node->get_next()) {
    // if we have too many children, make sure we don't overflow cv
    if (cv >= 4)
      return;

    // all children must be constant
    if (!node->is_constant())
      return;

    // all children must be float/int constants - get their val or bail if they're wrong
    switch (node->get_constant_value()->get_type()->get_itype()) {
      case LST_FLOATINGPOINT:
        v[cv++] = ((LLScriptFloatConstant *) node->get_constant_value())->get_value();
        break;
      case LST_INTEGER:
        v[cv++] = (F32)((LLScriptIntegerConstant *) node->get_constant_value())->get_value();
        break;
      default:
        return;
    }
  }

  if (cv < 4)
    return;

  // create constant value
  constant_value = gAllocationManager->new_tracked<LLScriptQuaternionConstant>(v[0], v[1], v[2], v[3]);

}



void LLScriptTypecastExpression::determine_value() {
  // what are we casting to
  auto *val = get_child(0)->get_constant_value();
  if (!val) {
    constant_value = nullptr;
    return;
  }
  constant_value = nullptr;
  auto orig_type = val->get_type()->get_itype();
  auto to_type = get_type()->get_itype();

  if (orig_type == to_type) {
    constant_value = val;
    return;
  }

  switch(orig_type) {
    case LST_KEY:
    case LST_STRING:
      constant_value = ((LLScriptStringConstant *)val)->cast(to_type);
      return;
    case LST_INTEGER:
      constant_value = ((LLScriptIntegerConstant *)val)->cast(to_type);
      return;
    case LST_FLOATINGPOINT:
      constant_value = ((LLScriptFloatConstant *)val)->cast(to_type);
      return;
    case LST_LIST:
      constant_value = ((LLScriptListConstant *)val)->cast(to_type);
      return;
    case LST_VECTOR:
      constant_value = ((LLScriptVectorConstant *)val)->cast(to_type);
      return;
    case LST_QUATERNION:
      constant_value = ((LLScriptQuaternionConstant *)val)->cast(to_type);
      return;
    case LST_MAX:
    case LST_NULL:
    case LST_ERROR:
      return;
  }
}

LLScriptConstant* LLScriptStringConstant::cast(LST_TYPE to_type) {
  auto *cv = ((LLScriptStringConstant *)constant_value)->get_value();
  switch(to_type) {
    case LST_INTEGER: {
      int base = 10;
      // Need to explicitly determine what the base should be, we only support
      // base 10 and base16 and we don't want `011` to be treated as octal!
      // This check is safe because `cv` must be a null terminated string.
      if (cv[0] == '0' && (cv[1] == 'x' || cv[2] == 'X'))
        base = 16;
      return gAllocationManager->new_tracked<LLScriptIntegerConstant>((S32)strtoul(cv, nullptr, base));
    }
    case LST_FLOATINGPOINT: {
      return gAllocationManager->new_tracked<LLScriptFloatConstant>((F32)strtod(cv, nullptr));
    }
    default:
      return nullptr;
  }
}

LLScriptConstant* LLScriptIntegerConstant::cast(LST_TYPE to_type) {
  auto cv = ((LLScriptIntegerConstant *)constant_value)->get_value();
  switch(to_type) {
    case LST_STRING: {
      return gAllocationManager->new_tracked<LLScriptStringConstant>(gAllocationManager->copy_str(
          std::to_string(cv).c_str()
      ));
    }
    default:
      return nullptr;
  }
}

LLScriptConstant *LLScriptFloatConstant::cast(LST_TYPE to_type) {
  auto cv = ((LLScriptFloatConstant *)constant_value)->get_value();
  switch(to_type) {
    case LST_STRING: {
      return gAllocationManager->new_tracked<LLScriptStringConstant>(gAllocationManager->copy_str(
          // to_string correctly handles -inf and friends
          std::to_string(cv).c_str()
      ));
    }
    default:
      return nullptr;
  }
}
