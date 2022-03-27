#include <cstdio>
#include <cstring>
#include <cassert>

#include "lslmini.hh"
#include "logger.hh"
#include "ast.hh"
#include "visitor.hh"
#include "passes/tree_simplifier.hh"
#include "passes/symbol_resolution.hh"


extern FILE *yyin;
//extern int yynerrs;
int yynwarns = 0;                // not defined by flex but named for consistency

//int yyerror(const char *fmt, ... );
//int yywarning(const char *fmt, ...);

namespace Sling {

const char *DEPRECATED_FUNCTIONS[][2] = {
        {"llSoundPreload",     "llPreloadSound"},
        {"llSound",            "llPlaySound, llLoopSound, or llTriggerSound"},
        {"llMakeExplosion",    "llParticleSystem"},
        {"llMakeFire",         "llParticleSystem"},
        {"llMakeFountain",     "llParticleSystem"},
        {"llMakeSmoke",        "llParticleSystem"},
        {"llRemoteLoadScript", "llRemoteLoadScriptPin and llSetRemoteScriptAccessPin"},
        {"llPointAt",          nullptr},
        {"llStopPointAt",      nullptr},
        {nullptr,              nullptr},
};


class SymbolLinkageVisitor: public ASTVisitor {
public:
    SymbolLinkageVisitor(bool unlink, LLScriptSymbolTable* root_table):
            unlink(unlink), root_table(root_table) {};

