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
#include "strings.hh"

namespace Tailslide {

typedef int   S32;
typedef float F32;

struct ScriptContext {
  LSLScript *script = nullptr;
  ScriptAllocator *allocator = nullptr;
  Logger *logger = nullptr;
  bool ast_sane = true;
  Tailslide::TAILSLIDE_LTYPE glloc {0};
};

class Vector3: public TrackableObject {
  public:
    Vector3(ScriptContext *ctx, float x, float y, float z) : TrackableObject(ctx), x(x), y(y), z(z) {};
    Vector3(float x, float y, float z): Vector3(nullptr, x, y, z) {};
    float x, y, z;

    bool operator==(const class Vector3 &other) {
      return (x == other.x) && (y == other.y) && (z == other.z);
    };
    bool operator!=(const class Vector3 &other) {
      return !(*this == other);
    };
};

class Quaternion: public TrackableObject {
  public:
    Quaternion(ScriptContext *ctx, float x, float y, float z, float s) : TrackableObject(ctx), x(x), y(y), z(z), s(s) {};
    Quaternion(float x, float y, float z, float s): Quaternion(nullptr, x, y, z, s) {};
    float x, y, z, s;

    bool operator==(const class Quaternion &other) {
      return (x == other.x) && (y == other.y) && (z == other.z) && (s == other.s);
    };
    bool operator!=(const class Quaternion &other) {
      return !(*this == other);
    };
};


class LSLScript : public LSLASTNode {
  public:
    LSLScript( ScriptContext *ctx, class LSLASTNodeList *globals, class LSLASTNodeList *states )
      : LSLASTNode( ctx, 2, globals, states ) {
        symbol_table = ctx->allocator->new_tracked<LSLSymbolTable>();
    };

    virtual const char *get_node_name() { return "script"; };
    virtual LSLNodeType get_node_type() { return NODE_SCRIPT; };
    virtual LSLSymbol *lookup_symbol(const char *name, LSLSymbolType sym_type, LSLASTNode *start_node);

    void optimize(const OptimizationContext &ctx);
    void recalculate_reference_data();
    void validate_globals(bool sl_strict);
};

class LSLGlobalStorage : public LSLASTNode {
  public:
    LSLGlobalStorage( ScriptContext *ctx, class LSLGlobalVariable *variables, class LSLGlobalFunction *functions )
      : LSLASTNode( ctx, 2, variables, functions ) {};
    virtual const char *get_node_name() { return "global storage"; }
    virtual LSLNodeType get_node_type() { return NODE_GLOBAL_STORAGE; };
};

class LSLIdentifier : public LSLASTNode {
  public:
    LSLIdentifier( ScriptContext *ctx, const char *name ) : LSLASTNode(ctx), symbol(NULL), name(name) {};
    LSLIdentifier( ScriptContext *ctx, class LSLType *_type, const char *name ) : LSLASTNode(ctx), symbol(NULL), name(name) { type = _type; };
    LSLIdentifier( ScriptContext *ctx, class LSLType *_type, const char *name, YYLTYPE *lloc ) : LSLASTNode( ctx, lloc, 0), symbol(NULL), name(name) { type = _type; };
    LSLIdentifier( ScriptContext *ctx, LSLIdentifier *other ) : LSLASTNode(ctx), symbol(NULL), name(other->get_name()) {};

    const char    *get_name() { return name; }

    void resolve_symbol(LSLSymbolType symbol_type);
    void set_symbol( LSLSymbol *_symbol ) { symbol = _symbol; };
    LSLSymbol *get_symbol() { return symbol; };

    virtual const char *get_node_name() {
      static thread_local char buf[256];
      snprintf(buf, 256, "identifier \"%s\"", name);
      return buf;
    }
    virtual LSLNodeType get_node_type() { return NODE_IDENTIFIER; };
    virtual class LSLConstant *get_constant_value();

  private:
    LSLSymbol                  *symbol;
    const char                      *name;
};

class LSLGlobalVariable : public LSLASTNode {
  public:
    LSLGlobalVariable( ScriptContext *ctx, class LSLIdentifier *identifier, class LSLExpression *value )
      : LSLASTNode(ctx, 2, identifier, value) { DEBUG( LOG_DEBUG_SPAM, NULL, "made a global var\n"); };
    virtual const char *get_node_name() { return "global var"; }
    virtual LSLNodeType get_node_type() { return NODE_GLOBAL_VARIABLE; };

