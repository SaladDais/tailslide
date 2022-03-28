#ifndef _LSLMINI_HH
#define _LSLMINI_HH 1

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cctype> // isprint()
#include <cstdarg>
#include <functional>
#include <sstream>

#include "lslmini.tab.hh"
#include "symtab.hh"
#include "ast.hh"
#include "types.hh"

namespace Sling {

typedef int   S32;
typedef float F32;

#define BUFFER_SIZE     1024
#define UUID_STR_LENGTH 36
#define MAX_NODES       3

extern thread_local class LLScriptScript *gScript;
extern thread_local bool gFatalError;

class LLVector: public LLTrackableObject {
  public:
    LLVector(float x, float y, float z) : x(x), y(y), z(z) {};
    float x, y, z;

    bool operator==(const class LLVector &other) {
      return (x == other.x) && (y == other.y) && (z == other.z);
    };
    bool operator!=(const class LLVector &other) {
      return !(*this == other);
    };
};

class LLQuaternion: public LLTrackableObject {
  public:
    LLQuaternion(float x, float y, float z, float s) : x(x), y(y), z(z), s(s) {};
    float x, y, z, s;

    bool operator==(const class LLQuaternion &other) {
      return (x == other.x) && (y == other.y) && (z == other.z) && (s == other.s);
    };
    bool operator!=(const class LLQuaternion &other) {
      return !(*this == other);
    };
};


class LLScriptScript : public LLASTNode {
  public:
    LLScriptScript( class LLScriptGlobalStorage *globals, class LLScriptState *states )
      : LLASTNode( 2, globals, states ) {
        symbol_table = gAllocationManager->new_tracked<LLScriptSymbolTable>();
    };

    virtual const char *get_node_name() { return "script"; };
    virtual LLNodeType get_node_type() { return NODE_SCRIPT; };
    virtual LLScriptSymbol *lookup_symbol(const char *name, LLSymbolType sym_type, LLASTNode *start_node);

    void optimize(const OptimizationContext &ctx);
    void recalculate_reference_data();
    void validate_globals(bool sl_strict);
};

class LLScriptGlobalStorage : public LLASTNode {
  public:
    LLScriptGlobalStorage( class LLScriptGlobalVariable *variables, class LLScriptGlobalFunction *functions )
      : LLASTNode( 2, variables, functions ) {};
    virtual const char *get_node_name() { return "global storage"; }
    virtual LLNodeType get_node_type() { return NODE_GLOBAL_STORAGE; };
};

class LLScriptIdentifier : public LLASTNode {
  public:
    LLScriptIdentifier( const char *name ) : LLASTNode(0), symbol(NULL), name(name) {};
    LLScriptIdentifier( class LLScriptType *_type, const char *name ) : LLASTNode(0), symbol(NULL), name(name) { type = _type; };
    LLScriptIdentifier( class LLScriptType *_type, const char *name, YYLTYPE *lloc ) : LLASTNode( lloc, 0), symbol(NULL), name(name) { type = _type; };
    LLScriptIdentifier( LLScriptIdentifier *other ) : LLASTNode(0), symbol(NULL), name(other->get_name()) {};

    const char    *get_name() { return name; }

    void resolve_symbol(LLSymbolType symbol_type);
    void set_symbol( LLScriptSymbol *_symbol ) { symbol = _symbol; };
    LLScriptSymbol *get_symbol() { return symbol; };

    virtual const char *get_node_name() {
      static thread_local char buf[256];
      snprintf(buf, 256, "identifier \"%s\"", name);
      return buf;
    }
    virtual LLNodeType get_node_type() { return NODE_IDENTIFIER; };
    virtual class LLScriptConstant *get_constant_value();

  private:
    LLScriptSymbol                  *symbol;
    const char                      *name;
};

class LLScriptGlobalVariable : public LLASTNode {
  public:
    LLScriptGlobalVariable( class LLScriptIdentifier *identifier, class LLScriptExpression *value )
      : LLASTNode(2, identifier, value) { DEBUG( LOG_DEBUG_SPAM, NULL, "made a global var\n"); };
    virtual const char *get_node_name() { return "global var"; }
    virtual LLNodeType get_node_type() { return NODE_GLOBAL_VARIABLE; };

