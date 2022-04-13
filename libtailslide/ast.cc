#include "lslmini.hh"
#include "ast.hh"
#include "visitor.hh"
#include "passes/values.hh"
#include "passes/best_practices.hh"
#include "passes/type_checking.hh"

namespace Tailslide {

LSLASTNode::LSLASTNode(ScriptContext *ctx) : type(nullptr), symbol_table(nullptr), constant_value(nullptr),
                                             declaration_allowed(true), children(nullptr),
                                             next(nullptr), prev(nullptr), parent(nullptr), TrackableObject(ctx) {
  type = TYPE(LST_NULL);
  if (ctx)
    lloc = ctx->glloc;
}

LST_TYPE LSLASTNode::get_itype() {
  return type->get_itype();
}

int LSLASTNode::get_parent_slot() {
  if (!parent)
    return -1;
  auto *child = parent->get_children();
  int idx = 0;
  while(child != nullptr) {
    if (child == this)
      return idx;
    child = child->get_next();
    ++idx;
  }
  return -1;
}

void LSLASTNode::add_children(int num, va_list ap) {
  LSLASTNode *node;
  for (; num--;) {
    node = va_arg(ap, LSLASTNode*);
    if (node == nullptr)
      node = new_null_node();
    push_child(node);
  }
}

LSLASTNode *LSLASTNode::new_null_node() {
  return context->allocator->new_tracked<LSLASTNullNode>();
}


void LSLASTNode::set_parent(LSLASTNode *newparent) {
  // walk tree, (un)registering descendants' symbol tables with
  // the root table
  assert(newparent != this);
  if (static_node) {
    if (!newparent) return;
    assert(0);
  }
  link_symbol_tables(true);

  parent = newparent;
  // because children are an intrusive linked list updating one child's parent
  // must update the parent of all of its siblings.
  for (auto* next_ptr = next; next_ptr != nullptr; next_ptr=next_ptr->next) {
    assert(next_ptr != this);
    next_ptr->parent = newparent;
  }
  for (auto* prev_ptr = prev; prev_ptr != nullptr; prev_ptr=prev_ptr->prev) {
    assert(prev_ptr != this);
    prev_ptr->parent = newparent;
  }

  if (newparent)
    link_symbol_tables();
}

void LSLASTNode::push_child(LSLASTNode *child) {
  if (child == nullptr)
    return;
  if (children == nullptr) {
    children = child;
  } else {
    children->add_next_sibling(child);
  }
  assert (child != this);
  child->set_parent(this);
}

LSLASTNode *LSLASTNode::take_child(int child_num) {
  LSLASTNode *child = get_child(child_num);
  if (child == nullptr)
    return nullptr;
  replace_node(child, new_null_node());
  return child;
}

void LSLASTNode::remove_child(LSLASTNode *child) {
  if (child == nullptr) return;

  LSLASTNode *prev_child = child->get_prev();
  LSLASTNode *next_child = child->get_next();

  child->prev = nullptr;
  child->next = nullptr;

  if (prev_child != nullptr)
    prev_child->set_next(next_child);
  else
    children = next_child;

  if (next_child != nullptr)
    next_child->set_prev(prev_child);

  // must be done last so we don't change the parent of siblings
  child->set_parent(nullptr);
}

void LSLASTNode::set_next(LSLASTNode *newnext) {
  DEBUG(LOG_DEBUG_SPAM, nullptr, "%s.set_next(%s)\n", get_node_name(), newnext ? newnext->get_node_name() : "nullptr");
  next = newnext;
  assert(next != this);
  if (newnext && newnext->get_prev() != this)
    newnext->set_prev(this);
}

void LSLASTNode::set_prev(LSLASTNode *newprev) {
  DEBUG(LOG_DEBUG_SPAM, nullptr, "%s.set_prev(%s)\n", get_node_name(), newprev ? newprev->get_node_name() : "nullptr");
  prev = newprev;
  if (newprev && newprev->get_next() != this)
    newprev->set_next(this);
}

void LSLASTNode::add_next_sibling(LSLASTNode *sibling) {
  assert (sibling != parent);
  assert (sibling != this);
  if (sibling == nullptr) return;
  if (next)
    next->add_next_sibling(sibling);
  else
    set_next(sibling);
}

void LSLASTNode::add_prev_sibling(LSLASTNode *sibling) {
  assert (sibling != parent);
  if (sibling == nullptr) return;
  if (prev)
    prev->add_prev_sibling(sibling);
  else
    set_prev(sibling);
}

void LSLASTNode::replace_node(LSLASTNode *old_node, LSLASTNode *replacement) {
  assert(replacement != nullptr && old_node != nullptr);
  replacement->set_prev(old_node->get_prev());
  replacement->set_next(old_node->get_next());
  auto *parent = old_node->get_parent();

  // First child, have to replace `children` entirely.
  if (old_node->get_prev() == nullptr && old_node->get_parent() != nullptr)
    old_node->get_parent()->children = replacement;
  old_node->next = nullptr;
  old_node->prev = nullptr;
  old_node->set_parent(nullptr);
  replacement->set_parent(parent);
}

void LSLASTNode::visit(ASTVisitor *visitor) {
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


void LSLASTNode::propagate_values() {
  TailslideOperationBehavior behavior(context->allocator);
  ConstantDeterminingVisitor visitor(&behavior);
  visit(&visitor);
}

void LSLASTNode::check_best_practices() {
  BestPracticesVisitor visitor;
  visit(&visitor);
}

// walk tree post-order and propagate types
void LSLASTNode::determine_types() {
  TypeCheckVisitor visitor;
  visit(&visitor);
}


}