    virtual LSLConstant* get_constant_value();

};


class LSLConstant : public LSLASTNode {
  public:
    LSLConstant(ScriptContext *ctx) : LSLASTNode(ctx) { constant_value = this; }
    virtual const char *get_node_name() { return "unknown constant"; }
    virtual LSLNodeType get_node_type() { return NODE_CONSTANT; };
    // make a shallow copy of the constant
    virtual LSLConstant* copy(ScriptAllocator *allocator) = 0;
    virtual bool contains_nan() { return false; };
};

/////////////////////////////////////////////////////
// Integer Constant

class LSLIntegerConstant : public LSLConstant {
  public:
    LSLIntegerConstant( ScriptContext *ctx, int v ) : LSLConstant(ctx), value(v) { type = TYPE(LST_INTEGER); }

    virtual const char *get_node_name() {
      static thread_local char buf[256];
      snprintf(buf, 256, "integer constant: %d", value);
      return buf;
    }

    virtual LSLNodeSubType get_node_sub_type() { return NODE_INTEGER_CONSTANT; }

    int get_value() { return value; }
    void set_value(int val) { value = val; }
    virtual LSLConstant* copy(ScriptAllocator *allocator) {
      auto* new_const = allocator->new_tracked<LSLIntegerConstant>(value);
      new_const->constant_value = new_const;
      return new_const;
    };

  private:
    int value;
};


/////////////////////////////////////////////////////
// Float Constant

class LSLFloatConstant : public LSLConstant {
  public:
    LSLFloatConstant( ScriptContext *ctx, float v ) : LSLConstant(ctx), value(v) { type = TYPE(LST_FLOATINGPOINT); }

    virtual const char *get_node_name() {
      static thread_local char buf[256];
      snprintf(buf, 256, "float constant: %f", value);
      return buf;
    }

    virtual LSLNodeSubType get_node_sub_type() { return NODE_FLOAT_CONSTANT; }

    float get_value() { return value; }
    void set_value(float val) { value = val; }
    virtual bool contains_nan();
    virtual LSLConstant* copy(ScriptAllocator *allocator) {
      auto* new_const = allocator->new_tracked<LSLFloatConstant>(value);
      new_const->constant_value = new_const;
      return new_const;
    };

  private:
    float value;
};


/////////////////////////////////////////////////////
// String Constant

class LSLStringConstant : public LSLConstant {
  public:
    LSLStringConstant( ScriptContext *ctx, char *v ) : LSLConstant(ctx), value(v) { type = TYPE(LST_STRING); }

    virtual const char *get_node_name() {
      static thread_local char buf[256];
      snprintf(buf, 256, "string constant: \"%s\"", escape_string(value).c_str());
      return buf;
    }

    virtual LSLNodeSubType get_node_sub_type() { return NODE_STRING_CONSTANT; }

    char *get_value() { return value; }
    void set_value(char *val) { value = val; }
    virtual LSLConstant* copy(ScriptAllocator *allocator) {
      auto* new_const = allocator->new_tracked<LSLStringConstant>(value);
      new_const->constant_value = new_const;
      return new_const;
    };

