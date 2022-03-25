#include "type_checking.hh"

namespace Sling {

bool TypeCheckVisitor::visit(LLASTNode *node) {
  if (!node->get_type())
    node->set_type(LLScriptType::get(LST_NULL));
  return true;
}

bool TypeCheckVisitor::visit(LLScriptGlobalVariable *node) {
  auto *id = (LLScriptIdentifier *) node->get_child(0);
  LLASTNode *rvalue = node->get_child(1);
  // already have our expected type from the declaration
  if (rvalue == nullptr || rvalue->get_node_type() == NODE_NULL)
    return true;
  if (!rvalue->get_type()->can_coerce(id->get_type())) {
    ERROR(IN(node), E_WRONG_TYPE_IN_ASSIGNMENT, id->get_type()->get_node_name(),
          id->get_name(), rvalue->get_type()->get_node_name());
  }
  return true;
}

bool TypeCheckVisitor::visit(LLScriptSimpleAssignable *node) {
  LLASTNode *s_val = node->get_child(0);
  LLScriptType *type;
  if (s_val == nullptr) {
    type = TYPE(LST_NULL);
  } else if (s_val->get_node_type() == NODE_IDENTIFIER) {
    auto *id = (LLScriptIdentifier *) s_val;
    id->resolve_symbol(SYM_VARIABLE);
    type = id->get_type();
  } else if (s_val->get_node_type() == NODE_CONSTANT) {
    type = s_val->get_type();
  } else {
    type = TYPE(LST_ERROR);
  }
  node->set_type(type);
  return true;
}

bool TypeCheckVisitor::visit(LLScriptStateStatement *node) {
  auto *id = (LLScriptIdentifier *) node->get_child(0);
  node->set_type(TYPE(LST_NULL));
  if (id != nullptr)
    id->resolve_symbol(SYM_STATE);

  // see if we're in a state or function, and if we're inside of an if
  bool is_in_if = false;

  for (LLASTNode *ancestor = node->get_parent(); ancestor; ancestor = ancestor->get_parent()) {
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
                !strcmp(((LLScriptIdentifier *) ancestor->get_child(0))->get_name(),
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

bool TypeCheckVisitor::visit(LLScriptJumpStatement *node) {
  auto *id = (LLScriptIdentifier *) node->get_child(0);
  // labels and jump statements are special in that they can jump forwards
  // in the script, so we resolve their symbols for the first time here
  // rather than during the general symbol resolution step.
  id->resolve_symbol(SYM_LABEL);
  node->set_type(id->get_type());
  return true;
}

bool TypeCheckVisitor::visit(LLScriptDeclaration *node) {
  auto *id = (LLScriptIdentifier *) node->get_child(0);
  LLASTNode *rvalue = node->get_child(1);
  if (rvalue == nullptr || rvalue->get_node_type() == NODE_NULL)
    return true;
  if (!rvalue->get_type()->can_coerce(id->get_type())) {
    ERROR(IN(node), E_WRONG_TYPE_IN_ASSIGNMENT, id->get_type()->get_node_name(),
          id->get_name(), rvalue->get_type()->get_node_name());
  }
  return true;
}

bool TypeCheckVisitor::visit(LLScriptReturnStatement *node) {
  LLASTNode *ancestor = node->get_parent();

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

static bool is_branch_empty(LLASTNode *node) {
  if (node == nullptr || node->get_node_type() == NODE_NULL)
    return true;
  if (node->get_node_type() != NODE_STATEMENT)
    return false;
  if(node->get_node_sub_type() == NODE_NO_SUB_TYPE || node->get_node_sub_type() == NODE_COMPOUND_STATEMENT)
    return node->get_children() == nullptr || node->get_children()->get_node_type() == NODE_NULL;
  return false;
}

bool TypeCheckVisitor::visit(LLScriptIfStatement *node) {
  node->set_type(TYPE(LST_NULL));
  // warn if main branch is an empty statement and secondary branch is null
  // or empty
  if (is_branch_empty(node->get_child(1)) && is_branch_empty(node->get_child(2))) {
    ERROR(IN(node->get_child(0)), W_EMPTY_IF);
    DEBUG(LOG_DEBUG_SPAM, nullptr, "TYPE=%d SUBTYPE=%d CHILDREN=%p n=%s\n",
          get_child(1)->get_node_type(), get_child(1)->get_node_sub_type(),
          get_child(1)->get_children(), get_child(1)->get_node_name());
    //    do_walk( this );
  }
  return true;
}

bool TypeCheckVisitor::visit(LLScriptForStatement *node) {
  node->set_type(TYPE(LST_NULL));
  if (is_branch_empty(node->get_child(3))) {
    ERROR(IN(node), W_EMPTY_LOOP);
  }
  return true;
}

bool TypeCheckVisitor::visit(LLScriptDoStatement *node) {
  node->set_type(TYPE(LST_NULL));
  if (is_branch_empty(node->get_child(0))) {
    ERROR(IN(node), W_EMPTY_LOOP);
  }
  return true;
}

bool TypeCheckVisitor::visit(LLScriptWhileStatement *node) {
  node->set_type(TYPE(LST_NULL));
  if (is_branch_empty(node->get_child(1))) {
    ERROR(IN(node), W_EMPTY_LOOP);
  }
  return true;
}


bool TypeCheckVisitor::visit(LLScriptExpression *node) {
  int operation = node->get_operation();
  LLScriptType *type;
  LLASTNode *left = node->get_child(0);
  LLASTNode *right = node->get_child(1);
  if (operation == 0 || operation == '(') {
    type = left->get_type();
  } else {
    type = left->get_type()->get_result_type(operation, right ? right->get_type() : nullptr);
    if (type == nullptr) {
      ERROR(
          IN(node),
          E_INVALID_OPERATOR,
          left->get_type()->get_node_name(),
          operation_str(operation),
          right ? right->get_type()->get_node_name() : ""
      );
      type = left->get_type();
    }
  }
  node->set_type(type);
  return true;
}

bool TypeCheckVisitor::visit(LLScriptListConstant *node) {
  node->set_type(TYPE(LST_LIST));

  LLASTNode *val_c = node->get_value();
  while (val_c != nullptr) {
    if (val_c->get_type() == TYPE(LST_LIST)) {
      ERROR(IN(node), E_LIST_IN_LIST);
    }
    val_c = val_c->get_next();
  }
  return true;
}

bool TypeCheckVisitor::visit(LLScriptListExpression *node) {
  node->set_type(TYPE(LST_LIST));

  LLASTNode *val_c = node->get_children();
  while (val_c != nullptr) {
    if (val_c->get_type() == TYPE(LST_LIST)) {
      ERROR(IN(node), E_LIST_IN_LIST);
    }
    val_c = val_c->get_next();
  }
  return true;
}

// check argument types and count for function calls
// and event handler definitions
static bool validate_func_arg_spec(
    LLScriptIdentifier *id,
    LLASTNode *node,
    LLScriptIdentifier *params
) {
  bool is_event_handler = node->get_node_type() == NODE_EVENT_HANDLER;

  LLScriptFunctionDec *function_decl;
  LLScriptIdentifier *declared_param_id;
  LLScriptIdentifier *passed_param_id;
  int param_num = 1;

  function_decl = id->get_symbol()->get_function_decl();
  declared_param_id = (LLScriptIdentifier *) function_decl->get_children();
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
    passed_param_id = (LLScriptIdentifier *) passed_param_id->get_next();
    declared_param_id = (LLScriptIdentifier *) declared_param_id->get_next();
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

bool TypeCheckVisitor::visit(LLScriptFunctionExpression *node) {
  auto *id = (LLScriptIdentifier *) node->get_child(0);
  id->resolve_symbol(SYM_FUNCTION);
  node->set_type(id->get_type());

  // can't check types if function is undeclared
  if (id->get_symbol() == nullptr)
    return true;

  validate_func_arg_spec(id, node, (LLScriptIdentifier *) node->get_child(1));
  return true;
}

bool TypeCheckVisitor::visit(LLScriptEventHandler *node) {
  auto *id = (LLScriptIdentifier *) node->get_child(0);
  node->set_type(TYPE(LST_NULL));
  // can't check arg spec if event handler isn't valid
  if (id->get_symbol() == nullptr)
    return true;

  validate_func_arg_spec(id, node, (LLScriptIdentifier *) node->get_child(1)->get_children());
  return true;
}

bool TypeCheckVisitor::visit(LLScriptLValueExpression *node) {
  auto *id = (LLScriptIdentifier *) node->get_child(0);
  id->resolve_symbol(SYM_VARIABLE);
  node->set_type(id->get_type());

  LLScriptSymbol *sym = id->get_symbol();
  // This refers to a local, walk back and see there's anything
  // between us and its declaration that'd make it unfoldable
  if (sym != nullptr && sym->get_sub_type() == SYM_LOCAL && sym->get_var_decl() != nullptr) {
    LLASTNode *local_decl = sym->get_var_decl();

    // walk up and find the statement at the top of this expression;
    LLASTNode *upper_node = node->get_parent();
    while (upper_node != nullptr && upper_node->get_node_type() != NODE_STATEMENT) {
      upper_node = upper_node->get_parent();
    }
    if (upper_node != nullptr) {
      auto *parent_stmt = (LLScriptStatement *) upper_node;
      auto *found_stmt = (LLScriptStatement *) parent_stmt->find_previous_in_scope(
          [local_decl](LLASTNode *to_check) {
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
      return true;
    }
  }
  node->set_is_foldable(true);
  return true;
}

bool TypeCheckVisitor::visit(LLScriptTypecastExpression *node) {
  auto *child = node->get_child(0);
  if(!child)
    return true;
  auto *from_type = child->get_type();
  auto *to_type = node->get_type();
  if(!is_cast_legal(from_type->get_itype(), to_type->get_itype())) {
    ERROR(IN(node), E_ILLEGAL_CAST, from_type->get_node_name(), to_type->get_node_name());
  }
  return true;
}

bool TypeCheckVisitor::visit(LLScriptVectorExpression *node) {
  node->set_type(TYPE(LST_VECTOR));
  LLASTNode *child = node->get_children();
  for (; child; child = child->get_next()) {
    if (!child->get_type()->can_coerce(TYPE(LST_FLOATINGPOINT))) {
      ERROR(IN(node), E_WRONG_TYPE_IN_MEMBER_ASSIGNMENT, "vector",
            child->get_type()->get_node_name());
      return true;
    }
  }
  return true;
}

bool TypeCheckVisitor::visit(LLScriptVectorConstant *node) {
  node->set_type(TYPE(LST_VECTOR));
  LLASTNode *child = node->get_children();
  for (; child; child = child->get_next()) {
    if (!child->get_type()->can_coerce(TYPE(LST_FLOATINGPOINT))) {
      ERROR(IN(node), E_WRONG_TYPE_IN_MEMBER_ASSIGNMENT, "vector",
            child->get_type()->get_node_name());
      return true;
    }
  }
  return true;
}

bool TypeCheckVisitor::visit(LLScriptQuaternionExpression *node) {
  node->set_type(TYPE(LST_QUATERNION));
  LLASTNode *child = node->get_children();
  for (; child; child = child->get_next()) {
    if (!child->get_type()->can_coerce(TYPE(LST_FLOATINGPOINT))) {
      ERROR(IN(node), E_WRONG_TYPE_IN_MEMBER_ASSIGNMENT, "quaternion",
            child->get_type()->get_node_name());
      return true;
    }
  }
  return true;
}

bool TypeCheckVisitor::visit(LLScriptQuaternionConstant *node) {
  node->set_type(TYPE(LST_QUATERNION));
  LLASTNode *child = node->get_children();
  for (; child; child = child->get_next()) {
    if (!child->get_type()->can_coerce(TYPE(LST_FLOATINGPOINT))) {
      ERROR(IN(node), E_WRONG_TYPE_IN_MEMBER_ASSIGNMENT, "quaternion",
            child->get_type()->get_node_name());
      return true;
    }
  }
  return true;
}


}
