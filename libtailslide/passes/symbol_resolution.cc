#include "symbol_resolution.hh"

namespace Tailslide {


bool SymbolResolutionVisitor::visit(LSLScript *script) {
  replaceSymbolTable(script);
  auto *globals = script->getGlobals();
  // all global var definitions are implicitly hoisted above function definitions
  // all functions and states have their declarations implicitly hoisted as well.
  // walk over just the global vars and prototypes of functions.
  for (auto *global : *globals) {
    if (global->getNodeType() == NODE_GLOBAL_VARIABLE) {
      global->visit(this);
    } else if (global->getNodeType() == NODE_GLOBAL_FUNCTION) {
      // just record the prototype of the function and don't descend for now.
      auto *global_func = (LSLGlobalFunction *)global;
      replaceSymbolTable(global_func);
      auto *identifier = global_func->getIdentifier();

      // define function in script scope since functions have their own scope
      identifier->setSymbol(_mAllocator->newTracked<LSLSymbol>(
          identifier->getName(),
          identifier->getType(),
          SYM_FUNCTION,
          SYM_GLOBAL,
          global_func->getLoc(),
          global_func->getArguments()
      ));
      script->defineSymbol(identifier->getSymbol());
    }
  }

  // now walk the states to register their prototypes
  auto *states = script->getStates();
  for (auto *state : *states) {
    replaceSymbolTable(state);
    auto *identifier = ((LSLState *)state)->getIdentifier();
    identifier->setSymbol(_mAllocator->newTracked<LSLSymbol>(
        identifier->getName(), identifier->getType(), SYM_STATE, SYM_GLOBAL, identifier->getLoc()
    ));
    state->getParent()->defineSymbol(identifier->getSymbol());
  }

  // visit function bodies
  for (auto *global : *globals) {
    // we've already descended into global vars, don't re-visit.
    if (global->getNodeType() == NODE_GLOBAL_VARIABLE)
      continue;
    global->visit(this);
  }
  // then state bodies
  states->visit(this);
  return false;
}

bool SymbolResolutionVisitor::visit(LSLGlobalVariable *glob_var) {
  // descend first so we can resolve any symbol references present in the rvalue
  // before we've defined the identifier from the lvalue.
  // Necessary so things like `string foo = foo;` will error correctly.
  if (auto *initializer = glob_var->getInitializer())
    initializer->visit(this);

  auto *identifier = glob_var->getIdentifier();
  identifier->setSymbol(_mAllocator->newTracked<LSLSymbol>(
      identifier->getName(), identifier->getType(), SYM_VARIABLE, SYM_GLOBAL, glob_var->getLoc(), nullptr, glob_var));
  glob_var->defineSymbol(identifier->getSymbol());
  return false;
}

bool SymbolResolutionVisitor::visit(LSLDeclaration *decl_stmt) {
  // visit the rvalue first so we correctly handle things like
  // `string foo = foo;`
  if (auto *initializer = decl_stmt->getInitializer())
    initializer->visit(this);

  auto *identifier = decl_stmt->getIdentifier();
  identifier->setSymbol(_mAllocator->newTracked<LSLSymbol>(
      identifier->getName(), identifier->getType(), SYM_VARIABLE, SYM_LOCAL, decl_stmt->getLoc(), nullptr, decl_stmt));
  decl_stmt->defineSymbol(identifier->getSymbol());

  // if (1) string foo; isn't valid!
  if (!decl_stmt->getDeclarationAllowed()) {
    NODE_ERROR(decl_stmt, E_DECLARATION_INVALID_HERE, identifier->getSymbol()->getName());
  }
  return false;
}

/// replace the node's old symbol table, registering the new one.
void SymbolResolutionVisitor::replaceSymbolTable(LSLASTNode *node) {
  // TODO: unregister old table? need to figure out node copy semantics.
  auto *symtab = _mAllocator->newTracked<LSLSymbolTable>();
  node->setSymbolTable(symtab);
  node->mContext->table_manager->registerTable(symtab);
}

bool SymbolResolutionVisitor::visit(LSLLValueExpression *lvalue) {
  lvalue->getIdentifier()->resolveSymbol(SYM_VARIABLE);
  return false;
}

bool SymbolResolutionVisitor::visit(LSLFunctionExpression *func_expr) {
  func_expr->getIdentifier()->resolveSymbol(SYM_FUNCTION);
  return true;
}

bool SymbolResolutionVisitor::visit(LSLGlobalFunction *glob_func) {
  assert(_mPendingJumps.empty());
  visitChildren(glob_func);
  resolvePendingJumps();
  return false;
}

bool SymbolResolutionVisitor::visit(LSLEventHandler *handler) {
  replaceSymbolTable(handler);

  auto *id = handler->getIdentifier();
  // look for a prototype for this event in the builtin namespace
  auto *sym = handler->getRoot()->lookupSymbol(id->getName(), SYM_EVENT);
  if (sym) {
    id->setSymbol(_mAllocator->newTracked<LSLSymbol>(
        id->getName(), id->getType(), SYM_EVENT, SYM_BUILTIN, handler->getLoc(), handler->getArguments()
    ));
    handler->getParent()->defineSymbol(id->getSymbol());
  } else {
    NODE_ERROR(handler, E_INVALID_EVENT, id->getName());
  }

  assert(_mPendingJumps.empty());
  visitChildren(handler);
  resolvePendingJumps();
  return false;
}

static void register_func_param_symbols(LSLASTNode *proto, bool is_event) {
  for (auto *child : *proto) {
    auto *identifier = (LSLIdentifier *) child;
    identifier->setSymbol(proto->mContext->allocator->newTracked<LSLSymbol>(
        identifier->getName(),
        identifier->getType(),
        SYM_VARIABLE,
        is_event ? SYM_EVENT_PARAMETER : SYM_FUNCTION_PARAMETER,
        child->getLoc()
    ));
    proto->defineSymbol(identifier->getSymbol());
  }
}

bool SymbolResolutionVisitor::visit(LSLFunctionDec *func_dec) {
  register_func_param_symbols(func_dec, false);
  return true;
}

bool SymbolResolutionVisitor::visit(LSLEventDec *event_dec) {
  register_func_param_symbols(event_dec, true);
  return true;
}

bool SymbolResolutionVisitor::visit(LSLLabel *label_stmt) {
  auto *identifier = label_stmt->getIdentifier();
  identifier->setSymbol(_mAllocator->newTracked<LSLSymbol>(
      identifier->getName(), identifier->getType(), SYM_LABEL, SYM_LOCAL, label_stmt->getLoc()
  ));
  label_stmt->defineSymbol(identifier->getSymbol());
  _mCollectedLabels.emplace_back(identifier);
  return true;
}

bool SymbolResolutionVisitor::visit(LSLJumpStatement *jump_stmt) {
  // Jumps are special. Since they can jump forwards in the control flow, we
  // can only resolve the labels they refer to after we leave the enclosing
  // function or event handler, having passed the last place the label it
  // refers to could have been defined.
  _mPendingJumps.emplace_back(jump_stmt->getIdentifier());
  return true;
}

bool SymbolResolutionVisitor::visit(LSLStateStatement *state_stmt) {
  state_stmt->getIdentifier()->resolveSymbol(SYM_STATE);
  return true;
}

bool SymbolResolutionVisitor::visit(LSLCompoundStatement *compound_stmt) {
  replaceSymbolTable(compound_stmt);
  return true;
}

void SymbolResolutionVisitor::resolvePendingJumps() {
  for (auto *id : _mPendingJumps) {
    // First do the lookup by lexical scope, triggering an error if it fails.
    id->resolveSymbol(SYM_LABEL);

    // That's all we have to do unless we want to match SL exactly.
    if (!_mLindenJumpSemantics)
      continue;

    // Labels in SL are weird in that they pretend they're lexically scoped but they
    // really aren't in either LSO or Mono. The label you're jumping to _must_
    // be in the lexical scope of your `jump`, but it will actually jump to the
    // last occurrence of a label with a given name within the function body,
    // crossing lexical scope boundaries. This was likely a mistake, but we have
    // to deal with it if we want the same `jump` semantics as LSO. Gnarly.

    // Note that Mono will actually fail to compile correctly on duplicated labels.
    // because they're included in the CIL verbatim.

    if (auto *orig_sym = id->getSymbol()) {
      LSLSymbol *new_sym = nullptr;
      // Now get the label this will jump to in SL, iterate in reverse so the last
      // instance of a label in a function will come first
      for (auto i = _mCollectedLabels.rbegin(); i != _mCollectedLabels.rend(); ++i) {
        auto *cand_sym = (*i)->getSymbol();
        if (!cand_sym || cand_sym->getSymbolType() != SYM_LABEL)
          continue;
        // name matches
        if (!strcmp(cand_sym->getName(), orig_sym->getName())) {
          new_sym = cand_sym;
          break;
        }
      }
      assert(new_sym);
      // This jump specifically will jump to a label other than the one you might expect,
      // so warn on that along with the general warning for duplicate label names.
      if (new_sym != orig_sym) {
        NODE_ERROR(id, W_JUMP_TO_WRONG_LABEL, orig_sym->getName());
      }
      id->setSymbol(new_sym);
    }
  }

  if (_mLindenJumpSemantics) {
    // Walk the list of collected labels and warn on any duplicated names
    std::set<std::string> label_names;
    for (auto &label_id: _mCollectedLabels) {
      if (label_names.find(label_id->getName()) != label_names.end()) {
        NODE_ERROR(label_id, W_DUPLICATE_LABEL_NAME, label_id->getName());
      } else {
        label_names.insert(label_id->getName());
      }
    }
  }

  _mPendingJumps.clear();
  _mCollectedLabels.clear();
}

}