  private:
    char *value;
};


/////////////////////////////////////////////////////
// List Constant

class LSLListConstant : public LSLConstant {
  public:
    LSLListConstant( ScriptContext *ctx, class LSLConstant *v ) : LSLConstant(ctx) {
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

    virtual LSLNodeSubType get_node_sub_type() { return NODE_LIST_CONSTANT; }

    class LSLConstant *get_value() { return (LSLConstant*)get_children(); }
    void set_value(class LSLConstant *val) {  }

    int get_length() {
      LSLASTNode *node = get_children();
      int i = 0;
      for ( ; node; node = node->get_next() )
        ++i;
      return i;
    }

    virtual LSLConstant* copy(ScriptAllocator *allocator) {
      // TODO: probably actually have to copy all of the elements.
      //  since it will reparent the child values.
      auto* new_const = allocator->new_tracked<LSLListConstant>(get_value());
      new_const->constant_value = new_const;
      return new_const;
    };
};

/////////////////////////////////////////////////////
// Vector Constant

class LSLVectorConstant : public LSLConstant {
  public:
    LSLVectorConstant( ScriptContext *ctx, float v1, float v2, float v3 ): LSLConstant(ctx) {
      value = ctx->allocator->new_tracked<Vector3>(v1, v2, v3 );
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

    virtual LSLNodeSubType get_node_sub_type() { return NODE_VECTOR_CONSTANT; }

    Vector3 *get_value() { return value; }
    void set_value(Vector3 *val) { value = val; }
    virtual bool contains_nan();

    virtual LSLConstant* copy(ScriptAllocator *allocator) {
      auto* new_const = allocator->new_tracked<LSLVectorConstant>(value->x, value->y, value->z);
      new_const->constant_value = new_const;
      return new_const;
    };

  private:
    Vector3 *value;
};


/////////////////////////////////////////////////////
// Quaternion Constant

class LSLQuaternionConstant : public LSLConstant {
  public:
    LSLQuaternionConstant( ScriptContext *ctx, float v1, float v2, float v3, float v4 ): LSLConstant(ctx) {
      value = ctx->allocator->new_tracked<Quaternion>(v1, v2, v3, v4 );
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

    virtual LSLNodeSubType get_node_sub_type() { return NODE_QUATERNION_CONSTANT; }

    Quaternion *get_value() { return value; }
    void set_value(class Quaternion *val) { value = val; }
    virtual bool contains_nan();

    virtual LSLConstant* copy(ScriptAllocator *allocator) {
      auto* new_const = allocator->new_tracked<LSLQuaternionConstant>(value->x, value->y, value->z, value->s);
      new_const->constant_value = new_const;
      return new_const;
    };

  private:
    Quaternion *value;
};


class LSLGlobalFunction : public LSLASTNode {
  public:
    LSLGlobalFunction( ScriptContext *ctx, class LSLIdentifier *identifier, class LSLFunctionDec *decl, class LSLStatement *statement )
      : LSLASTNode( ctx, 3, identifier, decl, statement ) {
        symbol_table = ctx->allocator->new_tracked<LSLSymbolTable>();
    };
    virtual const char *get_node_name() { return "global func"; }
    virtual LSLNodeType get_node_type() { return NODE_GLOBAL_FUNCTION; };
};

class LSLParamList : public LSLASTNode {
  public:
    LSLParamList( ScriptContext *ctx ) : LSLASTNode(ctx, 0) {};
    LSLParamList( ScriptContext *ctx, class LSLIdentifier *identifier ) : LSLASTNode(ctx, 1, identifier) {};
};

class LSLFunctionDec : public LSLParamList {
  public:
    LSLFunctionDec(ScriptContext *ctx) : LSLParamList(ctx) {};
    LSLFunctionDec( ScriptContext *ctx, class LSLIdentifier *identifier ) : LSLParamList(ctx, identifier) {};
    virtual const char *get_node_name() { return "function decl"; }
    virtual LSLNodeType get_node_type() { return NODE_FUNCTION_DEC; };
};

class LSLEventDec : public LSLParamList {
  public:
    LSLEventDec(ScriptContext *ctx) : LSLParamList(ctx) {};
    LSLEventDec( ScriptContext *ctx, class LSLIdentifier *identifier ) : LSLParamList(ctx, identifier) {};
    virtual const char *get_node_name() { return "event decl"; }
    virtual LSLNodeType get_node_type() { return NODE_EVENT_DEC; };
};

class LSLState : public LSLASTNode {
  public:
    LSLState( ScriptContext *ctx, class LSLIdentifier *identifier, class LSLEventHandler *state_body )
        : LSLASTNode( ctx, 2, identifier, state_body ) {
      symbol_table = ctx->allocator->new_tracked<LSLSymbolTable>();
    };
    virtual const char *get_node_name() { return "state"; }
    virtual LSLNodeType get_node_type() { return NODE_STATE; };
};

class LSLEventHandler : public LSLASTNode {
  public:
    LSLEventHandler( ScriptContext *ctx, class LSLIdentifier *identifier, class LSLEventDec *decl, class LSLStatement *body )
      : LSLASTNode(ctx, 3, identifier, decl, body) {
        symbol_table = ctx->allocator->new_tracked<LSLSymbolTable>();
    };
    virtual const char *get_node_name() { return "event handler"; }
    virtual LSLNodeType get_node_type() { return NODE_EVENT_HANDLER; };
};

class LSLStatement : public LSLASTNode {
  public:
    LSLStatement( ScriptContext *ctx, int num, ... ): LSLASTNode(ctx) {
      va_list ap;
      va_start(ap, num);
      add_children(num, ap);
      va_end(ap);
    };
    LSLStatement( ScriptContext *ctx, class LSLExpression *expression ) : LSLASTNode(ctx, 1, expression) {};
    virtual const char *get_node_name() { return "statement"; }
    virtual LSLNodeType get_node_type() { return NODE_STATEMENT; };
};

class LSLCompoundStatement : public LSLStatement {
  public:
    LSLCompoundStatement( ScriptContext *ctx, class LSLStatement *body ) : LSLStatement(ctx, 1, body) {
      symbol_table = ctx->allocator->new_tracked<LSLSymbolTable>();
    }
    virtual const char *get_node_name() { return "compound statement"; };
    virtual LSLNodeSubType get_node_sub_type() { return NODE_COMPOUND_STATEMENT; };
};

class LSLStateStatement : public LSLStatement {
  public:
    LSLStateStatement(ScriptContext *ctx) : LSLStatement(ctx, 0) {};
    LSLStateStatement( ScriptContext *ctx, class LSLIdentifier *identifier ) : LSLStatement(ctx, 1, identifier) {};
    virtual const char *get_node_name() { return "setstate"; };
    virtual LSLNodeSubType get_node_sub_type() { return NODE_STATE_STATEMENT; };
};

class LSLJumpStatement : public LSLStatement {
  public:
    LSLJumpStatement( ScriptContext *ctx, class LSLIdentifier *identifier ) : LSLStatement(ctx, 1, identifier) {};
    virtual const char *get_node_name() { return "jump"; };
    virtual LSLNodeSubType get_node_sub_type() { return NODE_JUMP_STATEMENT; };
};

class LSLLabel : public LSLStatement {
  public:
    LSLLabel( ScriptContext *ctx, class LSLIdentifier *identifier ) : LSLStatement(ctx, 1, identifier) {};
    virtual const char *get_node_name() { return "label"; };
    virtual LSLNodeSubType get_node_sub_type() { return NODE_LABEL; };
};

class LSLReturnStatement : public LSLStatement {
  public:
    LSLReturnStatement( ScriptContext *ctx, class LSLExpression *expression ) : LSLStatement(ctx, 1, expression) {};
    virtual const char *get_node_name() { return "return"; };
    virtual LSLNodeSubType get_node_sub_type() { return NODE_RETURN_STATEMENT; };
};

class LSLIfStatement : public LSLStatement {
  public:
    LSLIfStatement( ScriptContext *ctx, class LSLExpression *expression, class LSLStatement *true_branch, class LSLStatement *false_branch)
      : LSLStatement( ctx, 3, expression, true_branch, false_branch ) {};
    virtual const char *get_node_name() { return "if"; };
    virtual LSLNodeSubType get_node_sub_type() { return NODE_IF_STATEMENT; };
};

class LSLForStatement : public LSLStatement {
  public:
    LSLForStatement(ScriptContext *ctx, class LSLASTNodeList *init, class LSLExpression *condition,
                    class LSLASTNodeList *cont, class LSLStatement *body)
      : LSLStatement( ctx, 4, init, condition, cont, body ) {};
    virtual const char *get_node_name() { return "for"; };
    virtual LSLNodeSubType get_node_sub_type() { return NODE_FOR_STATEMENT; };
};

class LSLDoStatement : public LSLStatement {
  public:
    LSLDoStatement( ScriptContext *ctx, class LSLStatement *body, class LSLExpression *condition )
      : LSLStatement(ctx, 2, body, condition) {};
    virtual const char *get_node_name() { return "do"; };
    virtual LSLNodeSubType get_node_sub_type() { return NODE_DO_STATEMENT; };
};

class LSLWhileStatement : public LSLStatement {
  public:
    LSLWhileStatement( ScriptContext *ctx, class LSLExpression *condition, class LSLStatement *body )
      : LSLStatement(ctx, 2, condition, body) {};
    virtual const char *get_node_name() { return "while"; };
    virtual LSLNodeSubType get_node_sub_type() { return NODE_WHILE_STATEMENT; };
};


class LSLDeclaration : public LSLStatement {
  public:
    LSLDeclaration(ScriptContext *ctx, class LSLIdentifier *identifier, class LSLExpression *value)
      : LSLStatement(ctx, 2, identifier, value) { };
    virtual const char *get_node_name() { return "declaration"; };
    virtual LSLNodeSubType get_node_sub_type() { return NODE_DECLARATION; };

