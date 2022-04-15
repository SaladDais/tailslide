#ifndef TAILSLIDE_LSLMINI_HH
#define TAILSLIDE_LSLMINI_HH

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
  LSLSymbolTable *builtins = nullptr;
  bool ast_sane = true;
  Tailslide::TailslideLType glloc {0};
};

struct Vector3 {
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {};
    Vector3(): Vector3(0, 0, 0) {};
    float x, y, z;

    bool operator==(const class Vector3 &other) const {
      return (x == other.x) && (y == other.y) && (z == other.z);
    };
    bool operator!=(const class Vector3 &other) const {
      return !(*this == other);
    };
};

struct Quaternion {
    Quaternion(float x, float y, float z, float s): x(x), y(y), z(z), s(s) {};
    Quaternion(): Quaternion(0, 0, 0, 1) {};
    float x, y, z, s;

    bool operator==(const class Quaternion &other) const {
      return (x == other.x) && (y == other.y) && (z == other.z) && (s == other.s);
    };
    bool operator!=(const class Quaternion &other) const {
      return !(*this == other);
    };
};


class LSLScript : public LSLASTNode {
  public:
    LSLScript( ScriptContext *ctx, class LSLASTNodeList *globals, class LSLASTNodeList *states )
      : LSLASTNode( ctx, 2, globals, states ) {
      _mSymbolTable = ctx->allocator->newTracked<LSLSymbolTable>();
    };

    virtual const char *getNodeName() { return "script"; };
    virtual LSLNodeType getNodeType() { return NODE_SCRIPT; };
    virtual LSLSymbol *lookupSymbol(const char *name, LSLSymbolType sym_type);

    void optimize(const OptimizationOptions &ctx);
    void recalculateReferenceData();
    void validateGlobals(bool sl_strict);
};

class LSLGlobalStorage : public LSLASTNode {
  public:
    LSLGlobalStorage( ScriptContext *ctx, class LSLGlobalVariable *variables, class LSLGlobalFunction *functions )
      : LSLASTNode( ctx, 2, variables, functions ) {};
    virtual const char *getNodeName() { return "global storage"; }
    virtual LSLNodeType getNodeType() { return NODE_GLOBAL_STORAGE; };
};

class LSLIdentifier : public LSLASTNode {
  public:
    LSLIdentifier( ScriptContext *ctx, const char *name ) : LSLASTNode(ctx), _mName(name) {};
    LSLIdentifier( ScriptContext *ctx, class LSLType *type, const char *name ) : LSLASTNode(ctx), _mName(name) { _mType = type; };
    LSLIdentifier( ScriptContext *ctx, class LSLType *type, const char *name, YYLTYPE *lloc ) : LSLASTNode(ctx, lloc, 0), _mName(name) { _mType = type; };
    LSLIdentifier( ScriptContext *ctx, LSLIdentifier *other ) : LSLASTNode(ctx), _mName(other->getName()) {};

    const char    *getName() { return _mName; }

    void resolveSymbol(LSLSymbolType symbol_type);
    void setSymbol(LSLSymbol *symbol ) { _mSymbol = symbol; };
    virtual LSLSymbol *getSymbol() { return _mSymbol; };

    virtual const char *getNodeName() {
      static thread_local char buf[256];
      snprintf(buf, 256, "identifier \"%s\"", _mName);
      return buf;
    }
    virtual LSLNodeType getNodeType() { return NODE_IDENTIFIER; };
    virtual class LSLConstant *getConstantValue();

  private:
    LSLSymbol                  *_mSymbol = nullptr;
    const char                      *_mName;
};

class LSLGlobalVariable : public LSLASTNode {
  public:
    LSLGlobalVariable( ScriptContext *ctx, class LSLIdentifier *identifier, class LSLExpression *value )
      : LSLASTNode(ctx, 2, identifier, value) { DEBUG( LOG_DEBUG_SPAM, nullptr, "made a global var\n"); };
    virtual const char *getNodeName() { return "global var"; }
    virtual LSLNodeType getNodeType() { return NODE_GLOBAL_VARIABLE; };

    virtual LSLConstant* getConstantValue();
    virtual LSLSymbol *getSymbol() {return ((LSLIdentifier *) getChild(0))->getSymbol(); }
};


