#ifndef SLING_SYMBOL_RESOLUTION_HH
#define SLING_SYMBOL_RESOLUTION_HH

#include "lslmini.hh"
#include "visitor.hh"

namespace Sling {

class SymbolResolutionVisitor : public ASTVisitor {
    virtual bool visit(LLScriptDeclaration *node);
    virtual bool visit(LLScriptGlobalVariable *node);
    virtual bool visit(LLScriptState *node);
    virtual bool visit(LLScriptGlobalFunction *node);
    virtual bool visit(LLScriptFunctionDec *node);
    virtual bool visit(LLScriptEventHandler *node);
    virtual bool visit(LLScriptEventDec *node);
    virtual bool visit(LLScriptLabel *node);
};

}
#endif //SLING_SYMBOL_RESOLUTION_HH
