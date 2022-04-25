#ifndef TAILSLIDE_SYMBOL_RESOLUTION_HH
#define TAILSLIDE_SYMBOL_RESOLUTION_HH

#include <vector>

#include "../lslmini.hh"
#include "../visitor.hh"

namespace Tailslide {

class BaseSymbolResolutionVisitor : public ASTVisitor {
  public:
    explicit BaseSymbolResolutionVisitor(ScriptAllocator *allocator): _mAllocator(allocator) {};
  protected:
    virtual bool visit(LSLLValueExpression *node);
    virtual bool visit(LSLFunctionExpression *node);

    void replaceSymbolTable(LSLASTNode *node);

    ScriptAllocator *_mAllocator;
};

class SymbolResolutionVisitor : public BaseSymbolResolutionVisitor {
  public:
    SymbolResolutionVisitor(bool linden_jump_semantics, ScriptAllocator *allocator)
      : _mLindenJumpSemantics(linden_jump_semantics), BaseSymbolResolutionVisitor(allocator) {}

  protected:
    virtual bool visit(LSLDeclaration *node);
    virtual bool visit(LSLGlobalVariable *node);
    virtual bool visit(LSLGlobalFunction *node);
    virtual bool visit(LSLScript *node);
    virtual bool visit(LSLFunctionDec *node);
    virtual bool visit(LSLEventHandler *node);
    virtual bool visit(LSLEventDec *node);
    virtual bool visit(LSLLabel *node);
    virtual bool visit(LSLJumpStatement *node);
    virtual bool visit(LSLStateStatement *node);
    virtual bool visit(LSLCompoundStatement *node);

    void resolvePendingJumps();
    std::vector<LSLIdentifier*> _mPendingJumps;
    std::vector<LSLIdentifier*> _mCollectedLabels;
    bool _mLindenJumpSemantics;
};

}
#endif //TAILSLIDE_SYMBOL_RESOLUTION_HH
