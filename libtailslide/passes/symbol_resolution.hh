#ifndef TAILSLIDE_SYMBOL_RESOLUTION_HH
#define TAILSLIDE_SYMBOL_RESOLUTION_HH

#include "lslmini.hh"
#include "visitor.hh"

#include <vector>

namespace Tailslide {

class ExprSymbolResolutionVisitor : public ASTVisitor {
    virtual bool visit(LSLLValueExpression *node);
    virtual bool visit(LSLFunctionExpression *node);
};

class SymbolResolutionVisitor : public ExprSymbolResolutionVisitor {
  public:
    SymbolResolutionVisitor(bool linden_jump_semantics) : _linden_jump_semantics(linden_jump_semantics) {};

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

    void _resolve_pending_jumps();
    std::vector<LSLIdentifier*> _pending_jumps;
    std::vector<LSLIdentifier*> _collected_labels;
    bool _linden_jump_semantics;
};

}
#endif //TAILSLIDE_SYMBOL_RESOLUTION_HH
