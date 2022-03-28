#ifndef SLING_SYMBOL_RESOLUTION_HH
#define SLING_SYMBOL_RESOLUTION_HH

#include "lslmini.hh"
#include "visitor.hh"

#include <vector>

namespace Sling {

class ExprSymbolResolutionVisitor : public ASTVisitor {
    virtual bool visit(LLScriptLValueExpression *node);
    virtual bool visit(LLScriptFunctionExpression *node);
};

class SymbolResolutionVisitor : public ExprSymbolResolutionVisitor {
    virtual bool visit(LLScriptDeclaration *node);
    virtual bool visit(LLScriptGlobalVariable *node);
    virtual bool visit(LLScriptGlobalFunction *node);
    virtual bool visit(LLScriptScript *node);
    virtual bool visit(LLScriptFunctionDec *node);
    virtual bool visit(LLScriptEventHandler *node);
    virtual bool visit(LLScriptEventDec *node);
    virtual bool visit(LLScriptLabel *node);
    virtual bool visit(LLScriptJumpStatement *node);
    virtual bool visit(LLScriptStateStatement *node);

    void _resolve_pending_jumps();
    std::vector<LLScriptIdentifier*> _pending_jump_labels;
};

}
#endif //SLING_SYMBOL_RESOLUTION_HH
