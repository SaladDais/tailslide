#include "symbol_resolution.hh"
#include "lslmini.hh"

namespace Sling {


// all global var definitions are implicitly hoisted above function definitions
// all functions and states have their declarations implicitly hoisted as well.
class GlobalSymbolResolutionVisitor: public ASTVisitor {
  public:
    virtual bool visit(LLScriptGlobalVariable *node) {
      // descend first so we can resolve any symbol references present in the rvalue
      // SimpleAssignable before we've defined the identifier from the lvalue.
      // Necessary so things like `string foo = foo;` will error correctly.
      visit_children(node);

      auto *identifier = (LLScriptIdentifier *) node->get_children();
      identifier->set_symbol(gAllocationManager->new_tracked<LLScriptSymbol>(
          identifier->get_name(), identifier->get_type(), SYM_VARIABLE, SYM_GLOBAL, node->get_lloc(), nullptr, node->get_parent()
      ));
      node->define_symbol(identifier->get_symbol());
      return false;
    };

    virtual bool visit(LLScriptSimpleAssignable *node) {
      LLASTNode *s_val = node->get_child(0);
      if (s_val && s_val->get_node_type() == NODE_IDENTIFIER) {
        auto *id = (LLScriptIdentifier *) s_val;
        id->resolve_symbol(SYM_VARIABLE);
        return false;
      }
      return true;
    }

    virtual bool visit(LLScriptGlobalFunction *node) {
      auto *identifier = (LLScriptIdentifier *) node->get_child(0);

      // define function in parent scope since functions have their own scope
      identifier->set_symbol(gAllocationManager->new_tracked<LLScriptSymbol>(
          identifier->get_name(),
          identifier->get_type(),
          SYM_FUNCTION,
          SYM_GLOBAL,
          node->get_lloc(),
          (LLScriptFunctionDec *) node->get_child(1)
      ));
      node->get_parent()->define_symbol(identifier->get_symbol());
      // don't descend, we only want the declaration.
      return false;
    };

    virtual bool visit(LLScriptState *node) {
      LLASTNode *maybe_id = node->get_children();
      LLScriptIdentifier *identifier;

      if (maybe_id->get_node_type() == NODE_NULL) // null identifier = default state, nothing to define
        return false;

      identifier = (LLScriptIdentifier *) maybe_id;
      identifier->set_symbol(gAllocationManager->new_tracked<LLScriptSymbol>(
          identifier->get_name(), identifier->get_type(), SYM_STATE, SYM_GLOBAL, identifier->get_lloc()
      ));
      node->get_parent()->define_symbol(identifier->get_symbol());
      // don't descend, we only want the declaration
      return false;
    };
};

bool SymbolResolutionVisitor::visit(LLScriptScript *node) {
  // Walk over just the globals before we descend into function
  // bodies and do general symbol resolution.
  GlobalSymbolResolutionVisitor visitor;
  node->visit(&visitor);
  return true;
}

bool SymbolResolutionVisitor::visit(LLScriptDeclaration *node) {
  // visit the rvalue first so we correctly handle things like
  // `string foo = foo;`
  LLASTNode *rvalue = node->get_child(1);
  if (rvalue)
    rvalue->visit(this);

  auto *identifier = (LLScriptIdentifier *) node->get_child(0);
  identifier->set_symbol(gAllocationManager->new_tracked<LLScriptSymbol>(
      identifier->get_name(), identifier->get_type(), SYM_VARIABLE, SYM_LOCAL, node->get_lloc(), nullptr, node
  ));
  node->define_symbol(identifier->get_symbol());

  if (!node->get_declaration_allowed()) {
    ERROR(IN(node), E_DECLARATION_INVALID_HERE, identifier->get_symbol()->get_name());
  }
  return false;
}

static void register_func_param_symbols(LLASTNode *proto, bool is_event) {
  LLASTNode *child = proto->get_children();
  while (child) {
    auto *identifier = (LLScriptIdentifier *) child;
    identifier->set_symbol(gAllocationManager->new_tracked<LLScriptSymbol>(
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

bool SymbolResolutionVisitor::visit(LLScriptFunctionDec *node) {
  register_func_param_symbols(node, false);
  return true;
}

bool SymbolResolutionVisitor::visit(LLScriptGlobalVariable *node) {
  // Symbol resolution for everything inside is already done, don't descend.
  return false;
}

bool SymbolResolutionVisitor::visit(LLScriptEventHandler *node) {
  auto *id = (LLScriptIdentifier *) node->get_child(0);
  // look for a prototype for this event in the builtin namespace
  auto *sym = node->get_root()->lookup_symbol(id->get_name(), SYM_EVENT);
  if (!sym) {
    ERROR(IN(node), E_INVALID_EVENT, id->get_name());
    return true;
  }
  id->set_symbol(gAllocationManager->new_tracked<LLScriptSymbol>(
      id->get_name(), id->get_type(), SYM_EVENT, SYM_BUILTIN, node->get_lloc(), sym->get_function_decl()
  ));
  node->get_parent()->define_symbol(id->get_symbol());
  return true;
}

bool SymbolResolutionVisitor::visit(LLScriptEventDec *node) {
  register_func_param_symbols(node, true);
  return true;
}

bool SymbolResolutionVisitor::visit(LLScriptLabel *node) {
  auto *identifier = (LLScriptIdentifier *) node->get_children();
  identifier->set_symbol(gAllocationManager->new_tracked<LLScriptSymbol>(
      identifier->get_name(), identifier->get_type(), SYM_LABEL, SYM_LOCAL, node->get_lloc()
  ));
  node->define_symbol(identifier->get_symbol());
  return true;
}

bool SymbolResolutionVisitor::visit(LLScriptStateStatement *node) {
  if (auto *id = (LLScriptIdentifier *) node->get_child(0))
    id->resolve_symbol(SYM_STATE);
  return true;
}

bool SymbolResolutionVisitor::visit(LLScriptLValueExpression *node) {
  auto *id = (LLScriptIdentifier *) node->get_child(0);
  id->resolve_symbol(SYM_VARIABLE);
  // not interested in the identifier children
  return false;
}

bool SymbolResolutionVisitor::visit(LLScriptFunctionExpression *node) {
  auto *id = (LLScriptIdentifier *) node->get_child(0);
  id->resolve_symbol(SYM_FUNCTION);
  return true;
}

}
