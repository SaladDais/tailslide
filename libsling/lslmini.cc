#include <cstdio>
#include <cstring>
#include <cassert>

#include "lslmini.hh"
#include "logger.hh"
#include "ast.hh"
#include "passes/type_checking.hh"


extern FILE *yyin;
//extern int yynerrs;
int yynwarns = 0;                // not defined by flex but named for consistency

//int yyerror(const char *fmt, ... );
//int yywarning(const char *fmt, ...);

namespace Sling {

thread_local YYLTYPE LLASTNode::glloc = {0, 0, 0, 0};

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

  // Make sure the declaration happened *before* our usage of it,
  // LSL doesn't hoist declarations!
  if (sym != nullptr && start_node != nullptr && sym->get_symbol_type() == SYM_VARIABLE &&
      (sym->get_sub_type() == SYM_LOCAL || sym->get_sub_type() == SYM_GLOBAL)) {
    bool is_local = (sym->get_sub_type() == SYM_LOCAL);
    LLASTNode *decl_node = sym->get_var_decl();
    LLASTNode *ref_ancestor = start_node;

    assert(decl_node != nullptr);

    /* handle `integer foo = foo + 1;` */
    while ((ref_ancestor = ref_ancestor->get_parent()) != nullptr) {
      if (ref_ancestor == decl_node) {
        sym = nullptr;
      }
    }
    if (sym != nullptr) {
      /* handle `llOwnerSay(foo); string foo;` */
      LLASTNode *found_node = start_node->find_previous_in_scope(
              [is_local, decl_node, this](LLASTNode *to_check) -> bool {
                  // `foo(){llOwnerSay(bar);} string bar="baz";` is fine
                  if (!is_local && to_check->get_node_type() == NODE_GLOBAL_FUNCTION) {
                    return true;
                  }

                  // stop searching once we hit the declaration or
                  // the end of our scope.
                  return to_check == decl_node || to_check == this;
              }
      );
      if (found_node == nullptr || found_node == this) {
        sym = nullptr;
      }
    }
  }

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
          if (shadow->get_sub_type() == SYM_BUILTIN) {
            // you're never allowed to shadow event names
            if (shadow->get_symbol_type() == SYM_EVENT)
              ERROR(IN(symbol), E_DUPLICATE_DECLARATION_EVENT, symbol->get_name());
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
  LLASTNode *child = get_children();
  define_symbols();
  while (child) {
    child->collect_symbols();
    child = child->get_next();
  }
}

void LLASTNode::define_symbols() {
  /* nothing */
}

void LLScriptDeclaration::define_symbols() {
  auto *identifier = (LLScriptIdentifier *) get_children();
  identifier->set_symbol(gAllocationManager->new_tracked<LLScriptSymbol>(
    identifier->get_name(), identifier->get_type(), SYM_VARIABLE, SYM_LOCAL, get_lloc(), nullptr, this
  ));
  define_symbol(identifier->get_symbol());

  if (!declaration_allowed) {
    ERROR(HERE, E_DECLARATION_INVALID_HERE, identifier->get_symbol()->get_name());
  }
}

void LLScriptGlobalVariable::define_symbols() {
  auto *identifier = (LLScriptIdentifier *) get_children();
  identifier->set_symbol(gAllocationManager->new_tracked<LLScriptSymbol>(
    identifier->get_name(), identifier->get_type(), SYM_VARIABLE, SYM_GLOBAL, get_lloc(), nullptr, get_parent()
  ));
  define_symbol(identifier->get_symbol());
}

void LLScriptScript::define_symbols() {
}

void LLScriptState::define_symbols() {
  LLASTNode *node = get_children();
  LLScriptIdentifier *identifier;

  if (node->get_node_type() == NODE_NULL) // null identifier = default state, nothing to define
    return;

  identifier = (LLScriptIdentifier *) node;
  identifier->set_symbol(gAllocationManager->new_tracked<LLScriptSymbol>(
    identifier->get_name(), identifier->get_type(), SYM_STATE, SYM_GLOBAL, identifier->get_lloc()
  ));
  define_symbol(identifier->get_symbol());
}

void LLScriptGlobalFunction::define_symbols() {
  auto *identifier = (LLScriptIdentifier *) get_child(0);

  // define function in parent scope since functions have their own scope
  identifier->set_symbol(gAllocationManager->new_tracked<LLScriptSymbol>(
    identifier->get_name(), identifier->get_type(), SYM_FUNCTION, SYM_GLOBAL, get_lloc(), (LLScriptFunctionDec *) get_child(1)
  ));
  get_parent()->define_symbol(identifier->get_symbol());
}

void LLScriptFunctionDec::define_symbols() {
  LLScriptIdentifier *identifier;
  LLASTNode *node = get_children();
  while (node) {
    identifier = (LLScriptIdentifier *) node;
    identifier->set_symbol(gAllocationManager->new_tracked<LLScriptSymbol>(
      identifier->get_name(), identifier->get_type(), SYM_VARIABLE, SYM_FUNCTION_PARAMETER, node->get_lloc()
    ));
    define_symbol(identifier->get_symbol());
    node = node->get_next();
  }
}

void LLScriptEventDec::define_symbols() {
  LLScriptIdentifier *identifier;
  LLASTNode *node = get_children();
  while (node) {
    identifier = (LLScriptIdentifier *) node;
    identifier->set_symbol(gAllocationManager->new_tracked<LLScriptSymbol>(
      identifier->get_name(), identifier->get_type(), SYM_VARIABLE, SYM_EVENT_PARAMETER, node->get_lloc()
    ));
    define_symbol(identifier->get_symbol());
    node = node->get_next();
  }
}

void LLScriptLabel::define_symbols() {
  auto *identifier = (LLScriptIdentifier *) get_children();
  identifier->set_symbol(gAllocationManager->new_tracked<LLScriptSymbol>(
    identifier->get_name(), identifier->get_type(), SYM_LABEL, SYM_LOCAL, get_lloc()
  ));
  define_symbol(identifier->get_symbol());
}

// walk tree post-order and propogate types
void LLASTNode::propogate_types() {
  TypeCheckVisitor visitor;
  visit(&visitor);
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
      ERROR(HERE, E_UNDECLARED, name);
    }

