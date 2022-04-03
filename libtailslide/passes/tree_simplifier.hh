#ifndef TAILSLIDE_TREE_SIMPLIFIER_HH
#define TAILSLIDE_TREE_SIMPLIFIER_HH

#include "visitor.hh"

namespace Tailslide {

class OptimizationContext {
  public:
    bool fold_constants = false;
    bool prune_unused_locals = false;
    bool prune_unused_globals = false;
    bool prune_unused_functions = false;
    bool may_create_new_strs = false;
    explicit operator bool() const {
      return fold_constants || prune_unused_functions || prune_unused_locals || prune_unused_globals;
    }
};

class TreeSimplifyingVisitor: public ASTVisitor {
  public:
    explicit TreeSimplifyingVisitor(const OptimizationContext &ctx): ctx(ctx) {};
    OptimizationContext ctx;
    int folded_total = 0;

    virtual bool visit(LSLDeclaration* node);
    virtual bool visit(LSLGlobalStorage* node);
    virtual bool visit(LSLExpression* node);
    virtual bool visit(LSLLValueExpression *node);
    virtual bool visit(LSLConstantExpression *node);
};
}


#endif //TAILSLIDE_TREE_SIMPLIFIER_HH