    virtual LSLConstant* get_constant_value();
};

class LSLExpression : public LSLASTNode {
public:
    LSLExpression(ScriptContext *ctx) : LSLASTNode(ctx, 0), operation(0) {};
    LSLExpression(ScriptContext *ctx, int num, ...): LSLASTNode(ctx), operation(0) {
      va_list ap;
      va_start(ap, num);
      add_children(num, ap);
      va_end(ap);
    };

    virtual const char *get_node_name() {
      return "base expression";
    };
    virtual LSLNodeType get_node_type() { return NODE_EXPRESSION; };

    virtual LSLConstant* get_constant_value();
    virtual bool node_allows_folding() { return true; };
    int get_operation() {return operation;};
    int operation;
};


class LSLConstantExpression: public LSLExpression {
public:
    LSLConstantExpression( ScriptContext *ctx, LSLConstant* constant )
      : LSLExpression(ctx) {
      assert(constant);
      if (constant->is_static())
        constant = constant->copy(ctx->allocator);
      push_child(constant);
      constant_value = constant;
    };

    virtual const char *get_node_name() {
      return "constant expression";
    };
    virtual LSLNodeSubType get_node_sub_type() { return NODE_CONSTANT_EXPRESSION; };
};


class LSLParenthesisExpression: public LSLExpression {
public:
    LSLParenthesisExpression( ScriptContext *ctx, LSLExpression* expr )
      : LSLExpression(ctx, 1, expr) { operation = '('; };

