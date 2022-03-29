#pragma once

#include "lslmini.hh"
#include "visitor.hh"

namespace Tailslide {

class DetermineReachableVisitor: public ASTVisitor {
  public:
    virtual bool visit(LLScriptGlobalFunction* node);
    virtual bool visit(LLScriptIfStatement *node);
};

}
