#ifndef TAILSLIDE_SYMBOL_RESOLUTION_HH
#define TAILSLIDE_SYMBOL_RESOLUTION_HH

#include <vector>

#include "../lslmini.hh"
#include "../visitor.hh"

namespace Tailslide {

class SymbolResolutionVisitor : public ASTVisitor {
  public:
    SymbolResolutionVisitor(bool linden_jump_semantics, ScriptAllocator *allocator)
      : _mLindenJumpSemantics(linden_jump_semantics), _mAllocator(allocator) {}

  protected:
    virtual bool visit(LSLDeclaration *node);
    virtual bool visit(LSLGlobalVariable *node);
    virtual bool visit(LSLGlobalFunction *node);
    virtual bool visit(LSLLValueExpression *node);
    virtual bool visit(LSLFunctionExpression *node);
    virtual bool visit(LSLScript *node);
    virtual bool visit(LSLFunctionDec *node);
    virtual bool visit(LSLEventHandler *node);
    virtual bool visit(LSLEventDec *node);
    virtual bool visit(LSLLabel *node);
    virtual bool visit(LSLJumpStatement *node);
    virtual bool visit(LSLStateStatement *node);
    virtual bool visit(LSLCompoundStatement *node);

    void replaceSymbolTable(LSLASTNode *node);

    void resolvePendingJumps();
    ScriptAllocator *_mAllocator;
    std::vector<LSLIdentifier*> _mPendingJumps;
    std::vector<LSLIdentifier*> _mCollectedLabels;
    bool _mLindenJumpSemantics;
};

}
#endif //TAILSLIDE_SYMBOL_RESOLUTION_HH
