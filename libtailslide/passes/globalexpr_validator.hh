#pragma once

#include "lslmini.hh"
#include "visitor.hh"

namespace Tailslide {


// Only cares that global variable rvalue is some expression that can
// be evaluated at compile-time and doesn't mutate state.
class GlobalExprValidatingVisitor: public ASTVisitor {
  protected:
    virtual bool visit(LLScriptGlobalVariable *node);
    virtual bool visit(LLScriptFunctionExpression *node);
    // reset to `true` before checking each rvalue, if it's false after
    // visiting the rvalue then raise an error about non-constant expression.
    bool valid_rvalue = true;
};

// More strict form of above. Enforces the same restrictions on
// expressions for global variable assignments as LL's implementation.
// Useful if you care about linting with SL in mind.
class SimpleAssignableValidatingVisitor: public GlobalExprValidatingVisitor {
  protected:
    virtual bool visit(LLScriptExpression *node);
    virtual bool visit(LLScriptUnaryExpression *node);
    virtual bool visit(LLScriptLValueExpression *node);
};

}
