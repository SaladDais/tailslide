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
    virtual bool visit(LSLDeclaration *decl_stmt);
    virtual bool visit(LSLGlobalVariable *glob_var);
    virtual bool visit(LSLGlobalFunction *glob_func);
    virtual bool visit(LSLLValueExpression *lvalue);
    virtual bool visit(LSLFunctionExpression *func_expr);
    virtual bool visit(LSLScript *script);
    virtual bool visit(LSLFunctionDec *func_dec);
    virtual bool visit(LSLEventHandler *handler);
    virtual bool visit(LSLEventDec *event_dec);
    virtual bool visit(LSLLabel *label_stmt);
    virtual bool visit(LSLJumpStatement *jump_stmt);
    virtual bool visit(LSLStateStatement *state_stmt);
    virtual bool visit(LSLCompoundStatement *compound_stmt);

    void replaceSymbolTable(LSLASTNode *node);

    void resolvePendingJumps();
    ScriptAllocator *_mAllocator;
    std::vector<LSLIdentifier*> _mPendingJumps;
    std::vector<LSLIdentifier*> _mCollectedLabels;
    bool _mLindenJumpSemantics;
};

}
#endif //TAILSLIDE_SYMBOL_RESOLUTION_HH
