#include "type_checking.hh"

namespace Tailslide {

bool TypeCheckVisitor::visit(LSLASTNode *node) {
  if (!node->get_type())
    node->set_type(LSLType::get(LST_NULL));
  return true;
}

bool TypeCheckVisitor::visit(LSLGlobalVariable *node) {
  auto *id = (LSLIdentifier *) node->get_child(0);
  LSLASTNode *rvalue = node->get_child(1);
  // already have our expected type from the declaration
  if (rvalue == nullptr || rvalue->get_node_type() == NODE_NULL)
    return true;
  // we already know there's something messed up going on with the types
  // in the rvalue, don't re-error
  if (rvalue->get_type() == TYPE(LST_ERROR))
    return true;
  if (!rvalue->get_type()->can_coerce(id->get_type())) {
    ERROR(IN(node), E_WRONG_TYPE_IN_ASSIGNMENT, id->get_type()->get_node_name(),
          id->get_name(), rvalue->get_type()->get_node_name());
  }
  return true;
}


bool TypeCheckVisitor::visit(LSLStateStatement *node) {
  auto *id = (LSLIdentifier *) node->get_child(0);
  node->set_type(TYPE(LST_NULL));

  // see if we're in a state or function, and if we're inside of an if
  bool is_in_if = false;

  for (LSLASTNode *ancestor = node->get_parent(); ancestor; ancestor = ancestor->get_parent()) {
    switch (ancestor->get_node_type()) {
      case NODE_STATEMENT:
        if (ancestor->get_node_sub_type() == NODE_IF_STATEMENT)
          is_in_if = true;
        break;
      case NODE_STATE:
        // we're in a state, see if it's the same one we're calling
        if (
          // in default and calling state default
            (ancestor->get_child(0)->get_node_type() == NODE_NULL && id == nullptr) ||
            (
                // make sure neither current nor target is default
                (id != nullptr && ancestor->get_child(0)->get_node_type() ==
                                  NODE_IDENTIFIER) &&
                // in state x calling state x
                !strcmp(((LSLIdentifier *) ancestor->get_child(0))->get_name(),
                        id->get_name())
            )
            ) {
          ERROR(IN(node), W_CHANGE_TO_CURRENT_STATE);
        }
        return true;
      case NODE_GLOBAL_FUNCTION:
        if (is_in_if) {
          // see what kind of function it is
          switch (ancestor->get_child(0)->get_type()->get_itype()) {
            case LST_LIST:
            case LST_STRING:
              ERROR(IN(node), W_CHANGE_STATE_HACK_CORRUPT);
              break;
            default:
              ERROR(IN(node), W_CHANGE_STATE_HACK);
              break;
          }
        } else {
          ERROR(IN(node), E_CHANGE_STATE_IN_FUNCTION);
        }
        return true;
      default:
        break;
    }
  }
  LOG(LOG_ERROR, IN(node), "INTERNAL ERROR: encountered state change statement "
                           "not in function or state!");
  return true;
}

bool TypeCheckVisitor::visit(LSLDeclaration *node) {
  auto *id = (LSLIdentifier *) node->get_child(0);
  LSLASTNode *rvalue = node->get_child(1);
  if (rvalue == nullptr || rvalue->get_node_type() == NODE_NULL)
    return true;
  // we already know there's something messed up going on with the types
  // in the rvalue, don't re-error
  if (rvalue->get_type() == TYPE(LST_ERROR))
    return true;
  if (!rvalue->get_type()->can_coerce(id->get_type())) {
    ERROR(IN(node), E_WRONG_TYPE_IN_ASSIGNMENT, id->get_type()->get_node_name(),
          id->get_name(), rvalue->get_type()->get_node_name());
  }
  return true;
}

bool TypeCheckVisitor::visit(LSLReturnStatement *node) {
  LSLASTNode *ancestor = node->get_parent();

  // crawl up until we find an event handler or global func
  while (ancestor->get_node_type() != NODE_EVENT_HANDLER &&
         ancestor->get_node_type() != NODE_GLOBAL_FUNCTION)
    ancestor = ancestor->get_parent();

  // if an event handler
  if (ancestor->get_node_type() == NODE_EVENT_HANDLER) {
    // make sure we're not returning anything
    if (node->get_child(0)->get_node_type() != NODE_NULL) {
      ERROR(IN(node), E_RETURN_VALUE_IN_EVENT_HANDLER);
    }
  } else {  // otherwise it's a function
    // the return type of the function is stored in the identifier which is
    // the first child
    if (!node->get_child(0)->get_type()->can_coerce(
        ancestor->get_child(0)->get_type())) {
      ERROR(IN(node), E_BAD_RETURN_TYPE,
            node->get_child(0)->get_type()->get_node_name(),
            ancestor->get_child(0)->get_type()->get_node_name());
    }
  }
  return true;
}

static bool is_branch_empty(LSLASTNode *node) {
  if (node == nullptr || node->get_node_type() == NODE_NULL)
    return true;
  if (node->get_node_type() != NODE_STATEMENT)
    return false;
  if(node->get_node_sub_type() == NODE_NO_SUB_TYPE || node->get_node_sub_type() == NODE_COMPOUND_STATEMENT)
    return node->get_children() == nullptr || node->get_children()->get_node_type() == NODE_NULL;
  return false;
}

bool TypeCheckVisitor::visit(LSLIfStatement *node) {
  node->set_type(TYPE(LST_NULL));
  // warn if main branch is an empty statement and secondary branch is null
  // or empty
  if (is_branch_empty(node->get_child(1)) && is_branch_empty(node->get_child(2))) {
    ERROR(IN(node->get_child(0)), W_EMPTY_IF);
    DEBUG(LOG_DEBUG_SPAM, nullptr, "TYPE=%d SUBTYPE=%d CHILDREN=%p n=%s\n",
          node->get_child(1)->get_node_type(), node->get_child(1)->get_node_sub_type(),
          node->get_child(1)->get_children(), node->get_child(1)->get_node_name());
    //    do_walk( this );
  }
  return true;
}

bool TypeCheckVisitor::visit(LSLForStatement *node) {
  node->set_type(TYPE(LST_NULL));
  if (is_branch_empty(node->get_child(3))) {
    ERROR(IN(node), W_EMPTY_LOOP);
  }
  return true;
}

bool TypeCheckVisitor::visit(LSLDoStatement *node) {
  node->set_type(TYPE(LST_NULL));
  if (is_branch_empty(node->get_child(0))) {
    ERROR(IN(node), W_EMPTY_LOOP);
  }
  return true;
}

bool TypeCheckVisitor::visit(LSLWhileStatement *node) {
  node->set_type(TYPE(LST_NULL));
  if (is_branch_empty(node->get_child(1))) {
    ERROR(IN(node), W_EMPTY_LOOP);
  }
  return true;
}


bool TypeCheckVisitor::visit(LSLExpression *node) {
  int operation = node->get_operation();
  LSLType *type;
  LSLASTNode *left = node->get_child(0);
  LSLASTNode *right = node->get_child(1);
  LSLType *l_type = left->get_type();
  LSLType *r_type = right ? right->get_type() : nullptr;
  if (operation == 0 || operation == '(') {
    type = l_type;
  } else if (l_type == TYPE(LST_ERROR) || r_type == TYPE(LST_ERROR)) {
    // If we have a type error on either side of the expression we can't
    // guess as to what the result of the expression is meant to be.
    type = TYPE(LST_ERROR);
  } else {
    type = l_type->get_result_type(operation, r_type);
    if (type == nullptr) {
      ERROR(
          IN(node),
          E_INVALID_OPERATOR,
          l_type->get_node_name(),
          operation_str(operation),
          r_type ? r_type->get_node_name() : ""
      );
      // We don't know what type this expression is supposed to result in,
      // either because this operation is unsupported.
      type = TYPE(LST_ERROR);
    }
  }
  node->set_type(type);
  return true;
}

bool TypeCheckVisitor::visit(LSLListConstant *node) {
  node->set_type(TYPE(LST_LIST));

  LSLASTNode *val_c = node->get_value();
  while (val_c != nullptr) {
    if (val_c->get_type() == TYPE(LST_LIST)) {
      ERROR(IN(node), E_LIST_IN_LIST);
      val_c->set_type(TYPE(LST_ERROR));
    }
    val_c = val_c->get_next();
  }
  return true;
}

bool TypeCheckVisitor::visit(LSLListExpression *node) {
  node->set_type(TYPE(LST_LIST));

  LSLASTNode *val_c = node->get_children();
  while (val_c != nullptr) {
    if (val_c->get_type() == TYPE(LST_LIST)) {
      ERROR(IN(node), E_LIST_IN_LIST);
      val_c->set_type(TYPE(LST_ERROR));
    }
    val_c = val_c->get_next();
  }
  return true;
}

// check argument types and count for function calls
// and event handler definitions
static bool validate_func_arg_spec(
    LSLIdentifier *id,
    LSLASTNode *node,
    LSLIdentifier *params
) {
  bool is_event_handler = node->get_node_type() == NODE_EVENT_HANDLER;

  LSLFunctionDec *function_decl;
  LSLIdentifier *declared_param_id;
  LSLIdentifier *passed_param_id;
  int param_num = 1;

  function_decl = id->get_symbol()->get_function_decl();
  declared_param_id = (LSLIdentifier *) function_decl->get_children();
  passed_param_id = params;

  while (declared_param_id != nullptr && passed_param_id != nullptr) {
    bool param_compatible;
    if (is_event_handler)
      param_compatible = passed_param_id->get_type() == declared_param_id->get_type();
    else
      param_compatible = passed_param_id->get_type()->can_coerce(declared_param_id->get_type());

    if (!param_compatible) {
      ERROR(IN(node), is_event_handler ? E_ARGUMENT_WRONG_TYPE_EVENT : E_ARGUMENT_WRONG_TYPE,
            passed_param_id->get_type()->get_node_name(),
            param_num,
            id->get_name(),
            declared_param_id->get_type()->get_node_name(),
            declared_param_id->get_name()
      );
      return false;
    }
    passed_param_id = (LSLIdentifier *) passed_param_id->get_next();
    declared_param_id = (LSLIdentifier *) declared_param_id->get_next();
    ++param_num;
  }

  if (passed_param_id != nullptr) {
    ERROR(IN(node), is_event_handler ? E_TOO_MANY_ARGUMENTS_EVENT : E_TOO_MANY_ARGUMENTS, id->get_name());
    return false;
  } else if (declared_param_id != nullptr) {
    ERROR(IN(node), is_event_handler ? E_TOO_FEW_ARGUMENTS_EVENT : E_TOO_FEW_ARGUMENTS, id->get_name());
    return false;
  }
  return true;
}

bool TypeCheckVisitor::visit(LSLFunctionExpression *node) {
  auto *id = (LSLIdentifier *) node->get_child(0);
  node->set_type(id->get_type());

  // can't check types if function is undeclared
  if (id->get_symbol() == nullptr) {
    node->set_type(TYPE(LST_ERROR));
    return true;
  }

  validate_func_arg_spec(id, node, (LSLIdentifier *) node->get_child(1));
  return true;
}

bool TypeCheckVisitor::visit(LSLEventHandler *node) {
  auto *id = (LSLIdentifier *) node->get_child(0);
  node->set_type(TYPE(LST_NULL));
  // can't check arg spec if event handler isn't valid
  if (id->get_symbol() == nullptr)
    return true;

  validate_func_arg_spec(id, node, (LSLIdentifier *) node->get_child(1)->get_children());
  return true;
}

bool TypeCheckVisitor::visit(LSLLValueExpression *node) {
  auto *id = (LSLIdentifier *) node->get_child(0);
  LSLASTNode *member_node = node->get_child(1);
  node->set_type(id->get_type());

  auto *symbol = id->get_symbol();
  if (!symbol) {
    node->set_type(TYPE(LST_ERROR));
    return false;
  }

  auto symbol_type = symbol->get_symbol_type();

  /// If we're requesting a member, like var.x or var.y
  if (member_node && member_node->get_node_type() == NODE_IDENTIFIER) {
    const char *name = id->get_name();
    const char *member = ((LSLIdentifier*)member_node)->get_name();

    if (member != nullptr) {
      // all members must be single letters
      if (member[1] != 0) {
        ERROR(IN(node), E_INVALID_MEMBER, name, member);
        node->set_type(TYPE(LST_ERROR));
        return false;
      }

      /// Make sure it's a variable
      if (symbol_type != SYM_VARIABLE) {
        ERROR(IN(node), E_MEMBER_NOT_VARIABLE, name, member, LSLSymbol::get_type_name(symbol_type));
        node->set_type(TYPE(LST_ERROR));
        return false;
      }

      // ZERO_VECTOR.x is not valid, because it's really `<0,0,0>.x` which is not allowed!
      if (symbol->get_sub_type() == SYM_BUILTIN) {
        ERROR(IN(node), E_INVALID_MEMBER, name, member);
        node->set_type(TYPE(LST_ERROR));
        return false;
      }

      // Make sure it's a vector or quaternion.
      switch (symbol->get_type()->get_itype()) {
        case LST_QUATERNION:
          if (member[0] == 's') {
            node->set_type(TYPE(LST_FLOATINGPOINT));
            break;
          }
          // FALL THROUGH
        case LST_VECTOR:
          switch (member[0]) {
            case 'x':
            case 'y':
            case 'z':
              node->set_type(TYPE(LST_FLOATINGPOINT));
              break;
            default:
              ERROR(IN(node), E_INVALID_MEMBER, name, member);
              node->set_type(TYPE(LST_ERROR));
              break;
          }
          break;
        default:
          ERROR(IN(node), E_MEMBER_WRONG_TYPE, name, member);
          node->set_type(TYPE(LST_ERROR));
          break;
      }
    }
  }

  // This refers to a local, walk back and see there's anything
  // between us and its declaration that'd make it unfoldable
  if (symbol->get_sub_type() == SYM_LOCAL && symbol->get_var_decl() != nullptr) {
    LSLASTNode *local_decl = symbol->get_var_decl();

    // walk up and find the statement at the top of this expression;
    LSLASTNode *upper_node = node->get_parent();
    while (upper_node != nullptr && upper_node->get_node_type() != NODE_STATEMENT) {
      upper_node = upper_node->get_parent();
    }
    if (upper_node != nullptr) {
      auto *parent_stmt = (LSLStatement *) upper_node;
      auto *found_stmt = (LSLStatement *) parent_stmt->find_previous_in_scope(
          [local_decl](LSLASTNode *to_check) {
            // stop searching once we hit the declaration or a label
            return to_check == local_decl || to_check->get_node_sub_type() == NODE_LABEL;
          }
      );
      // the declaration really should be above us somewhere
      // unless they did something screwy like `if(r)string r = baz;`

      // hit a label before the declaration? Not gonna inline it.
      // we could be smarter about this, but it's probably not worth it.
      // we could check if there was also a jump to that label somewhere
      // before us in the same scope (before the declaration obviously.)
      node->set_is_foldable(found_stmt && (found_stmt->get_node_sub_type() != NODE_LABEL));
      return false;
    }
  }
  node->set_is_foldable(true);
  return false;
}

bool TypeCheckVisitor::visit(LSLTypecastExpression *node) {
  auto *child = node->get_child(0);
  if(!child)
    return true;
  auto *from_type = child->get_type();
  auto *to_type = node->get_type();
  if(!is_cast_legal(from_type->get_itype(), to_type->get_itype())) {
    // this is just an error bubbling up
    if (from_type->get_type() != TYPE(LST_ERROR)) {
      ERROR(IN(node), E_ILLEGAL_CAST, from_type->get_node_name(), to_type->get_node_name());
    }
  }
  return true;
}

bool TypeCheckVisitor::visit(LSLVectorExpression *node) {
  node->set_type(TYPE(LST_VECTOR));
  LSLASTNode *child = node->get_children();
  for (; child; child = child->get_next()) {
    if (!child->get_type()->can_coerce(TYPE(LST_FLOATINGPOINT))) {
      ERROR(IN(node), E_WRONG_TYPE_IN_MEMBER_ASSIGNMENT, "vector",
            child->get_type()->get_node_name());
      return true;
    }
  }
  return true;
}

bool TypeCheckVisitor::visit(LSLVectorConstant *node) {
  node->set_type(TYPE(LST_VECTOR));
  return true;
}

bool TypeCheckVisitor::visit(LSLQuaternionExpression *node) {
  node->set_type(TYPE(LST_QUATERNION));
  LSLASTNode *child = node->get_children();
  for (; child; child = child->get_next()) {
    if (!child->get_type()->can_coerce(TYPE(LST_FLOATINGPOINT))) {
      ERROR(IN(node), E_WRONG_TYPE_IN_MEMBER_ASSIGNMENT, "quaternion",
            child->get_type()->get_node_name());
      return true;
    }
  }
  return true;
}

bool TypeCheckVisitor::visit(LSLQuaternionConstant *node) {
  node->set_type(TYPE(LST_QUATERNION));
  return true;
}


}
