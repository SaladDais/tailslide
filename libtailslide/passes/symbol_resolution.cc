#include "symbol_resolution.hh"
#include "lslmini.hh"

namespace Tailslide {


bool ExprSymbolResolutionVisitor::visit(LSLLValueExpression *node) {
  ((LSLIdentifier*)node->get_child(0))->resolve_symbol(SYM_VARIABLE);
  return false;
}

bool ExprSymbolResolutionVisitor::visit(LSLFunctionExpression *node) {
  auto *id = (LSLIdentifier *) node->get_child(0);
  id->resolve_symbol(SYM_FUNCTION);
  return true;
}


// all global var definitions are implicitly hoisted above function definitions
// all functions and states have their declarations implicitly hoisted as well.
class GlobalSymbolResolutionVisitor: public ExprSymbolResolutionVisitor {
  public:
    virtual bool visit(LSLGlobalVariable *node) {
      // descend first so we can resolve any symbol references present in the rvalue
      // before we've defined the identifier from the lvalue.
      // Necessary so things like `string foo = foo;` will error correctly.
      visit_children(node);

      auto *identifier = (LSLIdentifier *) node->get_children();
      identifier->set_symbol(node->context->allocator->new_tracked<LSLSymbol>(
          identifier->get_name(), identifier->get_type(), SYM_VARIABLE, SYM_GLOBAL, node->get_lloc(), nullptr, node->get_parent()
      ));
      node->define_symbol(identifier->get_symbol());
      return false;
    };

    virtual bool visit(LSLGlobalFunction *node) {
      auto *identifier = (LSLIdentifier *) node->get_child(0);

      // define function in parent scope since functions have their own scope
      identifier->set_symbol(node->context->allocator->new_tracked<LSLSymbol>(
          identifier->get_name(),
          identifier->get_type(),
          SYM_FUNCTION,
          SYM_GLOBAL,
          node->get_lloc(),
          (LSLFunctionDec *) node->get_child(1)
      ));
      node->get_parent()->define_symbol(identifier->get_symbol());
      // don't descend, we only want the declaration.
      return false;
    };

