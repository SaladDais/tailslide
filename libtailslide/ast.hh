#ifndef TAILSLIDE_AST_HH
#define TAILSLIDE_AST_HH
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
  NODE_KEY_CONSTANT,
  NODE_VECTOR_CONSTANT,
  NODE_QUATERNION_CONSTANT,
  NODE_LIST_CONSTANT,

  NODE_COMPOUND_STATEMENT,
  NODE_EXPRESSION_STATEMENT,
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
  NODE_BOOL_CONVERSION_EXPRESSION,
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

class OptimizationOptions;
class ASTVisitor;

class LSLASTNode : public TrackableObject {
  public:
    explicit LSLASTNode(ScriptContext *ctx);

    LSLASTNode( ScriptContext *ctx, YYLTYPE *loc, int num, ... )
      : LSLASTNode(ctx) {
      _mLoc = *loc;
      va_list ap;
      va_start(ap, num);
      addChildren(num, ap);
      va_end(ap);
    }

    explicit LSLASTNode( ScriptContext *ctx, int num, ... ) : LSLASTNode(ctx) {
      va_list ap;
      va_start(ap, num);
      addChildren(num, ap);
      va_end(ap);
    }

    ~LSLASTNode() override = default;

    void addChildren(int num, va_list ap);

    LSLASTNode *getNext() { return _mNext; }
    LSLASTNode *getPrev() { return _mPrev; }
    LSLASTNode *getChildren() { return _mChildren; }
    LSLASTNode *getParent() { return _mParent; }

    LSLASTNode *getChild(int i) {
      LSLASTNode *c = _mChildren;
      while (i-- && c)
        c = c->getNext();
      return c;
    }

    size_t getNumChildren() const {
      LSLASTNode *c = _mChildren;
      size_t num = 0;
      // empty children (NODE_NULL) are still considered valid.
      while (c) {
        c = c->getNext();
        ++num;
      }
      return num;
    };

    // Get the topmost node in the tree
    LSLASTNode *getRoot() {
      LSLASTNode *last_node = this;
      LSLASTNode *test_root;
      while ((test_root = last_node->getParent()) != nullptr)
        last_node = test_root;
      return last_node;
    }

    /// Get our position within our parents' children
    int getParentSlot();

    LSLASTNode *newNullNode();

    /* Set our parent, and make sure all our siblings do too. */
    void setParent(LSLASTNode *newparent );
    // Add child to end of list.
    void pushChild(LSLASTNode *child);
    /* Set our next sibling, and ensure it links back to us. */
    void setNext(LSLASTNode *newnext);
    /* Set our previous sibling, and ensure it links back to us. */
    void setPrev(LSLASTNode *newprev);
    void addNextSibling(LSLASTNode *sibling);
    /* remove a child from the list of nodes, shifting other children up */
    void removeChild(LSLASTNode *child);
    /* replace a node from the list of children with null, returning it */
    LSLASTNode *takeChild(int child_num);

    // replace an arbitrary node with another, setting
    // prev, next and parent as appropriate
    static void replaceNode(LSLASTNode *old_node, LSLASTNode *replacement);


    void                setType(LSLType *type) { _mType = type;   }
    class LSLType *getType()                    { return _mType;    }
    LSLIType getIType();


    void markStatic() { _mStaticNode = true;}
    bool isStatic() const {return _mStaticNode;}

    /// passes                  ///
    // generic visitor functions
    void visit(ASTVisitor *visitor);

    // Convenience methods for common visitor uses
    void collectSymbols();
    void determineTypes();
    void propagateValues(bool create_heap_values=true);
    void checkBestPractices();
    void checkSymbols(); // look for unused symbols, etc

    /// symbol functions        ///
    virtual LSLSymbol *lookupSymbol(const char *name, LSLSymbolType type );
    void            defineSymbol(LSLSymbol *symbol );
    LSLSymbolTable *getSymbolTable() { return _mSymbolTable; }
    void setSymbolTable(LSLSymbolTable *table) {_mSymbolTable = table;}


    YYLTYPE     *getLoc()     { return &_mLoc; };
    void setLoc(YYLTYPE *yylloc) { _mLoc = *yylloc; };


    // Set whether this node is allowed to be a declaration,
    // usually due to occurring in a conditional statement without a new scope.
    void setDeclarationAllowed(bool allowed) { _mDeclarationAllowed = allowed; };
    bool getDeclarationAllowed() const { return _mDeclarationAllowed; };

    // bad hacks for figuring out if something is _really_ in scope
    class LSLASTNode* findPreviousInScope(std::function<bool (class LSLASTNode *)> const &checker);
    class LSLASTNode* findDescInScope(std::function<bool (class LSLASTNode *)> const &checker);


    /// identification          ///
    virtual const char  *getNodeName() { return "node";    };
    virtual LSLNodeType  getNodeType() { return NODE_NODE; };
    virtual LSLNodeSubType getNodeSubType() { return NODE_NO_SUB_TYPE; }

    /// constants ///
    virtual class LSLConstant  *getConstantValue()    { return _mConstantValue; };
    void setConstantValue(class LSLConstant *cv) {
      if (cv)
        _mConstantPrecluded = false;
      _mConstantValue = cv;
    };
    bool            isConstant()           { return getConstantValue() != nullptr; };
    void setConstantPrecluded(bool precluded) { _mConstantPrecluded = precluded; };
    bool getConstantPrecluded() const { return _mConstantPrecluded; };
    virtual bool nodeAllowsFolding() { return false; };
    virtual LSLSymbol *getSymbol() { return nullptr; }

    bool getSynthesized() const { return _mSynthesized; };
    void setSynthesized(bool synthesized) { _mSynthesized = synthesized; };

  protected:
    bool _mSynthesized = false;
    class LSLType          *_mType;
    LSLSymbolTable         *_mSymbolTable;
    class LSLConstant      *_mConstantValue;
    bool                   _mConstantPrecluded = false;

  private:
    YYLTYPE                      _mLoc {0};

    LSLASTNode *_mChildren = nullptr;
    LSLASTNode *_mChildrenTail = nullptr;
    LSLASTNode *_mParent;
    LSLASTNode *_mNext;
    LSLASTNode *_mPrev;

  protected:
    bool                        _mDeclarationAllowed;
    bool                        _mStaticNode = false;
};

class LSLASTNullNode : public LSLASTNode {
  public:
    explicit LSLASTNullNode(ScriptContext *ctx): LSLASTNode(ctx) {};
    virtual const char *getNodeName() { return "null"; };
    virtual LSLNodeType getNodeType() { return NODE_NULL; };
};

class LSLASTNodeList : public LSLASTNode {
  public:
    explicit LSLASTNodeList(ScriptContext *ctx) : LSLASTNode(ctx, 0) {};
    LSLASTNodeList(ScriptContext *ctx, class LSLASTNode *nodes ) : LSLASTNode(ctx, 1, nodes) {};
    virtual const char *getNodeName() { return "ast node list"; }
    virtual LSLNodeType getNodeType() { return NODE_AST_NODE_LIST; };
};

}

#endif