class LSLConstant : public LSLASTNode {
  public:
    explicit LSLConstant(ScriptContext *ctx) : LSLASTNode(ctx) { _mConstantValue = this; }
    virtual const char *getNodeName() { return "unknown constant"; }
    virtual LSLNodeType getNodeType() { return NODE_CONSTANT; };
    // make a shallow copy of the constant
    virtual LSLConstant* copy(ScriptAllocator *allocator) = 0;
    virtual bool containsNaN() { return false; };
};

/////////////////////////////////////////////////////
// Integer Constant

class LSLIntegerConstant : public LSLConstant {
  public:
    LSLIntegerConstant( ScriptContext *ctx, int v ) : LSLConstant(ctx), _mValue(v) { _mType = TYPE(LST_INTEGER); }

    virtual const char *getNodeName() {
      static thread_local char buf[256];
      snprintf(buf, 256, "integer constant: %d", _mValue);
      return buf;
    }

    virtual LSLNodeSubType getNodeSubType() { return NODE_INTEGER_CONSTANT; }

    int getValue() const { return _mValue; }
    virtual LSLConstant* copy(ScriptAllocator *allocator) {
      auto* new_const = allocator->newTracked<LSLIntegerConstant>(_mValue);
      new_const->_mConstantValue = new_const;
      return new_const;
    };

  private:
    int _mValue;
};


/////////////////////////////////////////////////////
// Float Constant

class LSLFloatConstant : public LSLConstant {
  public:
    LSLFloatConstant( ScriptContext *ctx, float v ) : LSLConstant(ctx), _mValue(v) { _mType = TYPE(LST_FLOATINGPOINT); }

    virtual const char *getNodeName() {
      static thread_local char buf[256];
      snprintf(buf, 256, "float constant: %f", _mValue);
      return buf;
    }

    virtual LSLNodeSubType getNodeSubType() { return NODE_FLOAT_CONSTANT; }

    float getValue() const { return _mValue; }
    virtual bool containsNaN();
    virtual LSLConstant* copy(ScriptAllocator *allocator) {
      auto* new_const = allocator->newTracked<LSLFloatConstant>(_mValue);
      new_const->_mConstantValue = new_const;
      return new_const;
    };

  private:
    float _mValue;
};


/////////////////////////////////////////////////////
// String Constant

class LSLStringConstant : public LSLConstant {
  public:
    LSLStringConstant( ScriptContext *ctx, const char *v ) : LSLConstant(ctx), _mValue(v) { _mType = TYPE(LST_STRING); }

    virtual const char *getNodeName() {
      static thread_local char buf[256];
      snprintf(buf, 256, "string constant: \"%s\"", escape_string(_mValue).c_str());
      return buf;
    }

    virtual LSLNodeSubType getNodeSubType() { return NODE_STRING_CONSTANT; }

    const char *getValue() { return _mValue; }
    virtual LSLConstant* copy(ScriptAllocator *allocator) {
      auto* new_const = allocator->newTracked<LSLStringConstant>(_mValue);
      new_const->_mConstantValue = new_const;
      return new_const;
    };

  private:
    const char *_mValue;
};


/////////////////////////////////////////////////////
// List Constant

class LSLListConstant : public LSLConstant {
  public:
    LSLListConstant( ScriptContext *ctx, class LSLConstant *v ) : LSLConstant(ctx) {
      _mType = TYPE(LST_LIST);
      // so we can do symbol resolution inside the list constant
      pushChild(v);
    }

    virtual const char *getNodeName() {
      static thread_local char buf[256];
      snprintf(buf, 256, "list constant: %d entries", getLength());
      return buf;
    }

    virtual LSLNodeSubType getNodeSubType() { return NODE_LIST_CONSTANT; }

    class LSLConstant *getValue() { return (LSLConstant*) getChildren(); }

    int getLength() { return (int) getNumChildren(); }

    virtual LSLConstant* copy(ScriptAllocator *allocator) {
      // TODO: probably actually have to copy all of the elements.
      //  since it will reparent the child values.
      auto* new_const = allocator->newTracked<LSLListConstant>(getValue());
      new_const->_mConstantValue = new_const;
      return new_const;
    };
};

/////////////////////////////////////////////////////
// Vector Constant

class LSLVectorConstant : public LSLConstant {
  public:
    LSLVectorConstant( ScriptContext *ctx, float x, float y, float z ): LSLConstant(ctx), _mValue({x, y, z}) {
      _mType = TYPE(LST_VECTOR);
    };

