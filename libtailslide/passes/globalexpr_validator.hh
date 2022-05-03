#pragma once

#include "../lslmini.hh"
#include "../visitor.hh"

namespace Tailslide {


// Only cares that global variable rvalue is some expression that can
// be evaluated at compile-time and doesn't mutate state.
class GlobalExprValidatingVisitor: public ASTVisitor {
  protected:
    virtual bool visit(LSLGlobalVariable *node);
    virtual bool visit(LSLFunctionExpression *node);
    // these aren't interesting for validation of globals
    virtual bool visit(LSLGlobalFunction *node) { return false; };
    virtual bool visit(LSLState *node) { return false; };
    // reset to `true` before checking each rvalue, if it's false after
    // visiting the rvalue then raise an error about non-constant expression.
    bool _mValidRValue = true;
};

// More strict form of above. Enforces the same restrictions on
// expressions for global variable assignments as LL's implementation.
// Useful if you care about linting with SL in mind.
class SimpleAssignableValidatingVisitor: public GlobalExprValidatingVisitor {
  public:
    explicit SimpleAssignableValidatingVisitor(bool mono_semantics)
        : GlobalExprValidatingVisitor(), _mMonoSemantics(mono_semantics) {}
  protected:
    virtual bool visit(LSLExpression *node);
    virtual bool visit(LSLUnaryExpression *node);
    virtual bool visit(LSLLValueExpression *node);
    bool _mMonoSemantics;
};

}
