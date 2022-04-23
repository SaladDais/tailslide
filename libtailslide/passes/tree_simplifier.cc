#include "tree_simplifier.hh"

namespace Tailslide {

bool TreeSimplifyingVisitor::visit(LSLDeclaration* node) {
  if (!mOpts.prune_unused_locals)
    return true;

  auto *id = (LSLIdentifier *) (node->getChild(0));
  auto *sym = id->getSymbol();
  if (!sym || sym->getReferences() != 1 || sym->getAssignments() != 0)
    return true;
  LSLASTNode *rvalue = node->getChild(1);
  // rvalue can't be reduced to a constant, don't know that we don't need
  // the side-effects of evaluating the expression.
  if(rvalue && !rvalue->getConstantValue())
    return true;

  ++mFoldedLevel;
  LSLASTNode *ancestor = node;
  // walk up and remove it from whatever symbol table it's in
  while (ancestor != nullptr) {
    if (ancestor->getSymbolTable() != nullptr) {
      if (ancestor->getSymbolTable()->remove(sym))
        break;
    }
    ancestor = ancestor->getParent();
  }
  assert(node->getParent() != nullptr);
  node->getParent()->removeChild(node);
  // child is totally gone now, can't recurse.
  return false;
}

bool TreeSimplifyingVisitor::visit(LSLGlobalStorage* node) {
  // GlobalStorages either contain a single var or a single function.
  LSLASTNode *contained = (node->getChild(0)->getNodeType() != NODE_NULL) ? node->getChild(0) : node->getChild(1);
  // and they both keep their identifier in the first child!
  auto *id = (LSLIdentifier *) (contained->getChild(0));
  assert(id != nullptr && id->getNodeType() == NODE_IDENTIFIER);

  LSLNodeType node_type = contained->getNodeType();
  auto *sym = id->getSymbol();

  if (((node_type == NODE_GLOBAL_FUNCTION && mOpts.prune_unused_functions) ||
       (node_type == NODE_GLOBAL_VARIABLE && mOpts.prune_unused_globals))
      && sym->getReferences() == 1) {
    ++mFoldedLevel;
    // these reside in the global scope, look for the root symbol table and the entry
    LSLASTNode *script = node->getRoot();
    script->getSymbolTable()->remove(sym);
    // remove the node itself
    node->getParent()->removeChild(node);
    return false;
  }
  return true;
}

bool TreeSimplifyingVisitor::visit(LSLExpression* node) {
  if (!mOpts.fold_constants)
    return true;

  LSLConstant *cv = node->getConstantValue();
  if(!cv)
    return true;
  auto c_type = cv->getIType();
  // this expression results in a list, don't fold the result in.
  if (c_type == LST_LIST)
    return true;
  // Need to be careful about where we inline these (I.E. in list casts or expressions)
  // just avoid inlining them at all for now.
  if (c_type == LST_KEY)
    return true;
  // LSL doesn't really have NaN literals, can't fold this.
  if (cv->containsNaN())
    return true;
  // this expression may result in a new entry in the string constant pool,
  // and we're not allowed to create new ones, don't fold.
  // TODO: Might be worth checking for cases where we're just splicing together two
  //  strings that aren't referenced anywhere else.
  if (!mOpts.may_create_new_strs && c_type == LST_STRING)
    return true;

  // We're going to change its parent / sibling connections,
  // so we need a copy.
  auto *new_expr = node->mContext->allocator->newTracked<LSLConstantExpression>(cv);
  new_expr->setLoc(node->getLoc());
  LSLASTNode::replaceNode(node, new_expr);
  ++mFoldedLevel;

  return false;
}

bool TreeSimplifyingVisitor::visit(LSLLValueExpression *node) {
  if (!mOpts.fold_constants)
    return false;

  LSLASTNode *child = node->getChild(0);
  if (child && child->getNodeType() == NODE_IDENTIFIER) {
    auto *id = (LSLIdentifier *)child;
    auto *sym = id->getSymbol();
    if (!sym)
      return false;
    // Is this a builtin constant? Don't bother replacing it.
    // These references are usually "free" given that they're
    // lexer tokens in SL proper.
    if (sym->getSubType() == SYM_BUILTIN)
      return false;

    // list lvalues may never be inlined.
    if (sym->getIType() == LST_LIST)
      return false;

    // Keys have special inlining rules so key-ness isn't lost.
    if (sym->getIType() == LST_KEY) {
      auto *node_ancestor = node->getParent();
      LSLASTNode *top_expr = node;

      // Need to be careful about which expressions we inline keys under (I.E. list casts and list expressions)
      while (node_ancestor && node_ancestor->getNodeType() == NODE_EXPRESSION) {
        if (node_ancestor->getNodeSubType() == NODE_LIST_EXPRESSION)
          return false;
        if (node_ancestor->getNodeSubType() == NODE_TYPECAST_EXPRESSION && node_ancestor->getIType() == LST_LIST)
          return false;
        // key-ness matters for print()!
        if (node_ancestor->getNodeSubType() == NODE_PRINT_EXPRESSION)
          return false;
        top_expr = node_ancestor;
        node_ancestor = node_ancestor->getParent();
      }
      // Still have ancestors after going past the top level of the expression, might be statements.
      // where the key-ness matters.
      if (top_expr->getIType() == LST_KEY) {
        node_ancestor = top_expr->getParent();
        // Might be the expression list of a for loop, look one above.
        if (node_ancestor && node_ancestor->getNodeType() == NODE_AST_NODE_LIST)
          node_ancestor = node_ancestor->getParent();
        if (node_ancestor && node_ancestor->getNodeType() == NODE_STATEMENT) {
          switch (node_ancestor->getNodeSubType()) {
            case NODE_WHILE_STATEMENT:
            case NODE_IF_STATEMENT:
            case NODE_DO_STATEMENT:
            case NODE_FOR_STATEMENT:
              // have to be careful about inlining directly within conditional expressions because
              // `key k = "1"; if (k) {...}` and `if ("1") { ... }` branch differently!
              return false;
            default:
              break;
          }
        }
      }
    }
    LSLConstant *cv = node->getConstantValue();
    if (cv && !cv->containsNaN()) {
      auto *new_expr = node->mContext->allocator->newTracked<LSLConstantExpression>(cv);
      new_expr->setLoc(node->getLoc());
      LSLASTNode::replaceNode(
          node,
          new_expr
      );
      ++mFoldedLevel;
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