    virtual const char *getNodeName() {
      static thread_local char buf[256];
      snprintf(buf, 256, "vector constant: <%g, %g, %g>", _mValue.x, _mValue.y, _mValue.z);
      return buf;
    }

    virtual LSLNodeSubType getNodeSubType() { return NODE_VECTOR_CONSTANT; }

    Vector3 *getValue() { return &_mValue; }
    virtual bool containsNaN();

    virtual LSLConstant* copy(ScriptAllocator *allocator) {
      auto* new_const = allocator->newTracked<LSLVectorConstant>(_mValue.x, _mValue.y, _mValue.z);
      new_const->_mConstantValue = new_const;
      return new_const;
    };

  private:
    Vector3 _mValue;
};


/////////////////////////////////////////////////////
// Quaternion Constant

class LSLQuaternionConstant : public LSLConstant {
  public:
    LSLQuaternionConstant( ScriptContext *ctx, float x, float y, float z, float s ): LSLConstant(ctx), _mValue({x, y, z, s}) {
      _mType = TYPE(LST_QUATERNION);
    };

    virtual const char *getNodeName() {
      static thread_local char buf[256];
      snprintf(buf, 256, "quaternion constant: <%g, %g, %g, %g>", _mValue.x, _mValue.y, _mValue.z, _mValue.s);
      return buf;
    }

    virtual LSLNodeSubType getNodeSubType() { return NODE_QUATERNION_CONSTANT; }

    Quaternion *getValue() { return &_mValue; }
    virtual bool containsNaN();

    virtual LSLConstant* copy(ScriptAllocator *allocator) {
      auto* new_const = allocator->newTracked<LSLQuaternionConstant>(_mValue.x, _mValue.y, _mValue.z, _mValue.s);
      new_const->_mConstantValue = new_const;
      return new_const;
    };

  private:
    Quaternion _mValue;
};


class LSLGlobalFunction : public LSLASTNode {
  public:
    LSLGlobalFunction( ScriptContext *ctx, class LSLIdentifier *identifier, class LSLFunctionDec *decl, class LSLStatement *statement )
      : LSLASTNode( ctx, 3, identifier, decl, statement ) {
      _mSymbolTable = ctx->allocator->newTracked<LSLSymbolTable>();
    };
    virtual const char *getNodeName() { return "global func"; }
    virtual LSLNodeType getNodeType() { return NODE_GLOBAL_FUNCTION; };
    virtual LSLSymbol *getSymbol() {return ((LSLIdentifier *) getChild(0))->getSymbol(); }
};

class LSLParamList : public LSLASTNode {
  public:
    explicit LSLParamList( ScriptContext *ctx ) : LSLASTNode(ctx, 0) {};
    LSLParamList( ScriptContext *ctx, class LSLIdentifier *identifier ) : LSLASTNode(ctx, 1, identifier) {};
};

class LSLFunctionDec : public LSLParamList {
  public:
    explicit LSLFunctionDec(ScriptContext *ctx) : LSLParamList(ctx) {};
    LSLFunctionDec( ScriptContext *ctx, class LSLIdentifier *identifier ) : LSLParamList(ctx, identifier) {};
    virtual const char *getNodeName() { return "function decl"; }
    virtual LSLNodeType getNodeType() { return NODE_FUNCTION_DEC; };
};

class LSLEventDec : public LSLParamList {
  public:
    explicit LSLEventDec(ScriptContext *ctx) : LSLParamList(ctx) {};
    LSLEventDec( ScriptContext *ctx, class LSLIdentifier *identifier ) : LSLParamList(ctx, identifier) {};
    virtual const char *getNodeName() { return "event decl"; }
    virtual LSLNodeType getNodeType() { return NODE_EVENT_DEC; };
};

class LSLState : public LSLASTNode {
  public:
    LSLState( ScriptContext *ctx, class LSLIdentifier *identifier, class LSLEventHandler *state_body )
        : LSLASTNode( ctx, 2, identifier, state_body ) {
      _mSymbolTable = ctx->allocator->newTracked<LSLSymbolTable>();
    };
    virtual const char *getNodeName() { return "state"; }
    virtual LSLNodeType getNodeType() { return NODE_STATE; };
    virtual LSLSymbol *getSymbol() {return ((LSLIdentifier *) getChild(0))->getSymbol(); }
};