    virtual LLScriptConstant* get_constant_value();

};


class LLScriptConstant : public LLASTNode {
  public:
    LLScriptConstant() : LLASTNode(0) { constant_value = this; }
    virtual const char *get_node_name() { return "unknown constant"; }
    virtual LLNodeType get_node_type() { return NODE_CONSTANT; };
    virtual LLScriptConstant *operation(int op, LLScriptConstant *other_const, YYLTYPE *lloc) = 0;
    // make a shallow copy of the constant
    virtual LLScriptConstant* copy() = 0;
    virtual LLScriptConstant* cast(LST_TYPE to_type) { return nullptr; };
};

/////////////////////////////////////////////////////
// Integer Constant

class LLScriptIntegerConstant : public LLScriptConstant {
  public:
    LLScriptIntegerConstant( int v ) : LLScriptConstant(), value(v) { type = TYPE(LST_INTEGER); }

    virtual const char *get_node_name() {
      static thread_local char buf[256];
      snprintf(buf, 256, "integer constant: %d", value);
      return buf;
    }

    virtual LLNodeSubType get_node_sub_type() { return NODE_INTEGER_CONSTANT; }

    int get_value() { return value; }
    void set_value(int val) { value = val; }
    virtual LLScriptConstant *operation(int op, LLScriptConstant *other_const, YYLTYPE *lloc);
    virtual LLScriptConstant* copy() {
      auto* new_const = gAllocationManager->new_tracked<LLScriptIntegerConstant>(value);
      new_const->constant_value = new_const;
      return new_const;
    };
    LLScriptConstant *cast(LST_TYPE to_type);

  private:
    int value;
};


/////////////////////////////////////////////////////
// Float Constant

class LLScriptFloatConstant : public LLScriptConstant {
  public:
    LLScriptFloatConstant( float v ) : LLScriptConstant(), value(v) { type = TYPE(LST_FLOATINGPOINT); }

    virtual const char *get_node_name() {
      static thread_local char buf[256];
      snprintf(buf, 256, "float constant: %f", value);
      return buf;
    }

    virtual LLNodeSubType get_node_sub_type() { return NODE_FLOAT_CONSTANT; }

    float get_value() { return value; }
    void set_value(float val) { value = val; }
    virtual LLScriptConstant *operation(int op, LLScriptConstant *other_const, YYLTYPE *lloc);
    virtual LLScriptConstant* copy() {
      auto* new_const = gAllocationManager->new_tracked<LLScriptFloatConstant>(value);
      new_const->constant_value = new_const;
      return new_const;
    };
    virtual LLScriptConstant *cast (LST_TYPE to_type);

  private:
    float value;
};


/////////////////////////////////////////////////////
// String Constant

class LLScriptStringConstant : public LLScriptConstant {
  public:
    LLScriptStringConstant( char *v ) : LLScriptConstant(), value(v) { type = TYPE(LST_STRING); }

    virtual const char *get_node_name() {
      static thread_local char buf[256];
      snprintf(buf, 256, "string constant: `%s'", value);
      return buf;
    }

    virtual LLNodeSubType get_node_sub_type() { return NODE_STRING_CONSTANT; }

    char *get_value() { return value; }
    void set_value(char *val) { value = val; }
    virtual LLScriptConstant *operation(int op, LLScriptConstant *other_const, YYLTYPE *lloc);
    virtual LLScriptConstant* copy() {
      auto* new_const = gAllocationManager->new_tracked<LLScriptStringConstant>(value);
      new_const->constant_value = new_const;
      return new_const;
    };
    virtual LLScriptConstant *cast(LST_TYPE to_type);

  private:
    char *value;
};


/////////////////////////////////////////////////////
// List Constant

class LLScriptListConstant : public LLScriptConstant {
  public:
    LLScriptListConstant( class LLScriptConstant *v ) : LLScriptConstant() {
      type = TYPE(LST_LIST);
      // so we can do symbol resolution inside the list constant
      push_child(v);
    }

    virtual const char *get_node_name() {
      static thread_local char buf[256];
      int len = 0;
      if (get_children())
        len = get_length();
      snprintf(buf, 256, "list constant: %d entries", len);
      return buf;
    }

    virtual LLNodeSubType get_node_sub_type() { return NODE_LIST_CONSTANT; }

    class LLScriptConstant *get_value() { return (LLScriptConstant*)get_children(); }
    void set_value(class LLScriptConstant *val) {  }

    int get_length() {
      LLASTNode *node = get_children();
      int i = 0;
      for ( ; node; node = node->get_next() )
        ++i;
      return i;
    }