    virtual const char *get_node_name() {
      return "parenthesis expression";
    };
    virtual LSLNodeSubType get_node_sub_type() { return NODE_PARENTHESIS_EXPRESSION; };
};


class LSLBinaryExpression : public LSLExpression {
public:
    LSLBinaryExpression( ScriptContext *ctx, LSLExpression *lvalue, int oper, LSLExpression *rvalue )
    : LSLExpression(ctx, 2, lvalue, rvalue) { operation = oper; };

    virtual const char *get_node_name() {
      static thread_local char buf[256];
      snprintf( buf, 256, "binary expression: '%s'", operation_repr_str(operation) );
      return buf;
    };
    virtual LSLNodeSubType get_node_sub_type() { return NODE_BINARY_EXPRESSION; };
};

class LSLUnaryExpression : public LSLExpression {
public:
    LSLUnaryExpression( ScriptContext *ctx, LSLExpression *lvalue, int oper )
            : LSLExpression(ctx, 1, lvalue) { operation = oper; };

    virtual const char *get_node_name() {
      static thread_local char buf[256];
      snprintf( buf, 256, "unary expression: '%s'", operation_repr_str(operation) );
      return buf;
    };
    virtual LSLNodeSubType get_node_sub_type() { return NODE_UNARY_EXPRESSION; };
};



class LSLTypecastExpression : public LSLExpression {
  public:
    LSLTypecastExpression( ScriptContext *ctx, LSLType *_type, LSLExpression *expression )
      : LSLExpression(ctx, 1, expression) {type = _type;};