class LSLEventHandler : public LSLASTNode {
  public:
    LSLEventHandler( ScriptContext *ctx, class LSLIdentifier *identifier, class LSLEventDec *decl, class LSLStatement *body )
      : LSLASTNode(ctx, 3, identifier, decl, body) {
      _mSymbolTable = ctx->allocator->newTracked<LSLSymbolTable>();
    };
    virtual const char *getNodeName() { return "event handler"; }
    virtual LSLNodeType getNodeType() { return NODE_EVENT_HANDLER; };
    virtual LSLSymbol *getSymbol() {return ((LSLIdentifier *) getChild(0))->getSymbol(); }
};

class LSLStatement : public LSLASTNode {
  public:
    LSLStatement( ScriptContext *ctx, int num, ... ): LSLASTNode(ctx) {
      va_list ap;
      va_start(ap, num);
      addChildren(num, ap);
      va_end(ap);
    };
    LSLStatement( ScriptContext *ctx, class LSLExpression *expression ) : LSLASTNode(ctx, 1, expression) {};
    virtual const char *getNodeName() { return "statement"; }
    virtual LSLNodeType getNodeType() { return NODE_STATEMENT; };
};

class LSLCompoundStatement : public LSLStatement {
  public:
    LSLCompoundStatement( ScriptContext *ctx, class LSLStatement *body ) : LSLStatement(ctx, 1, body) {
      _mSymbolTable = ctx->allocator->newTracked<LSLSymbolTable>();
    }
    virtual const char *getNodeName() { return "compound statement"; };
    virtual LSLNodeSubType getNodeSubType() { return NODE_COMPOUND_STATEMENT; };
};

class LSLStateStatement : public LSLStatement {
  public:
    explicit LSLStateStatement(ScriptContext *ctx) : LSLStatement(ctx, 0) {};
    LSLStateStatement( ScriptContext *ctx, class LSLIdentifier *identifier ) : LSLStatement(ctx, 1, identifier) {};
    virtual const char *getNodeName() { return "setstate"; };
    virtual LSLNodeSubType getNodeSubType() { return NODE_STATE_STATEMENT; };
    virtual LSLSymbol *getSymbol() {return ((LSLIdentifier *) getChild(0))->getSymbol(); }
};

class LSLJumpStatement : public LSLStatement {
  public:
    LSLJumpStatement( ScriptContext *ctx, class LSLIdentifier *identifier ) : LSLStatement(ctx, 1, identifier) {};
    virtual const char *getNodeName() { return "jump"; };
    virtual LSLNodeSubType getNodeSubType() { return NODE_JUMP_STATEMENT; };
    virtual LSLSymbol *getSymbol() {return ((LSLIdentifier *) getChild(0))->getSymbol(); }
};

class LSLLabel : public LSLStatement {
  public:
    LSLLabel( ScriptContext *ctx, class LSLIdentifier *identifier ) : LSLStatement(ctx, 1, identifier) {};
    virtual const char *getNodeName() { return "label"; };
    virtual LSLNodeSubType getNodeSubType() { return NODE_LABEL; };
    virtual LSLSymbol *getSymbol() {return ((LSLIdentifier *) getChild(0))->getSymbol(); }
};

class LSLReturnStatement : public LSLStatement {
  public:
    LSLReturnStatement( ScriptContext *ctx, class LSLExpression *expression ) : LSLStatement(ctx, 1, expression) {};
    virtual const char *getNodeName() { return "return"; };
    virtual LSLNodeSubType getNodeSubType() { return NODE_RETURN_STATEMENT; };
};

class LSLIfStatement : public LSLStatement {
  public:
    LSLIfStatement( ScriptContext *ctx, class LSLExpression *expression, class LSLStatement *true_branch, class LSLStatement *false_branch)
      : LSLStatement( ctx, 3, expression, true_branch, false_branch ) {};
    virtual const char *getNodeName() { return "if"; };
    virtual LSLNodeSubType getNodeSubType() { return NODE_IF_STATEMENT; };
};

class LSLForStatement : public LSLStatement {
  public:
    LSLForStatement(ScriptContext *ctx, class LSLASTNodeList *init, class LSLExpression *condition,
                    class LSLASTNodeList *cont, class LSLStatement *body)
      : LSLStatement( ctx, 4, init, condition, cont, body ) {};
    virtual const char *getNodeName() { return "for"; };
    virtual LSLNodeSubType getNodeSubType() { return NODE_FOR_STATEMENT; };
};

