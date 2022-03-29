#pragma once

#include "lslmini.hh"
#include "visitor.hh"

namespace Tailslide {
class TypeCheckVisitor: public DepthFirstASTVisitor {
  public:
    virtual bool visit(LLASTNode *node);

    virtual bool visit(LLScriptGlobalVariable *node);
    virtual bool visit(LLScriptStateStatement *node);
    virtual bool visit(LLScriptDeclaration *node);
    virtual bool visit(LLScriptReturnStatement *node);
    virtual bool visit(LLScriptIfStatement *node);
    virtual bool visit(LLScriptForStatement *node);
    virtual bool visit(LLScriptDoStatement *node);
    virtual bool visit(LLScriptWhileStatement *node);
    virtual bool visit(LLScriptExpression *node);
    virtual bool visit(LLScriptEventHandler *node);
    virtual bool visit(LLScriptFunctionExpression *node);
    virtual bool visit(LLScriptLValueExpression *node);
    virtual bool visit(LLScriptTypecastExpression *node);
    virtual bool visit(LLScriptVectorExpression *node);
    virtual bool visit(LLScriptVectorConstant *node);
    virtual bool visit(LLScriptQuaternionExpression *node);
    virtual bool visit(LLScriptQuaternionConstant *node);
    virtual bool visit(LLScriptListConstant *node);
    virtual bool visit(LLScriptListExpression *node);
};
}
