#include "type_checking.hh"

namespace Tailslide {

bool TypeCheckVisitor::visit(LSLASTNode *node) {
  if (!node->getType())
    node->setType(LSLType::get(LST_NULL));
  return true;
}

// both LSLGlobalVariable and LSLDeclaration have the same child layout
// so they can have a common visitor.
void TypeCheckVisitor::handleDeclaration(LSLASTNode *node) {
  auto *id = (LSLIdentifier *) node->getChild(0);
  LSLASTNode *rvalue = node->getChild(1);
  // already have our expected type from the declaration
  if (rvalue == nullptr || rvalue->getNodeType() == NODE_NULL)
    return;
  // we already know there's something messed up going on with the types
  // in the rvalue, don't re-error
  if (rvalue->getType() == TYPE(LST_ERROR))
    return;
  if (!rvalue->getType()->canCoerce(id->getType())) {
    NODE_ERROR(node, E_WRONG_TYPE_IN_ASSIGNMENT, id->getType()->getNodeName(),
               id->getName(), rvalue->getType()->getNodeName());
  }
}

bool TypeCheckVisitor::visit(LSLGlobalVariable *node) {
  handleDeclaration(node);
  return true;
}

bool TypeCheckVisitor::visit(LSLDeclaration *node) {
  handleDeclaration(node);
  return true;
}


bool TypeCheckVisitor::visit(LSLStateStatement *node) {
  // TODO: probably makes more sense in the best practices visitor?
  //  not really related to type-checking.
  auto *id = (LSLIdentifier *) node->getChild(0);

  // see if we're in a state or function, and if we're inside of an if
  bool is_in_if = false;

  for (LSLASTNode *ancestor = node->getParent(); ancestor; ancestor = ancestor->getParent()) {
    switch (ancestor->getNodeType()) {
      case NODE_STATEMENT:
        if (ancestor->getNodeSubType() == NODE_IF_STATEMENT)
          is_in_if = true;
        break;
      case NODE_STATE:
        // we're in a state, see if it's the same one we're calling
        if (!strcmp(((LSLIdentifier *) ancestor->getChild(0))->getName(), id->getName())) {
          NODE_ERROR(node, W_CHANGE_TO_CURRENT_STATE);
        }
        // if we've hit a state there's no sense ascending further
        return true;
      case NODE_GLOBAL_FUNCTION:
        if (is_in_if) {
          // see what kind of function it is
          switch (ancestor->getChild(0)->getIType()) {
            case LST_LIST:
            case LST_STRING:
              NODE_ERROR(node, W_CHANGE_STATE_HACK_CORRUPT);
              break;
            default:
              NODE_ERROR(node, W_CHANGE_STATE_HACK);
              break;
          }
        } else {
          NODE_ERROR(node, E_CHANGE_STATE_IN_FUNCTION);
        }
        // if we've hit a function there's no sense ascending further
        return true;
      default:
        break;
    }
  }
  return true;
}


bool TypeCheckVisitor::visit(LSLReturnStatement *node) {
  LSLASTNode *ancestor = node->getParent();
  LSLASTNode *effective_parent = node->getParent();

  // crawl up until we find an event handler or global func
  while (ancestor->getNodeType() != NODE_EVENT_HANDLER &&
      ancestor->getNodeType() != NODE_GLOBAL_FUNCTION)
    ancestor = ancestor->getParent();
  // figure out if we're directly under an event handler or global func
  // or nested within a control statement
  while (effective_parent->getNodeSubType() == NODE_COMPOUND_STATEMENT)
    effective_parent = effective_parent->getParent();
  bool func_is_parent = effective_parent->getNodeType() == NODE_EVENT_HANDLER ||
                        effective_parent->getNodeType() == NODE_GLOBAL_FUNCTION;

  auto *ancestor_type = ancestor->getChild(0)->getType();
  auto *ret = node->getChild(0);

  // if an event handler
  if (ancestor->getNodeType() == NODE_EVENT_HANDLER) {
    // make sure we're not returning a value
    // returning the result of a void expression is only allowed if
    // the return is nested in some other statement.
    if (ret->getIType() != LST_NULL || (ret->getNodeType() != NODE_NULL && func_is_parent)) {
      NODE_ERROR(node, E_RETURN_VALUE_IN_EVENT_HANDLER);
    }
  } else {
    // otherwise it's a function
    // the return type of the function is stored in the identifier which is
    // the first child
    auto *ret_type = ret->getType();
    if (!ret_type->canCoerce(ancestor_type)
        || (ret->getIType() == LST_NULL && (ret->getNodeType() != NODE_NULL && func_is_parent))) {
      NODE_ERROR(node, E_BAD_RETURN_TYPE, ret_type->getNodeName(), ancestor_type->getNodeName());
    }
  }
  return true;
}

static bool is_branch_empty(LSLASTNode *node) {
  if (node == nullptr || node->getNodeType() == NODE_NULL)
    return true;
  if (node->getNodeType() != NODE_STATEMENT)
    return false;
  if(node->getNodeSubType() == NODE_NO_SUB_TYPE || node->getNodeSubType() == NODE_COMPOUND_STATEMENT)
    return node->getChildren() == nullptr || node->getChildren()->getNodeType() == NODE_NULL;
  return false;
}

bool TypeCheckVisitor::visit(LSLIfStatement *node) {
  node->setType(TYPE(LST_NULL));
  // warn if main branch is an empty statement and secondary branch is null
  // or empty
  if (is_branch_empty(node->getChild(1)) && is_branch_empty(node->getChild(2))) {
    NODE_ERROR(node->getChild(0), W_EMPTY_IF);
    DEBUG(LOG_DEBUG_SPAM, nullptr, "TYPE=%d SUBTYPE=%d CHILDREN=%p n=%s\n",
          node->getChild(1)->getNodeType(), node->getChild(1)->getNodeSubType(),
          node->getChild(1)->getChildren(), node->getChild(1)->getNodeName());
    //    do_walk( this );
  }
  return true;
}

bool TypeCheckVisitor::visit(LSLForStatement *node) {
  if (is_branch_empty(node->getChild(3))) {
    NODE_ERROR(node, W_EMPTY_LOOP);
  }
  return true;
}

bool TypeCheckVisitor::visit(LSLDoStatement *node) {
  if (is_branch_empty(node->getChild(0))) {
    NODE_ERROR(node, W_EMPTY_LOOP);
  }
  return true;
}

bool TypeCheckVisitor::visit(LSLWhileStatement *node) {
  if (is_branch_empty(node->getChild(1))) {
    NODE_ERROR(node, W_EMPTY_LOOP);
  }
  return true;
}


bool TypeCheckVisitor::visit(LSLExpression *node) {
  int operation = node->getOperation();
  LSLType *type;
  LSLASTNode *left = node->getChild(0);
  LSLASTNode *right = node->getChild(1);
  LSLType *l_type = left->getType();
  LSLType *r_type = right ? right->getType() : nullptr;
  if (operation == 0 || operation == '(') {
    type = l_type;
  } else if (l_type == TYPE(LST_ERROR) || r_type == TYPE(LST_ERROR)) {
    // If we have a type error on either side of the expression we can't
    // guess as to what the result of the expression is meant to be.
    type = TYPE(LST_ERROR);
  } else {
    type = l_type->getResultType(operation, r_type);
    if (type == nullptr) {
      NODE_ERROR(
          node,
          E_INVALID_OPERATOR,
          l_type->getNodeName(),
          operation_str(operation),
          r_type ? r_type->getNodeName() : ""
      );
      // We don't know what type this expression is supposed to result in,
      // either because this operation is unsupported.
      type = TYPE(LST_ERROR);
    } else if (l_type == TYPE(LST_INTEGER) && r_type == TYPE(LST_FLOATINGPOINT) && operation == MUL_ASSIGN) {
      // see note in `LSLType::getResultType` for details on this case.
      NODE_ERROR(node, W_INT_FLOAT_MUL_ASSIGN);
    }
  }
  node->setType(type);
  return true;
}

bool TypeCheckVisitor::visit(LSLListConstant *node) {
  LSLASTNode *val_c = node->getValue();
  while (val_c != nullptr) {
    if (val_c->getType() == TYPE(LST_LIST)) {
      NODE_ERROR(node, E_LIST_IN_LIST);
      val_c->setType(TYPE(LST_ERROR));
    }
    val_c = val_c->getNext();
  }
  return true;
}

bool TypeCheckVisitor::visit(LSLListExpression *node) {
  LSLASTNode *val_c = node->getChildren();
  while (val_c != nullptr) {
    auto child_type = val_c->getIType();
    if (child_type == LST_LIST) {
      NODE_ERROR(node, E_LIST_IN_LIST);
      val_c->setType(TYPE(LST_ERROR));
    } else if (child_type == LST_NULL) {
      // not technically illegal in LL's compiler, but doesn't ever make sense,
      // mono crashes on compile and LSO results in weird type confusion bugs
      // and eventual heap corruption.
      NODE_ERROR(node, E_NULL_IN_LIST);
      val_c->setType(TYPE(LST_ERROR));
    }
    val_c = val_c->getNext();
  }
  return true;
}

// check argument types and count for function calls
// and event handler definitions
static bool validate_func_arg_spec(
    LSLIdentifier *id,
    LSLASTNode *node,
    LSLIdentifier *params,
    LSLParamList *function_decl
) {
  bool is_event_handler = node->getNodeType() == NODE_EVENT_HANDLER;

  LSLIdentifier *declared_param_id;
  LSLIdentifier *passed_param_id;
  int param_num = 1;

  declared_param_id = (LSLIdentifier *) function_decl->getChildren();
  passed_param_id = params;

  while (declared_param_id != nullptr && passed_param_id != nullptr) {
    bool param_compatible;
    if (is_event_handler)
      param_compatible = passed_param_id->getType() == declared_param_id->getType();
    else
      param_compatible = passed_param_id->getType()->canCoerce(declared_param_id->getType());

    if (!param_compatible) {
      NODE_ERROR(node, is_event_handler ? E_ARGUMENT_WRONG_TYPE_EVENT : E_ARGUMENT_WRONG_TYPE,
                 passed_param_id->getType()->getNodeName(),
            param_num,
                 id->getName(),
                 declared_param_id->getType()->getNodeName(),
                 declared_param_id->getName()
      );
      return false;
    }
    passed_param_id = (LSLIdentifier *) passed_param_id->getNext();
    declared_param_id = (LSLIdentifier *) declared_param_id->getNext();
    ++param_num;
  }

  if (passed_param_id != nullptr) {
    NODE_ERROR(node, is_event_handler ? E_TOO_MANY_ARGUMENTS_EVENT : E_TOO_MANY_ARGUMENTS, id->getName());
    return false;
  } else if (declared_param_id != nullptr) {
    NODE_ERROR(node, is_event_handler ? E_TOO_FEW_ARGUMENTS_EVENT : E_TOO_FEW_ARGUMENTS, id->getName());
    return false;
  }
  return true;
}

bool TypeCheckVisitor::visit(LSLFunctionExpression *node) {
  auto *id = (LSLIdentifier *) node->getChild(0);
  node->setType(id->getType());

  // can't check types if function is undeclared
  if (id->getSymbol() == nullptr) {
    node->setType(TYPE(LST_ERROR));
    return true;
  }

  validate_func_arg_spec(id, node, (LSLIdentifier *) node->getChild(1), id->getSymbol()->getFunctionDecl());
  return true;
}

bool TypeCheckVisitor::visit(LSLEventHandler *node) {
  auto *id = (LSLIdentifier *) node->getChild(0);
  // can't check arg spec if event handler isn't valid
  if (id->getSymbol() == nullptr)
    return true;

  // get the expected event handler prototype from the builtins
  auto *function_decl = node->mContext->builtins->lookup(id->getName(), SYM_EVENT)->getFunctionDecl();
  validate_func_arg_spec(id, node, (LSLIdentifier *) node->getChild(1)->getChildren(), function_decl);
  return true;
}

bool TypeCheckVisitor::visit(LSLLValueExpression *node) {
  auto *id = (LSLIdentifier *) node->getChild(0);
  LSLASTNode *member_node = node->getChild(1);
  node->setType(id->getType());

  auto *symbol = id->getSymbol();
  if (!symbol) {
    node->setType(TYPE(LST_ERROR));
    return false;
  }

  auto symbol_type = symbol->getSymbolType();

  /// If we're requesting a member, like var.x or var.y
  if (member_node && member_node->getNodeType() == NODE_IDENTIFIER) {
    const char *name = id->getName();
    const char *member = ((LSLIdentifier *) member_node)->getName();

    if (member != nullptr) {
      // all members must be single letters
      if (member[1] != 0) {
        NODE_ERROR(node, E_INVALID_MEMBER, name, member);
        node->setType(TYPE(LST_ERROR));
        return false;
      }

      /// Make sure it's a variable
      if (symbol_type != SYM_VARIABLE) {
        NODE_ERROR(node, E_MEMBER_NOT_VARIABLE, name, member, LSLSymbol::getTypeName(symbol_type));
        node->setType(TYPE(LST_ERROR));
        return false;
      }

      // ZERO_VECTOR.x is not valid, because it's really `<0,0,0>.x` which is not allowed!
      if (symbol->getSubType() == SYM_BUILTIN) {
        NODE_ERROR(node, E_INVALID_MEMBER, name, member);
        node->setType(TYPE(LST_ERROR));
        return false;
      }

      // Make sure it's a vector or quaternion.
      switch (symbol->getIType()) {
        case LST_QUATERNION:
          if (member[0] == 's') {
            node->setType(TYPE(LST_FLOATINGPOINT));
            break;
          }
          // FALL THROUGH
        case LST_VECTOR:
          switch (member[0]) {
            case 'x':
            case 'y':
            case 'z':
              node->setType(TYPE(LST_FLOATINGPOINT));
              break;
            default:
              NODE_ERROR(node, E_INVALID_MEMBER, name, member);
              node->setType(TYPE(LST_ERROR));
              break;
          }
          break;
        default:
          NODE_ERROR(node, E_MEMBER_WRONG_TYPE, name, member);
          node->setType(TYPE(LST_ERROR));
          break;
      }
    }
  }

  // This refers to a local, walk back and see there's anything
  // between us and its declaration that'd make it unfoldable
  if (symbol->getSubType() == SYM_LOCAL && symbol->getVarDecl() != nullptr) {
    LSLASTNode *local_decl = symbol->getVarDecl();

    // walk up and find the statement at the top of this expression;
    LSLASTNode *upper_node = node->getParent();
    while (upper_node != nullptr && upper_node->getNodeType() != NODE_STATEMENT) {
      upper_node = upper_node->getParent();
    }
    if (upper_node != nullptr) {
      auto *parent_stmt = (LSLStatement *) upper_node;
      auto *found_stmt = (LSLStatement *) parent_stmt->findPreviousInScope(
          [local_decl](LSLASTNode *to_check) {
            // stop searching once we hit the declaration or a label
            return to_check == local_decl || to_check->getNodeSubType() == NODE_LABEL;
          }
      );
      // the declaration really should be above us somewhere
      // unless they did something screwy like `if(r)string r = baz;`

      // hit a label before the declaration? Not gonna inline it.
      // we could be smarter about this, but it's probably not worth it.
      // we could check if there was also a jump to that label somewhere
      // before us in the same scope (before the declaration obviously.)
      node->setIsFoldable(found_stmt && (found_stmt->getNodeSubType() != NODE_LABEL));
      return false;
    }
  }
  node->setIsFoldable(true);
  return false;
}

bool TypeCheckVisitor::visit(LSLTypecastExpression *node) {
  auto *child = node->getChild(0);
  if(!child)
    return true;
  auto *from_type = child->getType();
  auto *to_type = node->getType();
  if(!is_cast_legal(from_type->getIType(), to_type->getIType())) {
    // this is just an error bubbling up
    if (from_type->getIType() != LST_ERROR) {
      NODE_ERROR(node, E_ILLEGAL_CAST, from_type->getNodeName(), to_type->getNodeName());
    }
  }
  return true;
}

bool TypeCheckVisitor::visit(LSLVectorExpression *node) {
  LSLASTNode *child = node->getChildren();
  for (; child; child = child->getNext()) {
    if (!child->getType()->canCoerce(TYPE(LST_FLOATINGPOINT))) {
      NODE_ERROR(node, E_WRONG_TYPE_IN_MEMBER_ASSIGNMENT, "vector",
                 child->getType()->getNodeName());
      return true;
    }
  }
  return true;
}

bool TypeCheckVisitor::visit(LSLQuaternionExpression *node) {
  LSLASTNode *child = node->getChildren();
  for (; child; child = child->getNext()) {
    if (!child->getType()->canCoerce(TYPE(LST_FLOATINGPOINT))) {
      NODE_ERROR(node, E_WRONG_TYPE_IN_MEMBER_ASSIGNMENT, "quaternion",
                 child->getType()->getNodeName());
      return true;
    }
  }
  return true;
}

}