    virtual LLScriptConstant *operation(int op, LLScriptConstant *other_const, YYLTYPE *lloc);
    virtual LLScriptConstant* copy() {
      auto* new_const = gAllocationManager->new_tracked<LLScriptListConstant>(*this);
      new_const->constant_value = new_const;
      return new_const;
    };
};

/////////////////////////////////////////////////////
// Vector Constant

class LLScriptVectorConstant : public LLScriptConstant {
  public:
    LLScriptVectorConstant( float v1, float v2, float v3 ) {
      value = gAllocationManager->new_tracked<LLVector>( v1, v2, v3 );
      type = TYPE(LST_VECTOR);
    };

    virtual const char *get_node_name() {
      static thread_local char buf[256];
      if ( value )
        snprintf(buf, 256, "vector constant: <%g, %g, %g>", value->x, value->y, value->z);
      else
        snprintf(buf, 256, "vector constant: unknown value?" );
      return buf;
    }

    virtual LLNodeSubType get_node_sub_type() { return NODE_VECTOR_CONSTANT; }

    LLVector *get_value() { return value; }
    void set_value(LLVector *val) { value = val; }

    virtual LLScriptConstant *operation(int op, LLScriptConstant *other_const, YYLTYPE *lloc);
    virtual LLScriptConstant* copy() {
      auto* new_const = gAllocationManager->new_tracked<LLScriptVectorConstant>(value->x, value->y, value->z);
      new_const->constant_value = new_const;
      return new_const;
    };

  private:
    LLVector *value;
};


/////////////////////////////////////////////////////
// Quaternion Constant

class LLScriptQuaternionConstant : public LLScriptConstant {
  public:
    LLScriptQuaternionConstant( float v1, float v2, float v3, float v4 ) {
      value = gAllocationManager->new_tracked<LLQuaternion>( v1, v2, v3, v4 );
      type = TYPE(LST_QUATERNION);
    };

    virtual const char *get_node_name() {
      static thread_local char buf[256];
      if (value)
        snprintf(buf, 256, "quaternion constant: <%g, %g, %g, %g>", value->x, value->y, value->z, value->s);
      else
        snprintf(buf, 256, "quaternion constant: unknown value?" );
      return buf;
    }

    virtual LLNodeSubType get_node_sub_type() { return NODE_QUATERNION_CONSTANT; }

    LLQuaternion *get_value() { return value; }
    void set_value(class LLQuaternion *val) { value = val; }

    virtual LLScriptConstant *operation(int op, LLScriptConstant *other_const, YYLTYPE *lloc);

    virtual LLScriptConstant* copy() {
      auto* new_const = gAllocationManager->new_tracked<LLScriptQuaternionConstant>(value->x, value->y, value->z, value->s);
      new_const->constant_value = new_const;
      return new_const;
    };

