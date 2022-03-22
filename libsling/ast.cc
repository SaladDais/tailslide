#include "lslmini.hh"
#include "ast.hh"

LLASTNullNode *LLASTNode::new_null_node() {
  return gAllocationManager->new_tracked<LLASTNullNode>();
}

void LLASTNode::visit(ASTVisitor *visitor) {
  // Use the node type and node subtype retvals to cast and choose
  // a more specific version of the visitor's visit methods to call.
  if(!visitor->visit_specific(this))
    return;
  // if the visitor returned true then we can continue decending into
  // the children as normal. A visitor may return false if it needs to
  // do something both before and after descending into the children,
  // or override the iteration order in some way.
  visitor->visit_children(this);
}