#pragma once

#include "visitor.hh"

namespace Tailslide {

/// Takes compound assignments and de-structures them, takes places where
/// implicit typecasting happens and adds explicit typecast nodes.
/// Doesn't currently wrap in parenthesis, so not appropriate for pretty-printing.
class DeSugaringVisitor : public ASTVisitor {
  public:
    explicit DeSugaringVisitor(ScriptAllocator *allocator) : _mAllocator(allocator) {}
  protected:
    bool visit(LSLConstantExpression *node) override;
    bool visit(LSLBinaryExpression *node) override;
    bool visit(LSLUnaryExpression *node) override;
    bool visit(LSLDeclaration *node) override;
    bool visit(LSLVectorExpression *node) override;
    bool visit(LSLQuaternionExpression *node) override;
    bool visit(LSLFunctionExpression *node) override;
    bool visit(LSLReturnStatement *node) override;
    bool visit(LSLLValueExpression *node) override;

    void handleCoordinateNode(LSLASTNode *node);
    void maybeInjectCast(LSLExpression *expr, LSLType *to);

    ScriptAllocator *_mAllocator;
};

}
