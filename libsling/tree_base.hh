#ifndef LSLINT_TREE_BASE_H
#define LSLINT_TREE_BASE_H

#include <cstddef>
#include <cstdarg>
#include <cassert>
#include "logger.hh"

namespace Sling {

template<typename T, typename TNullType>
class ATreeBase : public LLTrackableObject {
protected:
    ATreeBase() : children(NULL), parent(NULL), next(NULL), prev(NULL) {}

public:
    ~ATreeBase() override = default;

    void add_children(int num, va_list ap) {
      T *node;
      for (; num--;) {
        node = va_arg(ap, T*);
        if (node == NULL)
          node = T::new_null_node();
        push_child(node);
      }
    }

    T *get_next() { return next; }

    T *get_prev() { return prev; }

    T *get_children() { return children; }

    T *get_parent() { return parent; }

    T *get_child(int i) {
      T *c = children;
      while (i-- && c)
        c = c->get_next();
      return c;
    }

    // Get the topmost node in the tree
    T *get_root() {
      T *last_node = (T *) this;
      T *test_root;
      while ((test_root = last_node->get_parent()) != nullptr)
        last_node = test_root;
      return last_node;
    }

    // Add child to end of list.
    void push_child(T *child) {
      if (child == nullptr) return;
      if (children == nullptr) {
        children = child;
      } else {
        children->add_next_sibling(child);
      }
      assert (child != this);
      child->set_parent((T *) this);
    }

    // replace an arbitrary node with another, setting
    // prev, next and parent as appropriate
    static void replace_node(T *old_node, T *replacement) {
      assert(replacement != NULL && old_node != NULL);
      replacement->set_prev(old_node->get_prev());
      replacement->set_next(old_node->get_next());
      T *parent = old_node->get_parent();

      // First child, have to replace `children` entirely.
      if (old_node->get_prev() == NULL && old_node->get_parent() != NULL)
        old_node->get_parent()->children = replacement;
      old_node->next = NULL;
      old_node->prev = NULL;
      old_node->set_parent(NULL);
      replacement->set_parent(parent);
    }

    /* remove a child from the list of nodes, shifting other children up */
    void remove_child(T *child) {
      if (child == NULL) return;

      T *prev_child = child->get_prev();
      T *next_child = child->get_next();

      child->prev = NULL;
      child->next = NULL;

      if (prev_child != NULL)
        prev_child->set_next(next_child);
      else
        children = next_child;

      if (next_child != NULL)
        next_child->set_prev(prev_child);

      // TODO: unset symbol table entries for descendants of popped?
      // must be done last so we don't change the parent of siblings
      child->set_parent(NULL);
    }

    /* replace a node from the list of children with null, returning it */
    T *take_child(int child_num) {
      T *child = get_child(child_num);
      if (child == NULL)
        return nullptr;
      T::replace_node(child, T::new_null_node());
      return child;
    }

    virtual void set_parent(T *newparent) {
      parent = newparent;
      // because children are an intrusive linked list updating one child's parent
      // must update the parent of all of its siblings.
      for (T* next_ptr = next; next_ptr != nullptr; next_ptr=next_ptr->next) {
        assert(next_ptr != this);
        next_ptr->parent = newparent;
      }
      for (T* prev_ptr = prev; prev_ptr != nullptr; prev_ptr=prev_ptr->prev) {
        assert(prev_ptr != this);
        prev_ptr->parent = newparent;
      }
    }

    /* Set our next sibling, and ensure it links back to us. */
    void set_next(T *newnext) {
      DEBUG(LOG_DEBUG_SPAM, NULL, "%s.set_next(%s)\n", get_node_name(), newnext ? newnext->get_node_name() : "NULL");
      next = newnext;
      assert(next != this);
      if (newnext && newnext->get_prev() != this)
        newnext->set_prev((T *) this);
    }

    /* Set our previous sibling, and ensure it links back to us. */
    void set_prev(T *newprev) {
      DEBUG(LOG_DEBUG_SPAM, NULL, "%s.set_prev(%s)\n", get_node_name(), newprev ? newprev->get_node_name() : "NULL");
      prev = newprev;
      if (newprev && newprev->get_next() != this)
        newprev->set_next((T *) this);
    }

    void add_next_sibling(T *sibling) {
      assert (sibling != parent);
      assert (sibling != this);
      if (sibling == nullptr) return;
      if (next)
        next->add_next_sibling(sibling);
      else
        set_next(sibling);
    }

    void add_prev_sibling(T *sibling) {
      assert (sibling != parent);
      if (sibling == nullptr) return;
      if (prev)
        prev->add_prev_sibling(sibling);
      else
        set_prev(sibling);
    }

    virtual const char *get_node_name() { return "base node"; };

protected:
    static TNullType *new_null_node() { return new TNullType(); }

private:
    T *children;
    T *parent;
    T *next;
    T *prev;
};

}

#endif //LSLINT_TREE_BASE_H
