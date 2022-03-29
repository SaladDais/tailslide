#ifndef TAILSLIDE_VALUES_HH
#define TAILSLIDE_VALUES_HH

#include "lslmini.hh"
#include "visitor.hh"

namespace Tailslide {
class ConstantDeterminingVisitor : public DepthFirstASTVisitor {
  public:
    virtual bool before_descend(LLASTNode *node);

    virtual bool visit(LLScriptScript *node);
    virtual bool visit(LLScriptDeclaration *node);
    virtual bool visit(LLScriptExpression *node);
    virtual bool visit(LLScriptGlobalVariable *node);
    virtual bool visit(LLScriptLValueExpression *node);
    virtual bool visit(LLScriptListExpression *node);
    virtual bool visit(LLScriptVectorExpression *node);
    virtual bool visit(LLScriptQuaternionExpression *node);
    virtual bool visit(LLScriptTypecastExpression *node);
};
}

#endif //TAILSLIDE_VALUES_HH