class LSLDoStatement : public LSLStatement {
  public:
    LSLDoStatement( ScriptContext *ctx, class LSLStatement *body, class LSLExpression *condition )
      : LSLStatement(ctx, 2, body, condition) {};
    virtual const char *getNodeName() { return "do"; };
    virtual LSLNodeSubType getNodeSubType() { return NODE_DO_STATEMENT; };
};

class LSLWhileStatement : public LSLStatement {
  public:
    LSLWhileStatement( ScriptContext *ctx, class LSLExpression *condition, class LSLStatement *body )
      : LSLStatement(ctx, 2, condition, body) {};
    virtual const char *getNodeName() { return "while"; };
    virtual LSLNodeSubType getNodeSubType() { return NODE_WHILE_STATEMENT; };
};


class LSLDeclaration : public LSLStatement {
  public:
    LSLDeclaration(ScriptContext *ctx, class LSLIdentifier *identifier, class LSLExpression *value)
      : LSLStatement(ctx, 2, identifier, value) { };
    virtual const char *getNodeName() { return "declaration"; };
    virtual LSLNodeSubType getNodeSubType() { return NODE_DECLARATION; };

    virtual LSLConstant* getConstantValue();
    virtual LSLSymbol *getSymbol() {return ((LSLIdentifier *) getChild(0))->getSymbol(); }
};

class LSLExpression : public LSLASTNode {
public:
    explicit LSLExpression(ScriptContext *ctx) : LSLASTNode(ctx, 0), _mOperation(0) {};
    LSLExpression(ScriptContext *ctx, int num, ...): LSLASTNode(ctx), _mOperation(0) {
      va_list ap;
      va_start(ap, num);
      addChildren(num, ap);
      va_end(ap);
    };

    virtual const char *getNodeName() {
      return "base expression";
    };
    virtual LSLNodeType getNodeType() { return NODE_EXPRESSION; };

    virtual LSLConstant* getConstantValue();
    virtual bool nodeAllowsFolding() { return true; };
    int getOperation() const {return _mOperation;};
  protected:
    int _mOperation;
};


class LSLConstantExpression: public LSLExpression {
public:
    LSLConstantExpression( ScriptContext *ctx, LSLConstant* constant )
      : LSLExpression(ctx) {
      assert(constant);
      if (constant->isStatic())
        constant = constant->copy(ctx->allocator);
      pushChild(constant);
      _mConstantValue = constant;
    };

    virtual const char *getNodeName() {
      return "constant expression";
    };
    virtual LSLNodeSubType getNodeSubType() { return NODE_CONSTANT_EXPRESSION; };
};


class LSLParenthesisExpression: public LSLExpression {
public:
    LSLParenthesisExpression( ScriptContext *ctx, LSLExpression* expr )
      : LSLExpression(ctx, 1, expr) { _mOperation = '('; };

    virtual const char *getNodeName() {
      return "parenthesis expression";
    };
    virtual LSLNodeSubType getNodeSubType() { return NODE_PARENTHESIS_EXPRESSION; };
};


class LSLBinaryExpression : public LSLExpression {
public:
    LSLBinaryExpression( ScriptContext *ctx, LSLExpression *lvalue, int oper, LSLExpression *rvalue )
    : LSLExpression(ctx, 2, lvalue, rvalue) { _mOperation = oper; };

    virtual const char *getNodeName() {
      static thread_local char buf[256];
      snprintf( buf, 256, "binary expression: '%s'", operation_repr_str(_mOperation) );
      return buf;
    };
    virtual LSLNodeSubType getNodeSubType() { return NODE_BINARY_EXPRESSION; };
};

class LSLUnaryExpression : public LSLExpression {
public:
    LSLUnaryExpression( ScriptContext *ctx, LSLExpression *lvalue, int oper )
            : LSLExpression(ctx, 1, lvalue) { _mOperation = oper; };

    virtual const char *getNodeName() {
      static thread_local char buf[256];
      snprintf( buf, 256, "unary expression: '%s'", operation_repr_str(_mOperation) );
      return buf;
    };
    virtual LSLNodeSubType getNodeSubType() { return NODE_UNARY_EXPRESSION; };
};



class LSLTypecastExpression : public LSLExpression {
  public:
    LSLTypecastExpression(ScriptContext *ctx, LSLType *type, LSLExpression *expression )
      : LSLExpression(ctx, 1, expression) { _mType = type;};