    virtual bool visit(LSLState *node) {
      LSLASTNode *maybe_id = node->get_children();
      LSLIdentifier *identifier;

      if (maybe_id->get_node_type() == NODE_NULL) // null identifier = default state, nothing to define
        return false;

      identifier = (LSLIdentifier *) maybe_id;
      identifier->set_symbol(node->context->allocator->new_tracked<LSLSymbol>(
          identifier->get_name(), identifier->get_type(), SYM_STATE, SYM_GLOBAL, identifier->get_lloc()
      ));
      node->get_parent()->define_symbol(identifier->get_symbol());
      // don't descend, we only want the declaration
      return false;
    };
};

bool SymbolResolutionVisitor::visit(LSLScript *node) {
  // Walk over just the globals before we descend into function
  // bodies and do general symbol resolution.
  GlobalSymbolResolutionVisitor visitor;
  node->visit(&visitor);
  return true;
}

bool SymbolResolutionVisitor::visit(LSLDeclaration *node) {
  // visit the rvalue first so we correctly handle things like
  // `string foo = foo;`
  LSLASTNode *rvalue = node->get_child(1);
  if (rvalue)
    rvalue->visit(this);

  auto *identifier = (LSLIdentifier *) node->get_child(0);
  identifier->set_symbol(node->context->allocator->new_tracked<LSLSymbol>(
      identifier->get_name(), identifier->get_type(), SYM_VARIABLE, SYM_LOCAL, node->get_lloc(), nullptr, node
  ));
  node->define_symbol(identifier->get_symbol());

  if (!node->get_declaration_allowed()) {
    NODE_ERROR(node, E_DECLARATION_INVALID_HERE, identifier->get_symbol()->get_name());
  }
  return false;
}

static void register_func_param_symbols(LSLASTNode *proto, bool is_event) {
  LSLASTNode *child = proto->get_children();
  while (child) {
    auto *identifier = (LSLIdentifier *) child;
    identifier->set_symbol(proto->context->allocator->new_tracked<LSLSymbol>(
        identifier->get_name(),
        identifier->get_type(),
        SYM_VARIABLE,
        is_event ? SYM_EVENT_PARAMETER : SYM_FUNCTION_PARAMETER,
        child->get_lloc()
    ));
    proto->define_symbol(identifier->get_symbol());
    child = child->get_next();
  }
}

bool SymbolResolutionVisitor::visit(LSLGlobalFunction *node) {
  assert(_pending_jumps.empty());
  visit_children(node);
  _resolve_pending_jumps();
  return false;
}

bool SymbolResolutionVisitor::visit(LSLFunctionDec *node) {
  register_func_param_symbols(node, false);
  return true;
}

bool SymbolResolutionVisitor::visit(LSLGlobalVariable *node) {
  // Symbol resolution for everything inside is already done, don't descend.
  return false;
}

bool SymbolResolutionVisitor::visit(LSLEventHandler *node) {
  auto *id = (LSLIdentifier *) node->get_child(0);
  // look for a prototype for this event in the builtin namespace
  auto *sym = node->get_root()->lookup_symbol(id->get_name(), SYM_EVENT);
  if (sym) {
    id->set_symbol(node->context->allocator->new_tracked<LSLSymbol>(
        id->get_name(), id->get_type(), SYM_EVENT, SYM_BUILTIN, node->get_lloc(), (LSLParamList*)node->get_child(1)
    ));
    node->get_parent()->define_symbol(id->get_symbol());
  } else {
    NODE_ERROR(node, E_INVALID_EVENT, id->get_name());
  }

  assert(_pending_jumps.empty());
  visit_children(node);
  _resolve_pending_jumps();
  return false;
}

bool SymbolResolutionVisitor::visit(LSLEventDec *node) {
  register_func_param_symbols(node, true);
  return true;
}

bool SymbolResolutionVisitor::visit(LSLLabel *node) {
  auto *identifier = (LSLIdentifier *) node->get_child(0);
  identifier->set_symbol(node->context->allocator->new_tracked<LSLSymbol>(
      identifier->get_name(), identifier->get_type(), SYM_LABEL, SYM_LOCAL, node->get_lloc()
  ));
  node->define_symbol(identifier->get_symbol());
  _collected_labels.emplace_back(identifier);
  return true;
}

bool SymbolResolutionVisitor::visit(LSLJumpStatement *node) {
  // Jumps are special. Since they can jump forwards in the control flow, we
  // can only resolve the labels they refer to after we leave the enclosing
  // function or event handler, having passed the last place the label it
  // refers to could have been defined.
  _pending_jumps.emplace_back((LSLIdentifier*)node->get_child(0));
  return true;
}

bool SymbolResolutionVisitor::visit(LSLStateStatement *node) {
  ((LSLIdentifier *) node->get_child(0))->resolve_symbol(SYM_STATE);
  return true;
}

void SymbolResolutionVisitor::_resolve_pending_jumps() {
  for (auto *id : _pending_jumps) {
    // First do the lookup by lexical scope, triggering an error if it fails.
    id->resolve_symbol(SYM_LABEL);

    // That's all we have to do unless we want to match SL exactly.
    if (!_linden_jump_semantics)
      continue;

    // Labels in SL are weird in that they pretend they're lexically scoped but they
    // really aren't in either LSO or Mono. The label you're jumping to _must_
    // be in the lexical scope of your `jump`, but it will actually jump to the
    // last occurrence of a label with a given name within the function body,
    // crossing lexical scope boundaries. This was likely a mistake, but we have
    // to deal with it if we want the same `jump` semantics as LSO. Gnarly.

    // Note that Mono will actually fail to compile correctly on duplicated labels.
    // because they're included in the CIL verbatim.

    if (auto *orig_sym = id->get_symbol()) {
      LSLSymbol *new_sym = nullptr;
      // Now get the label this will jump to in SL, iterate in reverse so the last
      // instance of a label in a function will come first
      for (auto i = _collected_labels.rbegin(); i != _collected_labels.rend(); ++i) {
        auto *cand_sym = (*i)->get_symbol();
        if (!cand_sym || cand_sym->get_symbol_type() != SYM_LABEL)
          continue;
        // name matches
        if (!strcmp(cand_sym->get_name(), orig_sym->get_name())) {
          new_sym = cand_sym;
          break;
        }
      }
      assert(new_sym);
      // This jump specifically will jump to a label other than the one you might expect,
      // so warn on that along with the general warning for duplicate label names.
      if (new_sym != orig_sym) {
        NODE_ERROR(id, W_JUMP_TO_WRONG_LABEL, orig_sym->get_name());
      }
      id->set_symbol(new_sym);
    }
  }

  if (_linden_jump_semantics) {
    // Walk the list of collected labels and warn on any duplicated names
    std::set<std::string> label_names;
    for (auto &label_id: _collected_labels) {
      if (label_names.find(label_id->get_name()) != label_names.end()) {
        NODE_ERROR(label_id, W_DUPLICATE_LABEL_NAME, label_id->get_name());
      } else {
        label_names.insert(label_id->get_name());
      }
    }
  }

  _pending_jumps.clear();
  _collected_labels.clear();
}

}
