#pragma once

#include "../visitor.hh"

namespace Tailslide {

/// Takes compound assignments and de-structures them, takes places where
/// implicit typecasting happens and adds explicit typecast nodes.
/// Doesn't currently wrap in parenthesis, so not appropriate for pretty-printing.
class DeSugaringVisitor : public ASTVisitor {
  public:
    explicit DeSugaringVisitor(ScriptAllocator *allocator, bool mono_semantics)
        : _mAllocator(allocator), _mMonoSemantics(mono_semantics) {}
  protected:
    bool visit(LSLBinaryExpression *node) override;
    bool visit(LSLUnaryExpression *node) override;
    bool visit(LSLDeclaration *node) override;
    bool visit(LSLGlobalVariable *node) override;
    bool visit(LSLVectorExpression *node) override;
    bool visit(LSLQuaternionExpression *node) override;
    bool visit(LSLFunctionExpression *node) override;
    bool visit(LSLLValueExpression *node) override;

    bool visit(LSLReturnStatement *node) override;
    bool visit(LSLForStatement *node) override;
    bool visit(LSLWhileStatement *node) override;
    bool visit(LSLDoStatement *node) override;
    bool visit(LSLIfStatement *node) override;

    virtual LSLASTNode *rewriteBuiltinLValue(LSLASTNode *node);
    void handleCoordinateExpression(LSLASTNode *node);
    void maybeInjectCast(LSLExpression *expr, LSLType *to);
    void maybeInjectBoolConversion(LSLExpression *expr);

    ScriptAllocator *_mAllocator;
    bool _mMonoSemantics;
};

// Desugaring that also rewrites the tree to match how LL's compiler
// would have parsed a given node. Results in less optimal bytecode.
class LLConformantDeSugaringVisitor : public DeSugaringVisitor {
  public:
    explicit LLConformantDeSugaringVisitor(ScriptAllocator *allocator, bool mono_semantics)
        : DeSugaringVisitor(allocator, mono_semantics) {};
  protected:
    bool visit(LSLConstantExpression *node) override;

    LSLASTNode *rewriteBuiltinLValue(LSLASTNode *node) override;
};

}
