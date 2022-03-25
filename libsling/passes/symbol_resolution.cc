#include "symbol_resolution.hh"
#include "lslmini.hh"

namespace Sling {


bool SymbolResolutionVisitor::visit(LLScriptDeclaration *node) {
  auto *identifier = (LLScriptIdentifier *) node->get_children();
  identifier->set_symbol(gAllocationManager->new_tracked<LLScriptSymbol>(
      identifier->get_name(), identifier->get_type(), SYM_VARIABLE, SYM_LOCAL, node->get_lloc(), nullptr, node
  ));
  node->define_symbol(identifier->get_symbol());

  if (!node->get_declaration_allowed()) {
    ERROR(IN(node), E_DECLARATION_INVALID_HERE, identifier->get_symbol()->get_name());
  }
  return true;
}

bool SymbolResolutionVisitor::visit(LLScriptGlobalVariable *node) {
  auto *identifier = (LLScriptIdentifier *) node->get_children();
  identifier->set_symbol(gAllocationManager->new_tracked<LLScriptSymbol>(
      identifier->get_name(), identifier->get_type(), SYM_VARIABLE, SYM_GLOBAL, node->get_lloc(), nullptr, node->get_parent()
  ));
  node->define_symbol(identifier->get_symbol());
  return true;
}

bool SymbolResolutionVisitor::visit(LLScriptState *node) {
  LLASTNode *maybe_id = node->get_children();
  LLScriptIdentifier *identifier;

  if (maybe_id->get_node_type() == NODE_NULL) // null identifier = default state, nothing to define
    return true;

  identifier = (LLScriptIdentifier *) maybe_id;
  identifier->set_symbol(gAllocationManager->new_tracked<LLScriptSymbol>(
      identifier->get_name(), identifier->get_type(), SYM_STATE, SYM_GLOBAL, identifier->get_lloc()
  ));
  node->get_parent()->define_symbol(identifier->get_symbol());
  return true;
}

bool SymbolResolutionVisitor::visit(LLScriptGlobalFunction *node) {
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
  return true;
}

bool SymbolResolutionVisitor::visit(LLScriptFunctionDec *node) {
  LLScriptIdentifier *identifier;
  LLASTNode *child = node->get_children();
  while (child) {
    identifier = (LLScriptIdentifier *) child;
    identifier->set_symbol(gAllocationManager->new_tracked<LLScriptSymbol>(
        identifier->get_name(), identifier->get_type(), SYM_VARIABLE, SYM_FUNCTION_PARAMETER, child->get_lloc()
    ));
    node->define_symbol(identifier->get_symbol());
    child = child->get_next();
  }
  return true;
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
  LLScriptIdentifier *identifier;
  LLASTNode *child = node->get_children();
  while (child) {
    identifier = (LLScriptIdentifier *) child;
    identifier->set_symbol(gAllocationManager->new_tracked<LLScriptSymbol>(
        identifier->get_name(), identifier->get_type(), SYM_VARIABLE, SYM_EVENT_PARAMETER, child->get_lloc()
    ));
    node->define_symbol(identifier->get_symbol());
    child = child->get_next();
  }
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

}
