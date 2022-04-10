#include "tree_simplifier.hh"

namespace Tailslide {

bool TreeSimplifyingVisitor::visit(LSLDeclaration* node) {
  if (!ctx.prune_unused_locals)
    return true;

  auto *id = (LSLIdentifier *) (node->get_child(0));
  auto *sym = id->get_symbol();
  if (!sym || sym->get_references() != 1 || sym->get_assignments() != 0)
    return true;
  LSLASTNode *rvalue = node->get_child(1);
  // rvalue can't be reduced to a constant, don't know that we don't need
  // the side-effects of evaluating the expression.
  if(rvalue && !rvalue->get_constant_value())
    return true;

  ++folded_total;
  LSLASTNode *ancestor = node;
  // walk up and remove it from whatever symbol table it's in
  while (ancestor != nullptr) {
    if (ancestor->get_symbol_table() != nullptr) {
      if (ancestor->get_symbol_table()->remove(sym))
        break;
    }
    ancestor = ancestor->get_parent();
  }
  assert(node->get_parent() != nullptr);
  node->get_parent()->remove_child(node);
  // child is totally gone now, can't recurse.
  return false;
}

bool TreeSimplifyingVisitor::visit(LSLGlobalStorage* node) {
  // GlobalStorages either contain a single var or a single function.
  LSLASTNode *contained = (node->get_child(0)->get_node_type() != NODE_NULL) ? node->get_child(0) : node->get_child(1);
  // and they both keep their identifier in the first child!
  auto *id = (LSLIdentifier *) (contained->get_child(0));
  assert(id != nullptr && id->get_node_type() == NODE_IDENTIFIER);

  LSLNodeType node_type = contained->get_node_type();
  auto *sym = id->get_symbol();

  if (((node_type == NODE_GLOBAL_FUNCTION && ctx.prune_unused_functions) ||
       (node_type == NODE_GLOBAL_VARIABLE && ctx.prune_unused_globals))
      && sym->get_references() == 1) {
    ++folded_total;
    // these reside in the global scope, look for the root symbol table and the entry
    LSLASTNode *script = node->get_root();
    script->get_symbol_table()->remove(sym);
    // remove the node itself
    node->get_parent()->remove_child(node);
    return false;
  }
  return true;
}

bool TreeSimplifyingVisitor::visit(LSLExpression* node) {
  if (!ctx.fold_constants)
    return true;

  LSLConstant *cv = node->get_constant_value();
  if(!cv)
    return true;
  auto c_type = cv->get_itype();
  // this expression results in a list, don't fold the result in.
  if (c_type == LST_LIST)
    return true;
  // LSL doesn't really have NaN literals, can't fold this.
  if (cv->contains_nan())
    return true;
  // this expression may result in a new entry in the string constant pool,
  // and we're not allowed to create new ones, don't fold.
  // TODO: Might be worth checking for cases where we're just splicing together two
  //  strings that aren't referenced anywhere else.
  if (!ctx.may_create_new_strs && c_type == LST_STRING)
    return true;

  // We're going to change its parent / sibling connections,
  // so we need a copy.
  auto *new_expr = node->context->allocator->new_tracked<LSLConstantExpression>(cv);
  new_expr->set_lloc(node->get_lloc());
  LSLASTNode::replace_node(node, new_expr);
  ++folded_total;

  return false;
}

bool TreeSimplifyingVisitor::visit(LSLLValueExpression *node) {
  if (!ctx.fold_constants)
    return false;

  LSLASTNode *child = node->get_child(0);
  if (child && child->get_node_type() == NODE_IDENTIFIER) {
    auto *id = (LSLIdentifier *)child;
    auto *sym = id->get_symbol();
    if (!sym)
      return false;
    // Is this a builtin constant? Don't bother replacing it.
    // These references are usually "free" given that they're
    // lexer tokens in SL proper.
    if (sym->get_sub_type() == SYM_BUILTIN)
      return false;
    LSLConstant *cv = node->get_constant_value();
    if (cv && !cv->contains_nan()) {
      auto *new_expr = node->context->allocator->new_tracked<LSLConstantExpression>(cv);
      new_expr->set_lloc(node->get_lloc());
      LSLASTNode::replace_node(
          node,
          new_expr
      );
      ++folded_total;
      return false;
    }
  }
  return false;
}

bool TreeSimplifyingVisitor::visit(LSLConstantExpression *node) {
  // Don't touch these at all, they can't be simplified any more!
  return false;
}

}
