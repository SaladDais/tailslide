#include "lslmini.hh"
#include "ast.hh"
#include "visitor.hh"
#include "passes/values.hh"
#include "passes/best_practices.hh"
#include "passes/type_checking.hh"

namespace Tailslide {

LSLASTNode::LSLASTNode(ScriptContext *ctx) : _mType(nullptr), _mSymbolTable(nullptr), _mConstantValue(nullptr),
                                             _mDeclarationAllowed(true), _mChildren(nullptr),
                                             _mNext(nullptr), _mPrev(nullptr), _mParent(nullptr), TrackableObject(ctx) {
  _mType = TYPE(LST_NULL);
  if (ctx) {
    _mLoc = ctx->glloc;
    _mSynthesized = !ctx->parsing;
  }
}

LSLIType LSLASTNode::getIType() {
  return _mType->getIType();
}

int LSLASTNode::getParentSlot() {
  if (!_mParent)
    return -1;
  auto *child = _mParent->getChildren();
  int idx = 0;
  while(child != nullptr) {
    if (child == this)
      return idx;
    child = child->getNext();
    ++idx;
  }
  return -1;
}

void LSLASTNode::addChildren(int num, va_list ap) {
  LSLASTNode *node;
  for (; num--;) {
    node = va_arg(ap, LSLASTNode*);
    if (node == nullptr)
      node = newNullNode();
    pushChild(node);
  }
}

LSLASTNode *LSLASTNode::newNullNode() {
  return mContext->allocator->newTracked<LSLASTNullNode>();
}


void LSLASTNode::setParent(LSLASTNode *newparent) {
  // walk tree, (un)registering descendants' symbol tables with
  // the root table
  assert(newparent != this);
  if (_mStaticNode) {
    if (!newparent) return;
    assert(0);
  }

  _mParent = newparent;
  // because children are an intrusive linked list updating one child's parent
  // must update the parent of all of its siblings.
  for (auto* next_ptr = _mNext; next_ptr != nullptr; next_ptr=next_ptr->_mNext) {
    assert(next_ptr != this);
    if (next_ptr->_mParent == newparent)
      break;
    next_ptr->_mParent = newparent;
  }
  for (auto* prev_ptr = _mPrev; prev_ptr != nullptr; prev_ptr=prev_ptr->_mPrev) {
    assert(prev_ptr != this);
    if (prev_ptr->_mParent == newparent)
      break;
    prev_ptr->_mParent = newparent;
  }
}

void LSLASTNode::pushChild(LSLASTNode *child) {
  if (child == nullptr)
    return;
  if (_mChildren == nullptr) {
    _mChildren = child;
  } else {
    _mChildren->addNextSibling(child);
  }
  assert (child != this);
  child->setParent(this);
}

LSLASTNode *LSLASTNode::takeChild(int child_num) {
  LSLASTNode *child = getChild(child_num);
  if (child == nullptr)
    return nullptr;
  replaceNode(child, newNullNode());
  return child;
}

void LSLASTNode::removeChild(LSLASTNode *child) {
  if (child == nullptr) return;

  LSLASTNode *prev_child = child->getPrev();
  LSLASTNode *next_child = child->getNext();

  child->_mPrev = nullptr;
  child->_mNext = nullptr;

  if (prev_child != nullptr)
    prev_child->setNext(next_child);
  else
    _mChildren = next_child;

  if (next_child != nullptr)
    next_child->setPrev(prev_child);

  // must be done last so we don't change the parent of siblings
  child->setParent(nullptr);
}

void LSLASTNode::setNext(LSLASTNode *newnext) {
  // DEBUG(LOG_DEBUG_SPAM, nullptr, "%s.setNext(%s)\n", getNodeName(), newnext ? newnext->getNodeName() : "nullptr");
  _mNext = newnext;
  assert(_mNext != this);
  if (newnext && newnext->getPrev() != this)
    newnext->setPrev(this);
}

void LSLASTNode::setPrev(LSLASTNode *newprev) {
  // DEBUG(LOG_DEBUG_SPAM, nullptr, "%s.setPrev(%s)\n", getNodeName(), newprev ? newprev->getNodeName() : "nullptr");
  _mPrev = newprev;
  if (newprev && newprev->getNext() != this)
    newprev->setNext(this);
}

void LSLASTNode::addNextSibling(LSLASTNode *sibling) {
  assert (sibling != _mParent);
  assert (sibling != this);
  if (sibling == nullptr) return;
  auto *last_sibling = this;
  LSLASTNode *next;
  while ((next = last_sibling->_mNext))
    last_sibling = next;
  last_sibling->setNext(sibling);
}

void LSLASTNode::replaceNode(LSLASTNode *old_node, LSLASTNode *replacement) {
  assert(replacement != nullptr && old_node != nullptr);
  replacement->setPrev(old_node->getPrev());
  replacement->setNext(old_node->getNext());
  auto *parent = old_node->getParent();

  // First child, have to replace `_mChildren` entirely.
  if (old_node->getPrev() == nullptr && old_node->getParent() != nullptr)
    old_node->getParent()->_mChildren = replacement;
  old_node->_mNext = nullptr;
  old_node->_mPrev = nullptr;
  old_node->setParent(nullptr);
  replacement->setParent(parent);
}

void LSLASTNode::visit(ASTVisitor *visitor) {
  if (!visitor->isDepthFirst()) {
    // Use the node type and node subtype retvals to cast and choose
    // a more specific version of the visitor's visit methods to call.
    if (!visitor->visitSpecific(this))
      return;
    // if the visitor returned true then we can continue descending into
    // the children as normal. A visitor may return false if it needs to
    // do something both before and after descending into the children,
    // or override the iteration order in some way.
    visitor->visitChildren(this);
  } else {
    // same as above, but for depth-first visitation (like for constant
    // value propagation that flows out from the innermost nodes.)
    // Since the current node is visited _after_ the children, we use
    // an additional method to see if we should block descent into the
    // children before descending.
    if (visitor->beforeDescend(this)) {
      visitor->visitChildren(this);
    }
    visitor->visitSpecific(this);
  }
}


void LSLASTNode::propagateValues(bool create_heap_values) {
  TailslideOperationBehavior behavior(mContext->allocator, create_heap_values);
  ConstantDeterminingVisitor visitor(&behavior, mContext->allocator);
  visit(&visitor);
}

void LSLASTNode::checkBestPractices() {
  BestPracticesVisitor visitor;
  visit(&visitor);
}

// walk tree post-order and propagate types
void LSLASTNode::determineTypes() {
  TypeCheckVisitor visitor;
  visit(&visitor);
}


}