  private:
    LLQuaternion *value;
};


class LLScriptGlobalFunction : public LLASTNode {
  public:
    LLScriptGlobalFunction( class LLScriptIdentifier *identifier, class LLScriptFunctionDec *decl, class LLScriptStatement *statement )
      : LLASTNode( 3, identifier, decl, statement ) {
        symbol_table = gAllocationManager->new_tracked<LLScriptSymbolTable>();
    };
    virtual const char *get_node_name() { return "global func"; }
    virtual LLNodeType get_node_type() { return NODE_GLOBAL_FUNCTION; };
};

class LLScriptParamList : public LLASTNode {
  public:
    LLScriptParamList() : LLASTNode(0) {};
    LLScriptParamList( class LLScriptIdentifier *identifier ) : LLASTNode(1, identifier) {};
};

class LLScriptFunctionDec : public LLScriptParamList {
  public:
    LLScriptFunctionDec() : LLScriptParamList() {};
    LLScriptFunctionDec( class LLScriptIdentifier *identifier ) : LLScriptParamList(identifier) {};
    virtual const char *get_node_name() { return "function decl"; }
    virtual LLNodeType get_node_type() { return NODE_FUNCTION_DEC; };
};

class LLScriptEventDec : public LLScriptParamList {
  public:
    LLScriptEventDec() : LLScriptParamList() {};
    LLScriptEventDec( class LLScriptIdentifier *identifier ) : LLScriptParamList(identifier) {};
    virtual const char *get_node_name() { return "event decl"; }
    virtual LLNodeType get_node_type() { return NODE_EVENT_DEC; };
};

class LLScriptState : public LLASTNode {
  public:
    LLScriptState( class LLScriptIdentifier *identifier, class LLScriptEventHandler *state_body )
        : LLASTNode( 2, identifier, state_body ) {
      symbol_table = gAllocationManager->new_tracked<LLScriptSymbolTable>();
    };
    virtual const char *get_node_name() { return "state"; }
    virtual LLNodeType get_node_type() { return NODE_STATE; };
};

class LLScriptEventHandler : public LLASTNode {
  public:
    LLScriptEventHandler( class LLScriptIdentifier *identifier, class LLScriptEventDec *decl, class LLScriptStatement *body )
      : LLASTNode(3, identifier, decl, body) {
        symbol_table = gAllocationManager->new_tracked<LLScriptSymbolTable>();
    };
    virtual const char *get_node_name() { return "event handler"; }
    virtual LLNodeType get_node_type() { return NODE_EVENT_HANDLER; };
};

class LLScriptStatement : public LLASTNode {
  public:
    LLScriptStatement( int num, ... ) {
      va_list ap;
      va_start(ap, num);
      add_children(num, ap);
      va_end(ap);
    };
    LLScriptStatement( class LLScriptExpression *expression ) : LLASTNode(1, expression) {};
    virtual const char *get_node_name() { return "statement"; }
    virtual LLNodeType get_node_type() { return NODE_STATEMENT; };
};

class LLScriptCompoundStatement : public LLScriptStatement {
  public:
    LLScriptCompoundStatement( class LLScriptStatement *body ) : LLScriptStatement(1, body) {
      symbol_table = gAllocationManager->new_tracked<LLScriptSymbolTable>();
    }
    virtual const char *get_node_name() { return "compound statement"; };
    virtual LLNodeSubType get_node_sub_type() { return NODE_COMPOUND_STATEMENT; };
};

class LLScriptStateStatement : public LLScriptStatement {
  public:
    LLScriptStateStatement( ) : LLScriptStatement(0) {};
    LLScriptStateStatement( class LLScriptIdentifier *identifier ) : LLScriptStatement(1, identifier) {};
    virtual const char *get_node_name() { return "setstate"; };
    virtual LLNodeSubType get_node_sub_type() { return NODE_STATE_STATEMENT; };
};

class LLScriptJumpStatement : public LLScriptStatement {
  public:
    LLScriptJumpStatement( class LLScriptIdentifier *identifier ) : LLScriptStatement(1, identifier) {};
    virtual const char *get_node_name() { return "jump"; };
    virtual LLNodeSubType get_node_sub_type() { return NODE_JUMP_STATEMENT; };
};

class LLScriptLabel : public LLScriptStatement {
  public:
    LLScriptLabel( class LLScriptIdentifier *identifier ) : LLScriptStatement(1, identifier) {};
    virtual const char *get_node_name() { return "label"; };
    virtual LLNodeSubType get_node_sub_type() { return NODE_LABEL; };
};

class LLScriptReturnStatement : public LLScriptStatement {
  public:
    LLScriptReturnStatement( class LLScriptExpression *expression ) : LLScriptStatement(1, expression) {};
    virtual const char *get_node_name() { return "return"; };
    virtual LLNodeSubType get_node_sub_type() { return NODE_RETURN_STATEMENT; };
};

class LLScriptIfStatement : public LLScriptStatement {
  public:
    LLScriptIfStatement( class LLScriptExpression *expression, class LLScriptStatement *true_branch, class LLScriptStatement *false_branch)
      : LLScriptStatement( 3, expression, true_branch, false_branch ) {};
    virtual const char *get_node_name() { return "if"; };
    virtual LLNodeSubType get_node_sub_type() { return NODE_IF_STATEMENT; };
};

class LLScriptForStatement : public LLScriptStatement {
  public:
    LLScriptForStatement( class LLScriptForExpressionList *init, class LLScriptExpression *condition,
        class LLScriptForExpressionList *cont, class LLScriptStatement *body)
      : LLScriptStatement( 4, init, condition, cont, body ) {};
    virtual const char *get_node_name() { return "for"; };
    virtual LLNodeSubType get_node_sub_type() { return NODE_FOR_STATEMENT; };
};

class LLScriptForExpressionList : public LLASTNode {
  public:
    LLScriptForExpressionList() : LLASTNode(0) {};
    LLScriptForExpressionList( class LLScriptExpression *expr ) : LLASTNode(1, expr) {};
    virtual const char *get_node_name() { return "for expr list"; }
    virtual LLNodeType get_node_type() { return NODE_FOR_EXPRESSION_LIST; };
};

class LLScriptDoStatement : public LLScriptStatement {
  public:
    LLScriptDoStatement( class LLScriptStatement *body, class LLScriptExpression *condition )
      : LLScriptStatement(2, body, condition) {};
    virtual const char *get_node_name() { return "do"; };
    virtual LLNodeSubType get_node_sub_type() { return NODE_DO_STATEMENT; };
};

class LLScriptWhileStatement : public LLScriptStatement {
  public:
    LLScriptWhileStatement( class LLScriptExpression *condition, class LLScriptStatement *body )
      : LLScriptStatement(2, condition, body) {};
    virtual const char *get_node_name() { return "while"; };
    virtual LLNodeSubType get_node_sub_type() { return NODE_WHILE_STATEMENT; };
};


class LLScriptDeclaration : public LLScriptStatement {
  public:
    LLScriptDeclaration(class LLScriptIdentifier *identifier, class LLScriptExpression *value)
      : LLScriptStatement(2, identifier, value) { };
    virtual const char *get_node_name() { return "declaration"; };
    virtual LLNodeSubType get_node_sub_type() { return NODE_DECLARATION; };

