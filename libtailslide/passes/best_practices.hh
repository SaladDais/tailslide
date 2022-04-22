#pragma once

#include "lslmini.hh"
#include "visitor.hh"

namespace Tailslide {

class BestPracticesVisitor: public ASTVisitor {
  public:
    virtual bool visit(LSLGlobalFunction* node);
    virtual bool visit(LSLIfStatement *node);
    virtual bool visit(LSLBinaryExpression *node);
    virtual bool visit(LSLExpressionStatement *node);
};

}