    bool visit(LLASTNode *node) override {
      LLScriptSymbolTable *our_table = node->get_symbol_table();
      if (our_table != nullptr) {
        if (unlink)
          root_table->unregister_subtable(our_table);
        else
          root_table->register_subtable(our_table);
      }
      return true;
    }
private:
    bool unlink;
    LLScriptSymbolTable *root_table;
};

void LLASTNode::link_symbol_tables(bool unlink) {
  LLASTNode *root = get_root();
  LLScriptSymbolTable *root_table = root ? root->get_symbol_table() : nullptr;
  if (root_table == nullptr)
    return;
  // NB: if setting parent to a node in another tree,
  // set parent to NULL first.
  auto visitor = SymbolLinkageVisitor(unlink, root_table);
  visit(&visitor);
}

// Lookup a symbol, propogating up the tree until it is found.
LLScriptSymbol *LLASTNode::lookup_symbol(const char *name, LLSymbolType sym_type, LLASTNode *start_node) {
  LLScriptSymbol *sym = nullptr;

  // If we have a symbol table of our own, look for it there
  if (symbol_table)
    sym = symbol_table->lookup(name, sym_type);

  // If we have no symbol table, or it wasn't in it, but we have a parent, ask them
  if (sym == nullptr && get_parent())
    sym = get_parent()->lookup_symbol(name, sym_type, start_node != nullptr ? start_node : this);

  return sym;
}

// Define a symbol, propogating up the tree to the nearest scope level.
void LLASTNode::define_symbol(LLScriptSymbol *symbol, bool check_existing) {

  // If we have a symbol table, define it there
  if (symbol_table) {
    LLScriptSymbol *shadow;

    DEBUG(LOG_DEBUG_SPAM, nullptr, "symbol definition caught in %s\n", get_node_name());

    if (check_existing) {
      // Check if already defined, if it exists in the current scope then shadowing is never allowed!
      shadow = symbol_table->lookup(symbol->get_name());
      if (shadow) {
        if (shadow->get_symbol_type() == SYM_EVENT)
          if (symbol->get_symbol_type() == SYM_EVENT)
            ERROR(IN(symbol), E_MULTIPLE_EVENT_HANDLERS, symbol->get_name());
          else
            ERROR(IN(symbol), E_EVENT_AS_IDENTIFIER, symbol->get_name());
        else
          ERROR(IN(symbol), E_DUPLICATE_DECLARATION, symbol->get_name(), shadow->get_lloc()->first_line, shadow->get_lloc()->first_column);
      } else {
        // Check for shadowed declarations
        if (shadow == nullptr && get_parent())
          shadow = get_parent()->lookup_symbol(symbol->get_name(), symbol->get_symbol_type());
        if (shadow == nullptr && get_root())
          shadow = get_root()->lookup_symbol(symbol->get_name());

        // define it for now even if it shadows so that we have something to work with.
        symbol_table->define(symbol);

        if (shadow != nullptr) {
          // events are _expected_ to "shadow" the event prototype declaration from the outer scope.
          if (shadow->get_symbol_type() == SYM_EVENT && symbol->get_symbol_type() == SYM_EVENT)
            return;
          if (shadow->get_sub_type() == SYM_BUILTIN) {
            // you're never allowed to shadow event names
            if (shadow->get_symbol_type() == SYM_EVENT)
              ERROR(IN(symbol), E_EVENT_AS_IDENTIFIER, symbol->get_name());
            else if (shadow->get_symbol_type() != SYM_FUNCTION || symbol->get_symbol_type() == SYM_FUNCTION)
              ERROR(IN(symbol), E_SHADOW_CONSTANT, symbol->get_name());
          } else {
            // nothing in a local scope can ever shadow a function, both
            // can be referenced simultaneously. Anything other than a function _will_ be shadowed.
            if (shadow->get_symbol_type() != SYM_FUNCTION || symbol->get_symbol_type() == SYM_FUNCTION)
              ERROR(IN(symbol), W_SHADOW_DECLARATION, symbol->get_name(), LINECOL(shadow->get_lloc()));
          }
        }
      }
    } else {
      symbol_table->define(symbol);
    }
  } else if (get_parent()) {
    // Otherwise, ask our parent to define it
    get_parent()->define_symbol(symbol);
  } else {
    // .. but if we don't have a parent, we're in trouble.
    throw "nowhere to define symbol!";
  }
}

// Define any symbols we have, and ask our children to
void LLASTNode::collect_symbols() {
  SymbolResolutionVisitor visitor;
  this->visit(&visitor);
}


/// Identifiers should have their type/symbol set by their parent node, because they don't know what
/// kind of symbol they represent by themselves. For example, this should work:
//    string test() { return "hi"; }
//    string func() {
//      integer test = 1;
//      llOwnerSay(test());
//    }
//  But if "test" looked itself up, it would think it is an integer. It's parent function
//  expression node can tell it what it needs to be before determining it's own type.
void LLScriptIdentifier::resolve_symbol(LLSymbolType symbol_type) {

  // If we already have a symbol, we don't need to look it up.
  if (symbol != nullptr) {
    type = symbol->get_type();
    return;
  }

  // If it's a builtin, check for deprecation
  if (symbol_type == SYM_FUNCTION) {
    int i;
    for (i = 0; DEPRECATED_FUNCTIONS[i][0]; ++i) {
      if (!strcmp(name, DEPRECATED_FUNCTIONS[i][0])) {
        if (DEPRECATED_FUNCTIONS[i][1] == nullptr) {
          ERROR(HERE, E_DEPRECATED, name);
        } else {
          ERROR(HERE, E_DEPRECATED_WITH_REPLACEMENT, name, DEPRECATED_FUNCTIONS[i][1]);
        }
        symbol = nullptr;
        type = TYPE(LST_ERROR);
        return;
      }
    }
  }

  // Look up the symbol with the requested type
  symbol = lookup_symbol(name, symbol_type);

  if (symbol == nullptr) {                       // no symbol of the right type
    symbol = lookup_symbol(name, SYM_ANY);    // so try the wrong one, so we can have a more descriptive error message in that case.
    if (symbol != nullptr && symbol->get_symbol_type() != symbol_type) {
      ERROR(HERE, E_WRONG_TYPE, name,
            LLScriptSymbol::get_type_name(symbol_type),
            LLScriptSymbol::get_type_name(symbol->get_symbol_type())
      );
    } else {
      /* Name suggestion was here */
      if (type != TYPE(LST_ERROR)) {
        // don't re-warn about undeclared if we already know we're broken.
        ERROR(HERE, E_UNDECLARED, name);
      }
    }

    // Set our symbol to null and type to error since we don't know what they should be.
    symbol = nullptr;
    type = TYPE(LST_ERROR);
    return;
  }

  // Set our type to our symbol's type.
  type = symbol->get_type();
}

LLASTNode *LLASTNode::find_previous_in_scope(std::function<bool(LLASTNode *)> const &checker) {
  LLASTNode *last_node = this;
  for (;;) {
    LLASTNode *node = last_node->get_prev();
    // No previous statements, walk up a level
    if (node == nullptr) {
      node = last_node->get_parent();
      // nothing up here either, bail out
      if (node == nullptr) {
        return nullptr;
      }
    }

    // These don't necessarily create new scopes, check inside them.
    // we need to do this because you can have labels deeply nested
    // in conditionals that don't create new scopes :/. ex:
    // `jump lsl_sucks; llOwnerSay("Surprise!"); if(0) @lsl_sucks;`
    bool check_sub_tree = false;
    switch (node->get_node_sub_type()) {
      case NODE_IF_STATEMENT:
      case NODE_WHILE_STATEMENT:
      case NODE_FOR_STATEMENT:
      case NODE_DO_STATEMENT:
        check_sub_tree = true;
      default:
        break;
    }

    if (check_sub_tree) {
      LLASTNode *found_node = node->find_desc_in_scope(checker);
      // one of our descendants matched the check, return it.
      if (found_node != nullptr)
        return found_node;
    }

    if (checker(node))
      return node;
    last_node = node;
  }
}

// find statements under this node that are still in the same scope
LLASTNode *LLASTNode::find_desc_in_scope(std::function<bool(LLASTNode *)> const &checker) {
  LLASTNode *child = get_children();
  if (checker(this))
    return this;
  while (child) {
    // don't descend into statements that make new scopes
    if (child->get_node_sub_type() != NODE_COMPOUND_STATEMENT) {
      LLASTNode *found_node = child->find_desc_in_scope(checker);
      if (found_node != nullptr)
        return found_node;
    }
    child = child->get_next();
  }
  return nullptr;
}

void LLASTNode::check_symbols() {
  LLASTNode *node;
  if (get_symbol_table() != nullptr)
    get_symbol_table()->check_symbols();

  for (node = get_children(); node; node = node->get_next())
    node->check_symbols();
}


class NodeReferenceUpdatingVisitor : public ASTVisitor {
  public:
    virtual bool visit(LLScriptExpression *node) {
      if (operation_mutates(node->get_operation())) {
        LLASTNode *child = node->get_child(0);
        // add assignment
        if (child->get_node_sub_type() == NODE_LVALUE_EXPRESSION &&
            child->get_child(0)->get_node_type() == NODE_IDENTIFIER) {
          auto *id = (LLScriptIdentifier *) child->get_child(0);
          if (id->get_symbol()) {
            if (id->get_symbol()->get_sub_type() == SYM_BUILTIN) {
              ERROR(IN(node), E_BUILTIN_LVALUE, id->get_symbol()->get_name());
              // make sure we don't muck with the assignment count on a builtin symbol!
              return true;
            }
            id->get_symbol()->add_assignment();
          }
        }
      }
      return true;
    };

