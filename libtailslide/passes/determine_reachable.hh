#pragma once

#include "lslmini.hh"
#include "visitor.hh"

namespace Tailslide {

class DetermineReachableVisitor: public ASTVisitor {
  public:
    virtual bool visit(LSLGlobalFunction* node);
    virtual bool visit(LSLIfStatement *node);
};

}
