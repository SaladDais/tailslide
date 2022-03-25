#ifndef AST_HH
#define AST_HH 1
#include <cassert>
#include <cstdlib> // NULL
#include <cstdarg> // va_arg
#include <set>
#include "tree_base.hh"
#include "symtab.hh" // symbol table
#include "logger.hh"
#include "allocator.hh"

namespace Sling {

// Base node types
enum LLNodeType {

  NODE_NODE,
  NODE_NULL,
  NODE_SCRIPT,
  NODE_GLOBAL_STORAGE,
  NODE_IDENTIFIER,
  NODE_GLOBAL_VARIABLE,
  NODE_SIMPLE_ASSIGNABLE,
  NODE_CONSTANT,
  NODE_GLOBAL_FUNCTION,
  NODE_FUNCTION_DEC,
  NODE_EVENT_DEC,
  NODE_FOR_EXPRESSION_LIST,
  NODE_STATE,
  NODE_EVENT_HANDLER,
  NODE_STATEMENT,
  NODE_EXPRESSION,
  NODE_TYPE,
};

// Node Sub-types
enum LLNodeSubType {

  NODE_NO_SUB_TYPE,

  NODE_INTEGER_CONSTANT,
  NODE_FLOAT_CONSTANT,
  NODE_STRING_CONSTANT,
  NODE_VECTOR_CONSTANT,
  NODE_QUATERNION_CONSTANT,
  NODE_LIST_CONSTANT,

  NODE_COMPOUND_STATEMENT,
  NODE_RETURN_STATEMENT,
  NODE_LABEL,
  NODE_JUMP_STATEMENT,
  NODE_IF_STATEMENT,
  NODE_FOR_STATEMENT,
  NODE_DO_STATEMENT,
  NODE_WHILE_STATEMENT,
  NODE_DECLARATION,
  NODE_STATE_STATEMENT,

  NODE_TYPECAST_EXPRESSION,
  NODE_PRINT_EXPRESSION,
  NODE_FUNCTION_EXPRESSION,
  NODE_VECTOR_EXPRESSION,
  NODE_QUATERNION_EXPRESSION,
  NODE_LIST_EXPRESSION,
  NODE_LVALUE_EXPRESSION,
  NODE_BINARY_EXPRESSION,
  NODE_UNARY_EXPRESSION,
  NODE_PARENTHESIS_EXPRESSION,
  NODE_CONSTANT_EXPRESSION,
};

class OptimizationContext;
class LLASTNullNode;
class ASTVisitor;

class LLASTNode : public ATreeBase<LLASTNode, class LLASTNullNode> {
  public:
    LLASTNode() : ATreeBase(), type(NULL), symbol_table(NULL), constant_value(NULL), lloc(glloc), declaration_allowed(true) {};
    LLASTNode( YYLTYPE *lloc, int num, ... )
      : ATreeBase(), type(NULL), symbol_table(NULL), constant_value(NULL), lloc(*lloc), declaration_allowed(true) {
      va_list ap;
      va_start(ap, num);
      add_children( num, ap );
      va_end(ap);
    }

    LLASTNode( int num, ... ) : ATreeBase(), type(NULL), symbol_table(NULL), constant_value(NULL), lloc(glloc), declaration_allowed(true) {
      va_list ap;
      va_start(ap, num);
      add_children( num, ap );
      va_end(ap);
    }

    void                set_type(LLScriptType *_type) { type = _type;   }
    class LLScriptType *get_type()                    { return type;    }


    /* Set our parent, and make sure all our siblings do too. */
    virtual void    set_parent( LLASTNode *newparent ) {
      // walk tree, (un)registering descendants' symbol tables with
      // the root table
      assert(newparent != this);
      if (static_node) {
        if (!newparent) return;
        assert(0);
      }
      link_symbol_tables(true);
      ATreeBase::set_parent(newparent);
      if (newparent != NULL)
        link_symbol_tables();
    }

    void mark_static() { static_node = true;}
    bool is_static() {return static_node;}

    void link_symbol_tables (bool unlink=false);

    /// passes                  ///
    // generic visitor functions
    void visit(ASTVisitor *visitor);

    // collect symbols from function/state/variable declarations
    void collect_symbols();
    virtual void define_symbols();

    // propogate types   TODO: rename to propogate_and_check_type / determine_and_check_type ?
    virtual void propogate_types();

    // propogate const values     TODO: come up with a better name?
    virtual void propogate_values();

    // final pre walk checks    TODO: come up with a better name?
    void determine_reachability();

    /// symbol functions        ///
    virtual LLScriptSymbol *lookup_symbol( const char *name, LLSymbolType type = SYM_ANY, LLASTNode *start_node = NULL );
    void            define_symbol( LLScriptSymbol *symbol, bool check_existing = true );
    void            check_symbols(); // look for unused symbols, etc
    LLScriptSymbolTable *get_symbol_table() { return symbol_table; }


    YYLTYPE     *get_lloc()     { return &lloc; };
    static void set_glloc(YYLTYPE *yylloc) { glloc = *yylloc; };
    void set_lloc(YYLTYPE *yylloc) { lloc = *yylloc; };


    // Set whether this node is allowed to be a declaration,
    // usually due to occurring in a conditional statement without a new scope.
    void set_declaration_allowed(bool allowed) { declaration_allowed = allowed; };

    // bad hacks for figuring out if something is _really_ in scope
    class LLASTNode* find_previous_in_scope(std::function<bool (class LLASTNode *)> const &checker);
    class LLASTNode* find_desc_in_scope(std::function<bool (class LLASTNode *)> const &checker);


    /// identification          ///
    virtual const char  *get_node_name() { return "node";    };
    virtual LLNodeType  get_node_type() { return NODE_NODE; };
    virtual LLNodeSubType get_node_sub_type() { return NODE_NO_SUB_TYPE; }

    /// constants ///
    virtual class LLScriptConstant  *get_constant_value()    { return constant_value; };
    void set_constant_value(class LLScriptConstant *cv) {constant_value = cv;};
    virtual bool node_allows_folding() { return false; };
    bool            is_constant()           { return get_constant_value() != NULL; };

    static LLASTNullNode * new_null_node();

  protected:
    class LLScriptType          *type;
    LLScriptSymbolTable         *symbol_table;
    class LLScriptConstant      *constant_value;

  private:
    YYLTYPE                      lloc;
    thread_local static YYLTYPE               glloc;

  protected:
    bool                        declaration_allowed;
    bool                        static_node = false;
};

class LLASTNullNode : public LLASTNode {
  public:
    LLASTNullNode();
    virtual const char *get_node_name() { return "null"; };
    virtual LLNodeType get_node_type() { return NODE_NULL; };
};

}

#endif /* not AST_HH */