    virtual const char *getNodeName() { return "typecast expression"; }
    virtual LSLNodeSubType getNodeSubType() { return NODE_TYPECAST_EXPRESSION; };
};

class LSLPrintExpression : public LSLExpression {
  public:
    LSLPrintExpression( ScriptContext *ctx, LSLExpression *expression )
      : LSLExpression( ctx, 1, expression ) { _mType = TYPE(LST_NULL); };
    virtual const char *getNodeName() { return "print() call"; }
    virtual LSLNodeSubType getNodeSubType() { return NODE_PRINT_EXPRESSION; };
};

class LSLFunctionExpression : public LSLExpression {
  public:
    LSLFunctionExpression( ScriptContext *ctx, LSLIdentifier *identifier )
      : LSLExpression( ctx, 1, identifier ) {};
    LSLFunctionExpression( ScriptContext *ctx, LSLIdentifier *identifier, LSLExpression *arguments )
      : LSLExpression( ctx, 2, identifier, arguments) {};
    virtual const char *getNodeName() { return "function call"; }
    virtual LSLNodeSubType getNodeSubType() { return NODE_FUNCTION_EXPRESSION; };

    virtual bool nodeAllowsFolding() { return false; };
    virtual LSLSymbol *getSymbol() {return ((LSLIdentifier *) getChild(0))->getSymbol(); }
};

class LSLVectorExpression : public LSLExpression {
  public:
    LSLVectorExpression(ScriptContext *ctx, LSLExpression *x, LSLExpression *y, LSLExpression *z )
      : LSLExpression(ctx, 3, x, y, z) { _mType = TYPE(LST_VECTOR); }
    explicit LSLVectorExpression(ScriptContext *ctx) : LSLExpression(ctx,0) {
      _mConstantValue = ctx->allocator->newTracked<LSLVectorConstant>(0.0f, 0.0f, 0.0f);
      _mType = TYPE(LST_VECTOR);
    }
    virtual const char *getNodeName() { return "vector expression"; }
    virtual LSLNodeSubType getNodeSubType() { return NODE_VECTOR_EXPRESSION; };
};

class LSLQuaternionExpression : public LSLExpression {
  public:
    LSLQuaternionExpression(ScriptContext *ctx, LSLExpression *x, LSLExpression *y, LSLExpression *z, LSLExpression *s )
      : LSLExpression(ctx, 4, x, y, z, s) { _mType = TYPE(LST_QUATERNION); };
    explicit LSLQuaternionExpression(ScriptContext *ctx) : LSLExpression(ctx,0) {
      _mConstantValue = ctx->allocator->newTracked<LSLQuaternionConstant>(0.0f, 0.0f, 0.0f, 0.0f);
      _mType = TYPE(LST_QUATERNION);
    };
    virtual const char *getNodeName() { return "quaternion expression"; };
    virtual LSLNodeSubType getNodeSubType() { return NODE_QUATERNION_EXPRESSION; };
};

class LSLListExpression : public LSLExpression {
  public:
    LSLListExpression( ScriptContext *ctx, LSLExpression *c ) : LSLExpression( ctx, 1, c ) { _mType = TYPE(LST_LIST); };

    virtual const char *getNodeName() { return "list expression"; };
    virtual LSLNodeSubType getNodeSubType() { return NODE_LIST_EXPRESSION; }
};

class LSLLValueExpression : public LSLExpression {
  public:
    LSLLValueExpression( ScriptContext *ctx, LSLIdentifier *identifier )
      : LSLExpression(ctx, 1, identifier), _mIsFoldable(false) {};
    LSLLValueExpression( ScriptContext *ctx, LSLIdentifier *identifier, LSLIdentifier *member )
        : LSLExpression(ctx, 2, identifier, member), _mIsFoldable(false) {};
    virtual const char *getNodeName() {
      static thread_local char buf[256];
      snprintf(buf, 256, "lvalue expression {%sfoldable}", _mIsFoldable ? "" : "not ");
      return buf;
    };
    virtual LSLNodeSubType getNodeSubType() { return NODE_LVALUE_EXPRESSION; };

    virtual LSLConstant* getConstantValue();
    void setIsFoldable(bool foldable) { _mIsFoldable = foldable;};
    bool getIsFoldable() const {return _mIsFoldable;};
  private:
    bool _mIsFoldable;
};

void tailslide_init_builtins(const char *builtins_file);

}

#endif