    // Set our symbol to null and type to error since we don't know what they should be.
    symbol = nullptr;
    type = TYPE(LST_ERROR);
    return;
  }

  /// If we're requesting a member, like var.x or var.y
  if (member != nullptr) {

    // all members must be single letters
    if (member[1] != 0) {
      ERROR(HERE, E_INVALID_MEMBER, name, member);
      type = TYPE(LST_ERROR);
      return;
    }

    /// Make sure it's a variable
    if (symbol_type != SYM_VARIABLE) {
      ERROR(HERE, E_MEMBER_NOT_VARIABLE, name, member, LLScriptSymbol::get_type_name(symbol_type));
      symbol = nullptr;
      type = TYPE(LST_ERROR);
      return;
    }

    // ZERO_VECTOR.x is not valid, because it's really `<0,0,0>.x` which is not allowed!
    if (symbol->get_sub_type() == SYM_BUILTIN) {
      ERROR(HERE, E_INVALID_MEMBER, name, member);
      type = TYPE(LST_ERROR);
      return;
    }

    // Make sure it's a vector or quaternion. TODO: is there a better way to do this?
    switch (symbol->get_type()->get_itype()) {
      case LST_QUATERNION:
        if (member[0] == 's') {
          type = TYPE(LST_FLOATINGPOINT);
          break;
        }
        // FALL THROUGH
      case LST_VECTOR:
        switch (member[0]) {
          case 'x':
          case 'y':
          case 'z':
            type = TYPE(LST_FLOATINGPOINT);
            break;
          default:
            ERROR(HERE, E_INVALID_MEMBER, name, member);
            type = TYPE(LST_ERROR);
            break;
        }
        break;
      default:
        ERROR(HERE, E_MEMBER_WRONG_TYPE, name, member);
        type = TYPE(LST_ERROR);
        break;
    }
  } else {
    // Set our type to our symbol's type.
    type = symbol->get_type();
  }
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

