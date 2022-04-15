#pragma once

#include "lslmini.hh"
#include "visitor.hh"

namespace Tailslide {
class TypeCheckVisitor: public DepthFirstASTVisitor {
  protected:
    virtual bool visit(LSLASTNode *node);
    virtual bool visit(LSLGlobalVariable *node);
    virtual bool visit(LSLDeclaration *node);
    virtual bool visit(LSLStateStatement *node);
    virtual bool visit(LSLReturnStatement *node);
    virtual bool visit(LSLIfStatement *node);
    virtual bool visit(LSLForStatement *node);
    virtual bool visit(LSLDoStatement *node);
    virtual bool visit(LSLWhileStatement *node);
    virtual bool visit(LSLExpression *node);
    virtual bool visit(LSLEventHandler *node);
    virtual bool visit(LSLFunctionExpression *node);
    virtual bool visit(LSLLValueExpression *node);
    virtual bool visit(LSLTypecastExpression *node);
    virtual bool visit(LSLVectorExpression *node);
    virtual bool visit(LSLQuaternionExpression *node);
    virtual bool visit(LSLListConstant *node);
    virtual bool visit(LSLListExpression *node);
    
    void handleDeclaration(LSLASTNode *node);
};
}
