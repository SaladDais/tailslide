#include "lslmini.hh"
#include "ast.hh"

namespace Sling {

LLASTNullNode *LLASTNode::new_null_node() {
  return gAllocationManager->new_tracked<LLASTNullNode>();
}

void LLASTNode::visit(ASTVisitor *visitor) {
  if (!visitor->is_depth_first()) {
    // Use the node type and node subtype retvals to cast and choose
    // a more specific version of the visitor's visit methods to call.
    if (!visitor->visit_specific(this))
      return;
    // if the visitor returned true then we can continue descending into
    // the children as normal. A visitor may return false if it needs to
    // do something both before and after descending into the children,
    // or override the iteration order in some way.
    visitor->visit_children(this);
  } else {
    // same as above, but for depth-first visitation (like for constant
    // value propagation that flows out from the innermost nodes.)
    // Since the current node is visited _after_ the children, we use
    // an additional method to see if we should block descent into the
    // children before descending.
    if (visitor->before_descend(this)) {
      visitor->visit_children(this);
    }
    visitor->visit_specific(this);
  }
}

}