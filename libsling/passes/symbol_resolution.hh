#ifndef SLING_SYMBOL_RESOLUTION_HH
#define SLING_SYMBOL_RESOLUTION_HH

#include "lslmini.hh"
#include "visitor.hh"

namespace Sling {

class SymbolResolutionVisitor : public ASTVisitor {
    virtual bool visit(LLScriptDeclaration *node);
    virtual bool visit(LLScriptGlobalVariable *node);
    virtual bool visit(LLScriptScript *node);
    virtual bool visit(LLScriptFunctionDec *node);
    virtual bool visit(LLScriptEventHandler *node);
    virtual bool visit(LLScriptEventDec *node);
    virtual bool visit(LLScriptLabel *node);
    virtual bool visit(LLScriptStateStatement *node);
    virtual bool visit(LLScriptLValueExpression *node);
    virtual bool visit(LLScriptFunctionExpression *node);
};

}
#endif //SLING_SYMBOL_RESOLUTION_HH
