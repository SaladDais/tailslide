#ifndef TAILSLIDE_VALUES_HH
#define TAILSLIDE_VALUES_HH

#include "lslmini.hh"
#include "visitor.hh"
#include "operations.hh"

namespace Tailslide {
class ConstantDeterminingVisitor : public DepthFirstASTVisitor {
  public:
    explicit ConstantDeterminingVisitor(AOperationBehavior *behavior, ScriptAllocator *allocator)
        : _mOperationBehavior(behavior), _mAllocator(allocator) {}

    virtual bool beforeDescend(LSLASTNode *node);

    virtual bool visit(LSLScript *node);
    virtual bool visit(LSLDeclaration *node);
    virtual bool visit(LSLExpression *node);
    virtual bool visit(LSLGlobalVariable *node);
    virtual bool visit(LSLLValueExpression *node);
    virtual bool visit(LSLListExpression *node);
    virtual bool visit(LSLVectorExpression *node);
    virtual bool visit(LSLQuaternionExpression *node);
    virtual bool visit(LSLTypecastExpression *node);
  protected:
    AOperationBehavior *_mOperationBehavior = nullptr;
    ScriptAllocator *_mAllocator;
};
}

#endif //TAILSLIDE_VALUES_HH
