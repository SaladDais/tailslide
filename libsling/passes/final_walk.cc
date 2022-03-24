#include "final_walk.hh"

namespace Sling {


static bool allret(LLASTNode *p) {
  bool ret = false;
  if (p->get_node_type() == NODE_STATEMENT && p->get_node_sub_type() == NODE_RETURN_STATEMENT) {
    // TODO check next value here for unreachable code
    return true;
  } else if (p->get_node_type() == NODE_STATEMENT && p->get_node_sub_type() == NODE_IF_STATEMENT) {
    bool true_branch = p->get_child(1) && allret(p->get_child(1));
    bool false_branch = p->get_child(2) && allret(p->get_child(2));

    return (true_branch && false_branch);
  } else if (p->get_node_type() == NODE_STATEMENT && p->get_node_sub_type() == NODE_COMPOUND_STATEMENT) {
    for (LLASTNode *q = p->get_children(); q; q = q->get_next()) {
      ret |= allret(q);
    }
  } else {
#if 0
    if (p->get_next()) {
       ret |= allret(p->get_next());
    }
    if (p->get_children()) {
       ret |= allret(p->get_children());
    }
#endif
  }
  return ret;
}

bool FinalCheckVisitor::visit(LLScriptGlobalFunction* node) {
  auto *id = (LLScriptIdentifier *) node->get_child(0);
  //LLScriptFunctionDec *decl = (LLScriptFunctionDec *) get_child(1);
  auto *statement = (LLScriptStatement *) node->get_child(2);

  if (id->get_symbol() == nullptr) {
    id->resolve_symbol(SYM_FUNCTION);
  }

  if (id->get_symbol() != nullptr) {
    LLScriptType *tipe = id->get_symbol()->get_type();

    if (tipe->get_itype() != LST_NULL && !allret(statement)) {
      ERROR(IN(node->get_child(0)), E_NOT_ALL_PATHS_RETURN);
    }
  }
  return true;
};

bool FinalCheckVisitor::visit(LLScriptIfStatement *node) {
  // see if expression is constant
  LLASTNode *cond = node->get_child(0);
  if (cond->get_constant_value() != nullptr) {
    // TODO: can conditions be something other than integer?
    // ^ Yep, `key`s for one, and probably a bunch of others.
    // `if (key_foo) {}` is the fastest way to validate UUIDs in LSL.
    if (cond->get_constant_value()->get_node_sub_type() == NODE_INTEGER_CONSTANT) {
      if (((LLScriptIntegerConstant *) cond->get_constant_value())->get_value()) {
        ERROR(IN(cond), W_CONDITION_ALWAYS_TRUE);
      } else {
        ERROR(IN(cond), W_CONDITION_ALWAYS_FALSE);
      }
    }
  }

  // set if expression is an assignment
  if (cond->get_node_type() == NODE_EXPRESSION) {
    auto *expr = (LLScriptExpression *) cond;
    if (expr->get_operation() == '=') {
      ERROR(IN(expr), W_ASSIGNMENT_IN_COMPARISON);
    }
  }
  return true;
}

bool FinalCheckVisitor::visit(LLScriptEventHandler *node) {
  LLASTNode *upper_node;
  int found = 0;
  auto *id = (LLScriptIdentifier *) node->get_child(0);

  // check for duplicates
  for (upper_node = node->get_parent()->get_children(); upper_node; upper_node = upper_node->get_next()) {
    if (upper_node->get_node_type() != NODE_EVENT_HANDLER)
      continue;
    auto *other_id = (LLScriptIdentifier *) upper_node->get_child(0);
    if (!strcmp(id->get_name(), other_id->get_name())) {
      found++;
    }
  }
  if (found > 1) {
    ERROR(IN(node), E_MULTIPLE_EVENT_HANDLERS, id->get_name());
  }

  // check parameters
  if (id->get_symbol() == nullptr) {
    id->resolve_symbol(SYM_EVENT);
  }

  if (id->get_symbol() != nullptr) {
    // check argument types
    LLScriptFunctionDec *function_decl;
    LLScriptIdentifier *declared_param_id;
    LLScriptIdentifier *passed_param_id;
    int param_num = 1;

    function_decl = id->get_symbol()->get_function_decl();
    declared_param_id = (LLScriptIdentifier *) function_decl->get_children();
    passed_param_id = (LLScriptIdentifier *) node->get_child(1)->get_children();

    while (declared_param_id != nullptr && passed_param_id != nullptr) {
      if (!passed_param_id->get_type()->can_coerce(
          declared_param_id->get_type())) {
        ERROR(IN(node), E_ARGUMENT_WRONG_TYPE_EVENT,
              passed_param_id->get_type()->get_node_name(),
              param_num,
              id->get_name(),
              declared_param_id->get_type()->get_node_name(),
              declared_param_id->get_name()
        );
        return true;
      }
      passed_param_id = (LLScriptIdentifier *) passed_param_id->get_next();
      declared_param_id = (LLScriptIdentifier *) declared_param_id->get_next();
      ++param_num;
    }

    if (passed_param_id != nullptr) {
      // printf("too many, extra is %s\n", passed_param_id->get_name());
      ERROR(IN(node), E_TOO_MANY_ARGUMENTS_EVENT, id->get_name());
    } else if (declared_param_id != nullptr) {
      // printf("too few, extra is %s\n", declared_param_id->get_name());
      ERROR(IN(node), E_TOO_FEW_ARGUMENTS_EVENT, id->get_name());
    }
  } else {
    ERROR(IN(node), E_INVALID_EVENT, id->get_name());
  }
  return true;
}

}