bool ASTVisitor::visit_specific(LLASTNode *node) {
  switch(node->get_node_type()) {
    case NODE_NODE:
      return visit(node);
    case NODE_NULL:
      return visit((LLASTNullNode *)node);
    case NODE_SCRIPT:
      return visit((LLScriptScript *)node);
    case NODE_GLOBAL_STORAGE:
      return visit((LLScriptGlobalStorage *)node);
    case NODE_GLOBAL_FUNCTION:
      return visit((LLScriptGlobalFunction *)node);
    case NODE_GLOBAL_VARIABLE:
      return visit((LLScriptGlobalVariable *)node);
    case NODE_IDENTIFIER:
      return visit((LLScriptIdentifier *)node);
    case NODE_SIMPLE_ASSIGNABLE:
      return visit((LLScriptSimpleAssignable *)node);
    case NODE_CONSTANT: {
      switch(node->get_node_sub_type()) {
        case NODE_INTEGER_CONSTANT:
          return visit((LLScriptIntegerConstant *)node);
        case NODE_FLOAT_CONSTANT:
          return visit((LLScriptFloatConstant *)node);
        case NODE_STRING_CONSTANT:
          return visit((LLScriptStringConstant *)node);
        case NODE_VECTOR_CONSTANT:
          return visit((LLScriptVectorConstant *)node);
        case NODE_QUATERNION_CONSTANT:
          return visit((LLScriptQuaternionConstant *)node);
        case NODE_LIST_CONSTANT:
          return visit((LLScriptListConstant *)node);
        default:
          return visit((LLScriptConstant *)node);
      }
    }
    case NODE_FUNCTION_DEC:
      return visit((LLScriptFunctionDec *)node);
    case NODE_EVENT_DEC:
      return visit((LLScriptEventDec *)node);
    case NODE_FOR_EXPRESSION_LIST:
      return visit((LLScriptForExpressionList *)node);
    case NODE_STATE:
      return visit((LLScriptState *)node);
    case NODE_EVENT_HANDLER:
      return visit((LLScriptEventHandler *)node);
    case NODE_STATEMENT:
      switch(node->get_node_sub_type()) {
        case NODE_COMPOUND_STATEMENT:
          return visit((LLScriptCompoundStatement *)node);
        case NODE_RETURN_STATEMENT:
          return visit((LLScriptReturnStatement *)node);
        case NODE_LABEL:
          return visit((LLScriptLabel *)node);
        case NODE_JUMP_STATEMENT:
          return visit((LLScriptJumpStatement *)node);
        case NODE_IF_STATEMENT:
          return visit((LLScriptIfStatement *)node);
        case NODE_FOR_STATEMENT:
          return visit((LLScriptForStatement *)node);
        case NODE_DO_STATEMENT:
          return visit((LLScriptDoStatement *)node);
        case NODE_WHILE_STATEMENT:
          return visit((LLScriptWhileStatement *)node);
        case NODE_DECLARATION:
          return visit((LLScriptDeclaration *)node);
        case NODE_STATE_STATEMENT:
          return visit((LLScriptStateStatement *)node);
        default:
          return visit((LLScriptStatement *)node);
      }
    case NODE_EXPRESSION:
      switch(node->get_node_sub_type()) {
        case NODE_TYPECAST_EXPRESSION:
          return visit((LLScriptTypecastExpression *)node);
        case NODE_PRINT_EXPRESSION:
          return visit((LLScriptPrintExpression *)node);
        case NODE_FUNCTION_EXPRESSION:
          return visit((LLScriptFunctionExpression *)node);
        case NODE_VECTOR_EXPRESSION:
          return visit((LLScriptVectorExpression *)node);
        case NODE_QUATERNION_EXPRESSION:
          return visit((LLScriptQuaternionExpression *)node);
        case NODE_LIST_EXPRESSION:
          return visit((LLScriptListExpression *)node);
        case NODE_LVALUE_EXPRESSION:
          return visit((LLScriptLValueExpression *)node);
        case NODE_PARENTHESIS_EXPRESSION:
          return visit((LLScriptParenthesisExpression *)node);
        case NODE_BINARY_EXPRESSION:
          return visit((LLScriptBinaryExpression *)node);
        case NODE_UNARY_EXPRESSION:
          return visit((LLScriptUnaryExpression *)node);
        case NODE_CONSTANT_EXPRESSION:
          return visit((LLScriptConstantExpression *)node);
        default:
          return visit((LLScriptExpression *)node);
      }
    case NODE_TYPE:
      return visit((LLScriptType *)node);
  }
  return visit(node);
}