    virtual bool visit(LLScriptIdentifier *node) {
      LLASTNode *upper_node = node->get_parent();
      while (upper_node != nullptr && upper_node->get_node_type() != NODE_GLOBAL_STORAGE) {
        // HACK: recursive calls don't count as a reference, won't handle mutual recursion!
        if (upper_node->get_node_type() == NODE_GLOBAL_FUNCTION) {
          auto *ident = (LLScriptIdentifier *) upper_node->get_child(0);
          if (ident != node && (ident)->get_symbol() == node->get_symbol())
            return false;
        }
        upper_node = upper_node->get_parent();
      }
      if (auto *symbol = node->get_symbol())
        symbol->add_reference();
      return false;
    };
};

void LLScriptScript::recalculate_reference_data() {
// get updated mutation / reference counts
  get_symbol_table()->reset_reference_data();
  auto visitor = NodeReferenceUpdatingVisitor();
  visit(&visitor);
}

void LLScriptScript::optimize(const OptimizationContext &ctx) {
  int optimized;
  // make sure we have updated reference data before we start folding any constants
  recalculate_reference_data();
  do {
    TreeSimplifyingVisitor folding_visitor(ctx);
    visit(&folding_visitor);
    optimized = folding_visitor.folded_total;

    // reference data may have changed since we folded constants
    if (optimized)
      recalculate_reference_data();
  } while (optimized);
}


LLScriptConstant *LLScriptIdentifier::get_constant_value() {
  if (symbol && symbol->get_assignments() == 0)
    return symbol->get_constant_value();
  return nullptr;
}


LLScriptConstant *LLScriptGlobalVariable::get_constant_value() {
  // It's not really constant if it gets mutated more than once, is it?
  // note that initialization during declaration doesn't count.
  auto *id = (LLScriptIdentifier *) get_child(0);
  if (id->get_symbol()->get_assignments() == 0) {
    return constant_value;
  }
  return nullptr;
}

LLScriptConstant *LLScriptDeclaration::get_constant_value() {
  auto *id = (LLScriptIdentifier *) get_child(0);
  if (id->get_symbol()->get_assignments() == 0) {
    return constant_value;
  }
  return nullptr;
}

LLScriptConstant *LLScriptExpression::get_constant_value() {
  // replacing `foo = "bar"` with `"bar"` == no
  if (!operation_mutates(operation)) {
    return constant_value;
  }
  return nullptr;
}


LLScriptConstant *LLScriptLValueExpression::get_constant_value() {
  if (is_foldable) {
    // We have to be careful about folding lists
    if (this->type == TYPE(LST_LIST)) {
      LLASTNode *top_foldable = this;
      LLASTNode *current_node = this;

      // Don't fold this in if it's a list expression at the foldable level
      while (current_node != nullptr && current_node->node_allows_folding()) {
        top_foldable = current_node;
        current_node = current_node->get_parent();
      }
      if (top_foldable->get_type() == TYPE(LST_LIST))
        return nullptr;
    }

    return constant_value;
  }
  return nullptr;
}

}
