#ifndef AST_HH
#define AST_HH 1
#include <cassert>
#include <cstdlib> // nullptr
#include <cstdarg> // va_arg
#include <set>
#include "symtab.hh" // symbol table
#include "logger.hh"
#include "allocator.hh"

namespace Tailslide {

// Base node types
enum LSLNodeType {

  NODE_NODE,
  NODE_NULL,
  NODE_AST_NODE_LIST,
  NODE_SCRIPT,
  NODE_GLOBAL_STORAGE,
  NODE_IDENTIFIER,
  NODE_GLOBAL_VARIABLE,
  NODE_CONSTANT,
  NODE_GLOBAL_FUNCTION,
  NODE_FUNCTION_DEC,
  NODE_EVENT_DEC,
  NODE_STATE,
  NODE_EVENT_HANDLER,
  NODE_STATEMENT,
  NODE_EXPRESSION,
  NODE_TYPE,
};

// Node Sub-types
enum LSLNodeSubType {

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
class ASTVisitor;

class LSLASTNode : public TrackableObject {
  public:
    explicit LSLASTNode(ScriptContext *ctx);
    LSLASTNode( ScriptContext *ctx, YYLTYPE *loc, int num, ... )
      : LSLASTNode(ctx) {
      lloc = *loc;
      va_list ap;
      va_start(ap, num);
      add_children( num, ap );
      va_end(ap);
    }

    explicit LSLASTNode( ScriptContext *ctx, int num, ... ) : LSLASTNode(ctx) {
      va_list ap;
      va_start(ap, num);
      add_children( num, ap );
      va_end(ap);
    }

    ~LSLASTNode() override = default;

    void add_children(int num, va_list ap);

    LSLASTNode *get_next() { return next; }
    LSLASTNode *get_prev() { return prev; }
    LSLASTNode *get_children() { return children; }
    LSLASTNode *get_parent() { return parent; }

    LSLASTNode *get_child(int i) {
      LSLASTNode *c = children;
      while (i-- && c)
        c = c->get_next();
      return c;
    }

    // Get the topmost node in the tree
    LSLASTNode *get_root() {
      LSLASTNode *last_node = this;
      LSLASTNode *test_root;
      while ((test_root = last_node->get_parent()) != nullptr)
        last_node = test_root;
      return last_node;
    }

    LSLASTNode *new_null_node();

    /* Set our parent, and make sure all our siblings do too. */
    void set_parent( LSLASTNode *newparent );
    // Add child to end of list.
    void push_child(LSLASTNode *child);
    /* Set our next sibling, and ensure it links back to us. */
    void set_next(LSLASTNode *newnext);
    /* Set our previous sibling, and ensure it links back to us. */
    void set_prev(LSLASTNode *newprev);
    void add_next_sibling(LSLASTNode *sibling);
    void add_prev_sibling(LSLASTNode *sibling);
    /* remove a child from the list of nodes, shifting other children up */
    void remove_child(LSLASTNode *child);
    /* replace a node from the list of children with null, returning it */
    LSLASTNode *take_child(int child_num);

    // replace an arbitrary node with another, setting
    // prev, next and parent as appropriate
    static void replace_node(LSLASTNode *old_node, LSLASTNode *replacement);


    void                set_type(LSLType *_type) { type = _type;   }
    class LSLType *get_type()                    { return type;    }
    LST_TYPE get_itype();


    void mark_static() { static_node = true;}
    bool is_static() {return static_node;}

    void link_symbol_tables (bool unlink=false);

    /// passes                  ///
    // generic visitor functions
    void visit(ASTVisitor *visitor);

    // Convenience methods for common visitor uses
    void collect_symbols();
    void determine_types();
    void propagate_values();
    void check_best_practices();
    void check_symbols(); // look for unused symbols, etc

    /// symbol functions        ///
    virtual LSLSymbol *lookup_symbol( const char *name, LSLSymbolType type );
    void            define_symbol( LSLSymbol *symbol );
    LSLSymbolTable *get_symbol_table() { return symbol_table; }


    YYLTYPE     *get_lloc()     { return &lloc; };
    void set_lloc(YYLTYPE *yylloc) { lloc = *yylloc; };


    // Set whether this node is allowed to be a declaration,
    // usually due to occurring in a conditional statement without a new scope.
    void set_declaration_allowed(bool allowed) { declaration_allowed = allowed; };
    bool get_declaration_allowed() { return declaration_allowed; };

    // bad hacks for figuring out if something is _really_ in scope
    class LSLASTNode* find_previous_in_scope(std::function<bool (class LSLASTNode *)> const &checker);
    class LSLASTNode* find_desc_in_scope(std::function<bool (class LSLASTNode *)> const &checker);


    /// identification          ///
    virtual const char  *get_node_name() { return "node";    };
    virtual LSLNodeType  get_node_type() { return NODE_NODE; };
    virtual LSLNodeSubType get_node_sub_type() { return NODE_NO_SUB_TYPE; }

    /// constants ///
    virtual class LSLConstant  *get_constant_value()    { return constant_value; };
    void set_constant_value(class LSLConstant *cv) {
      if (cv)
        constant_precluded = false;
      constant_value = cv;
    };
    void set_constant_precluded(bool precluded) { constant_precluded = precluded; };
    bool get_constant_precluded() { return constant_precluded; };
    virtual bool node_allows_folding() { return false; };
    bool            is_constant()           { return get_constant_value() != nullptr; };
    virtual LSLSymbol *get_symbol() { return nullptr; }

  protected:
    class LSLType          *type;
    LSLSymbolTable         *symbol_table;
    class LSLConstant      *constant_value;
    bool                        constant_precluded = false;

  private:
    YYLTYPE                      lloc {0};

    LSLASTNode *children;
    LSLASTNode *parent;
    LSLASTNode *next;
    LSLASTNode *prev;

  protected:
    bool                        declaration_allowed;
    bool                        static_node = false;
};

class LSLASTNullNode : public LSLASTNode {
  public:
    explicit LSLASTNullNode(ScriptContext *ctx): LSLASTNode(ctx) {};
    virtual const char *get_node_name() { return "null"; };
    virtual LSLNodeType get_node_type() { return NODE_NULL; };
};

class LSLASTNodeList : public LSLASTNode {
  public:
    explicit LSLASTNodeList(ScriptContext *ctx) : LSLASTNode(ctx, 0) {};
    LSLASTNodeList(ScriptContext *ctx, class LSLASTNode *nodes ) : LSLASTNode(ctx, 1, nodes) {};
    virtual const char *get_node_name() { return "ast node list"; }
    virtual LSLNodeType get_node_type() { return NODE_AST_NODE_LIST; };
};

}

#endif /* not AST_HH */