void ASTVisitor::visit_children(LLASTNode *node) {
  LLASTNode *child = node->get_children();

  while (child != nullptr) {
    LLASTNode *next = child->get_next();
    assert(child != next);
    assert(child != node);
    child->visit(this);
    child = next;
  }

  /* Stupid list constants with their stupid hidden trees */
  if (node->get_node_sub_type() == NODE_LIST_CONSTANT) {
    LLASTNode *list_child = ((LLScriptListConstant *) node)->get_value();
    while (list_child != nullptr) {
      LLASTNode *next = list_child->get_next();
      assert(list_child != next);
      assert(list_child != node);
      list_child->visit(this);
      list_child = next;
    }
  }
}

class TreeSimplifyingVisitor: public ASTVisitor {
public:
  explicit TreeSimplifyingVisitor(const OptimizationContext &ctx): ctx(ctx) {};
  OptimizationContext ctx;
  int folded_total = 0;

  virtual bool visit(LLScriptDeclaration* node) {
    if (!ctx.prune_unused_locals)
      return true;

    auto *id = (LLScriptIdentifier *) (node->get_child(0));
    auto *sym = id->get_symbol();
    if (!sym || sym->get_references() != 1 || sym->get_assignments() != 0)
      return true;
    LLASTNode *rvalue = node->get_child(1);
    // rvalue can't be reduced to a constant, don't know that we don't need
    // the side-effects of evaluating the expression.
    if(rvalue && !rvalue->get_constant_value())
      return true;

    ++folded_total;
    LLASTNode *ancestor = node;
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
  };

  virtual bool visit(LLScriptGlobalStorage* node) {
    // GlobalStorages either contain a single var or a single function.
    LLASTNode *contained = (node->get_child(0)->get_node_type() != NODE_NULL) ? node->get_child(0) : node->get_child(1);
    // and they both keep their identifier in the first child!
    auto *id = (LLScriptIdentifier *) (contained->get_child(0));
    assert(id != nullptr && id->get_node_type() == NODE_IDENTIFIER);

    LLNodeType node_type = contained->get_node_type();
    auto *sym = id->get_symbol();

    if (((node_type == NODE_GLOBAL_FUNCTION && ctx.prune_unused_functions) ||
         (node_type == NODE_GLOBAL_VARIABLE && ctx.prune_unused_globals))
        && sym->get_references() == 1) {
      ++folded_total;
      // these reside in the global scope, look for the root symbol table and the entry
      LLASTNode *script = node->get_root();
      script->get_symbol_table()->remove(sym);
      // remove the node itself
      node->get_parent()->remove_child(node);
      return false;
    }
    return true;
  };