    virtual LLScriptConstant* get_constant_value();
};

class LLScriptExpression : public LLASTNode {
public:
    LLScriptExpression() : LLASTNode(0), operation(0) {};
    LLScriptExpression(int num, ...): operation(0) {
      va_list ap;
      va_start(ap, num);
      add_children(num, ap);
      va_end(ap);
    };

    virtual const char *get_node_name() {
      return "base expression";
    };
    virtual LLNodeType get_node_type() { return NODE_EXPRESSION; };

    virtual LLScriptConstant* get_constant_value();
    virtual bool node_allows_folding() { return true; };
    int get_operation() {return operation;};
    int operation;
};


class LLScriptConstantExpression: public LLScriptExpression {
public:
    LLScriptConstantExpression( LLScriptConstant* constant )
      : LLScriptExpression() {
      assert(constant);
      if (constant->is_static())
        constant = constant->copy();
      push_child(constant);
      constant_value = constant;
    };

    virtual const char *get_node_name() {
      return "constant expression";
    };
    virtual LLNodeType get_node_type() { return NODE_EXPRESSION; };
    virtual LLNodeSubType get_node_sub_type() { return NODE_CONSTANT_EXPRESSION; };
};


class LLScriptParenthesisExpression: public LLScriptExpression {
public:
    LLScriptParenthesisExpression( LLScriptExpression* expr )
      : LLScriptExpression(1, expr) { operation = '('; };

    virtual const char *get_node_name() {
      return "parenthesis expression";
    };
    virtual LLNodeType get_node_type() { return NODE_EXPRESSION; };
    virtual LLNodeSubType get_node_sub_type() { return NODE_PARENTHESIS_EXPRESSION; };
};


class LLScriptBinaryExpression : public LLScriptExpression {
public:
    LLScriptBinaryExpression( LLScriptExpression *lvalue, int oper, LLScriptExpression *rvalue )
    : LLScriptExpression(2, lvalue, rvalue) { operation = oper; };

    virtual const char *get_node_name() {
      static thread_local char buf[256];
      snprintf( buf, 256, "binary expression: '%s'", operation_str(operation) );
      return buf;
    };
    virtual LLNodeType get_node_type() { return NODE_EXPRESSION; };
    virtual LLNodeSubType get_node_sub_type() { return NODE_BINARY_EXPRESSION; };
};

class LLScriptUnaryExpression : public LLScriptExpression {
public:
    LLScriptUnaryExpression( LLScriptExpression *lvalue, int oper )
            : LLScriptExpression(1, lvalue) { operation = oper; };

    virtual const char *get_node_name() {
      static thread_local char buf[256];
      snprintf( buf, 256, "unary expression: '%s'", operation_str(operation) );
      return buf;
    };
    virtual LLNodeType get_node_type() { return NODE_EXPRESSION; };
    virtual LLNodeSubType get_node_sub_type() { return NODE_UNARY_EXPRESSION; };
};



class LLScriptTypecastExpression : public LLScriptExpression {
  public:
    LLScriptTypecastExpression( LLScriptType *_type, LLScriptExpression *expression )
      : LLScriptExpression(1, expression) {type = _type;};