    virtual const char *get_node_name() { return "typecast expression"; }
    virtual LSLNodeSubType get_node_sub_type() { return NODE_TYPECAST_EXPRESSION; };
};

class LSLPrintExpression : public LSLExpression {
  public:
    LSLPrintExpression( ScriptContext *ctx, LSLExpression *expression )
      : LSLExpression( ctx, 1, expression ) {type = TYPE(LST_NULL); };
    virtual const char *get_node_name() { return "print() call"; }
    virtual LSLNodeSubType get_node_sub_type() { return NODE_PRINT_EXPRESSION; };
};

class LSLFunctionExpression : public LSLExpression {
  public:
    LSLFunctionExpression( ScriptContext *ctx, LSLIdentifier *identifier )
      : LSLExpression( ctx, 1, identifier ) {};
    LSLFunctionExpression( ScriptContext *ctx, LSLIdentifier *identifier, LSLExpression *arguments )
      : LSLExpression( ctx, 2, identifier, arguments) {};
    virtual const char *get_node_name() { return "function call"; }
    virtual LSLNodeSubType get_node_sub_type() { return NODE_FUNCTION_EXPRESSION; };

    virtual bool node_allows_folding() { return false; };
};

class LSLVectorExpression : public LSLExpression {
  public:
    LSLVectorExpression( ScriptContext *ctx, LSLExpression *v1, LSLExpression *v2, LSLExpression *v3 )
      : LSLExpression(ctx, 3, v1, v2, v3) { type = TYPE(LST_VECTOR); }
    LSLVectorExpression(ScriptContext *ctx) : LSLExpression(ctx,0) {
      constant_value = ctx->allocator->new_tracked<LSLVectorConstant>(0.0f,0.0f,0.0f);
      type = TYPE(LST_VECTOR);
    }
    virtual const char *get_node_name() { return "vector expression"; }
    virtual LSLNodeSubType get_node_sub_type() { return NODE_VECTOR_EXPRESSION; };
};

class LSLQuaternionExpression : public LSLExpression {
  public:
    LSLQuaternionExpression( ScriptContext *ctx, LSLExpression *v1, LSLExpression *v2, LSLExpression *v3, LSLExpression *v4 )
      : LSLExpression(ctx, 4, v1, v2, v3, v4) { type = TYPE(LST_QUATERNION); };
    LSLQuaternionExpression(ScriptContext *ctx) : LSLExpression(ctx,0) {
      constant_value = ctx->allocator->new_tracked<LSLQuaternionConstant>(0.0f, 0.0f, 0.0f, 0.0f);
      type = TYPE(LST_QUATERNION);
    };
    virtual const char *get_node_name() { return "quaternion expression"; };
    virtual LSLNodeSubType get_node_sub_type() { return NODE_QUATERNION_EXPRESSION; };
};

class LSLListExpression : public LSLExpression {
  public:
    LSLListExpression( ScriptContext *ctx, LSLExpression *c ) : LSLExpression( ctx, 1, c ) { type = TYPE(LST_LIST); };

    virtual const char *get_node_name() { return "list expression"; };
    virtual LSLNodeSubType get_node_sub_type() { return NODE_LIST_EXPRESSION; }
};

class LSLLValueExpression : public LSLExpression {
  public:
    LSLLValueExpression( ScriptContext *ctx, LSLIdentifier *identifier )
      : LSLExpression(ctx, 1, identifier), is_foldable(false) {};
    LSLLValueExpression( ScriptContext *ctx, LSLIdentifier *identifier, LSLIdentifier *member )
        : LSLExpression(ctx, 2, identifier, member), is_foldable(false) {};
    virtual const char *get_node_name() {
      static thread_local char buf[256];
      snprintf(buf, 256, "lvalue expression {%sfoldable}", is_foldable ? "" : "not ");
      return buf;
    };
    virtual LSLNodeSubType get_node_sub_type() { return NODE_LVALUE_EXPRESSION; };

    virtual LSLConstant* get_constant_value();
    void set_is_foldable(bool foldable) {is_foldable = foldable;};
    bool get_is_foldable() {return is_foldable;};
  private:
    bool is_foldable;
};

void tailslide_init_builtins(const char *builtins_file);

}

#endif /* not _LSLMINI_HH */