  virtual bool visit(LLScriptExpression* node) {
    if (!ctx.fold_constants)
      return true;

    LLScriptConstant *cv = node->get_constant_value();
    if(!cv)
      return true;
    auto c_type = node->get_type()->get_itype();
    // this expression results in a list, don't fold the result in.
    if (c_type == LST_LIST)
      return true;
    // this expression may result in a new entry in the string constant pool,
    // and we're not allowed to create new ones, don't fold.
    // TODO: Might be worth checking for cases where we're just splicing together two
    //  strings that aren't referenced anywhere else.
    if (!ctx.may_create_new_strs && c_type == LST_STRING)
      return true;

    // We're going to change its parent / sibling connections,
    // so we need a copy.
    auto *new_expr = gAllocationManager->new_tracked<LLScriptConstantExpression>(cv);
    new_expr->set_lloc(node->get_lloc());
    LLASTNode::replace_node(node, new_expr);
    ++folded_total;

    return false;
  };

  virtual bool visit(LLScriptSimpleAssignable *node) {
    if (!ctx.fold_constants)
      return true;
    LLASTNode *child = node->get_child(0);
    if (child && child->get_node_type() == NODE_IDENTIFIER) {
      auto *id = (LLScriptIdentifier *)child;
      auto *sym = id->get_symbol();
      // Is this a builtin constant? Don't bother replacing it.
      // These references are usually "free" given that they're
      // lexer tokens in SL proper.
      if (!sym || sym->get_sub_type() == SYM_BUILTIN)
        return false;
      if (sym->get_type()->get_itype() == LST_LIST)
        return false;
      LLScriptConstant *cv = child->get_constant_value();
      if (cv) {
        cv = cv->copy();
        cv->set_lloc(child->get_lloc());
        LLASTNode::replace_node(child, cv);
        ++folded_total;
      }
    }
    return true;
  }

  virtual bool visit(LLScriptLValueExpression *node) {
    if (!ctx.fold_constants)
      return true;
    LLASTNode *child = node->get_child(0);
    if (child && child->get_node_type() == NODE_IDENTIFIER) {
      auto *id = (LLScriptIdentifier *)child;
      auto *sym = id->get_symbol();
      // Is this a builtin constant? Don't bother replacing it.
      // These references are usually "free" given that they're
      // lexer tokens in SL proper.
      if (!sym || sym->get_sub_type() == SYM_BUILTIN)
        return false;
      // no inlining references to lists, they don't get put in
      // a constant pool like strings or keys!
      if (sym->get_type()->get_itype() == LST_LIST)
        return false;
      LLScriptConstant *cv = child->get_constant_value();
      if (cv) {
        auto *new_expr = gAllocationManager->new_tracked<LLScriptConstantExpression>(cv);
        new_expr->set_lloc(node->get_lloc());
        LLASTNode::replace_node(
          node,
          new_expr
        );
        ++folded_total;
        return false;
      }
    }
    return true;
  };

  virtual bool visit(LLScriptConstantExpression *node) {
    // Don't touch these at all, they can't be simplified any more!
    return false;
  };
};

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

void LLASTNode::check_symbols() {
  LLASTNode *node;
  if (get_symbol_table() != nullptr)
    get_symbol_table()->check_symbols();

  for (node = get_children(); node; node = node->get_next())
    node->check_symbols();
}

bool TreePrintingVisitor::visit(LLASTNode* node) {
  LLASTNode *child = node->get_children();
  char buf[256];

  auto lloc = node->get_lloc();
  auto type = node->get_type();
  auto constant_value = node->get_constant_value();
  int i;
  for (i = 0; i < walklevel; i++) {
    stream << "  ";
  }
  stream << node->get_node_name()
      << " [" << (type ? type->get_node_name() : "") << "] "
      << "(cv=" << (constant_value ? constant_value->get_node_name() : "") << ") "
      << '(' << lloc->first_line << ',' << lloc->first_column << ")\n";

  ++walklevel;
  visit_children(node);
  --walklevel;
  return false;
}

}