    virtual const char *get_node_name() { return "typecast expression"; }
    virtual LLNodeSubType get_node_sub_type() { return NODE_TYPECAST_EXPRESSION; };
};

class LLScriptPrintExpression : public LLScriptExpression {
  public:
    LLScriptPrintExpression( LLScriptExpression *expression )
      : LLScriptExpression( 1, expression ) {type = TYPE(LST_NULL); };
    virtual const char *get_node_name() { return "print() call"; }
    virtual LLNodeSubType get_node_sub_type() { return NODE_PRINT_EXPRESSION; };
};

class LLScriptFunctionExpression : public LLScriptExpression {
  public:
    LLScriptFunctionExpression( LLScriptIdentifier *identifier )
      : LLScriptExpression( 1, identifier ) {};
    LLScriptFunctionExpression( LLScriptIdentifier *identifier, LLScriptExpression *arguments )
      : LLScriptExpression( 2, identifier, arguments) {};
    virtual const char *get_node_name() { return "function call"; }
    virtual LLNodeSubType get_node_sub_type() { return NODE_FUNCTION_EXPRESSION; };

    virtual bool node_allows_folding() { return false; };
};

class LLScriptVectorExpression : public LLScriptExpression {
  public:
    LLScriptVectorExpression( LLScriptExpression *v1, LLScriptExpression *v2, LLScriptExpression *v3 )
      : LLScriptExpression(3, v1, v2, v3) { type = TYPE(LST_VECTOR); }
    LLScriptVectorExpression( ) : LLScriptExpression(0) {
      constant_value = gAllocationManager->new_tracked<LLScriptVectorConstant>(0.0f,0.0f,0.0f);
      type = TYPE(LST_VECTOR);
    }
    virtual const char *get_node_name() { return "vector expression"; }
    virtual LLNodeSubType get_node_sub_type() { return NODE_VECTOR_EXPRESSION; };
};

class LLScriptQuaternionExpression : public LLScriptExpression {
  public:
    LLScriptQuaternionExpression( LLScriptExpression *v1, LLScriptExpression *v2, LLScriptExpression *v3, LLScriptExpression *v4 )
      : LLScriptExpression(4, v1, v2, v3, v4) { type = TYPE(LST_QUATERNION); };
    LLScriptQuaternionExpression( ) : LLScriptExpression(0) {
      constant_value = gAllocationManager->new_tracked<LLScriptQuaternionConstant>(0.0f, 0.0f, 0.0f, 0.0f);
      type = TYPE(LST_QUATERNION);
    };
    virtual const char *get_node_name() { return "quaternion expression"; };
    virtual LLNodeSubType get_node_sub_type() { return NODE_QUATERNION_EXPRESSION; };
};

class LLScriptListExpression : public LLScriptExpression {
  public:
    LLScriptListExpression( LLScriptExpression *c ) : LLScriptExpression( 1, c ) { type = TYPE(LST_LIST); };

    virtual const char *get_node_name() { return "list expression"; };
    virtual LLNodeSubType get_node_sub_type() { return NODE_LIST_EXPRESSION; }
};

class LLScriptLValueExpression : public LLScriptExpression {
  public:
    LLScriptLValueExpression( LLScriptIdentifier *identifier )
      : LLScriptExpression(1, identifier), is_foldable(false) {};
    LLScriptLValueExpression( LLScriptIdentifier *identifier, LLScriptIdentifier *member )
        : LLScriptExpression(2, identifier, member), is_foldable(false) {};
    virtual const char *get_node_name() {
      static thread_local char buf[256];
      snprintf(buf, 256, "lvalue expression {%sfoldable}", is_foldable ? "" : "not ");
      return buf;
    };
    virtual LLNodeSubType get_node_sub_type() { return NODE_LVALUE_EXPRESSION; };

    virtual LLScriptConstant* get_constant_value();
    void set_is_foldable(bool foldable) {is_foldable = foldable;};
    bool get_is_foldable() {return is_foldable;};
  private:
    bool is_foldable;
};

void sling_init_builtins(const char *builtins_file);

}

#endif /* not _LSLMINI_HH */
