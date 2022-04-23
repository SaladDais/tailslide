#pragma once

#include <map>
#include <string>

#include "bytecode_format.hh"
#include "resource_collector.hh"
#include "visitor.hh"

namespace Tailslide {

class LSOBytecodeCompiler : public ASTVisitor {
  public:
    explicit LSOBytecodeCompiler(LSOSymbolDataMap &symbol_data_map) : _mSymData (symbol_data_map) {}

  protected:
    void buildFunction(LSLASTNode *node);
    bool visit(LSLEventHandler *node) override;
    bool visit(LSLGlobalFunction *node) override;

    bool visit(LSLConstantExpression *node) override;
    bool visit(LSLVectorExpression *node) override;
    bool visit(LSLQuaternionExpression *node) override;
    bool visit(LSLListExpression *node) override;
    bool visit(LSLUnaryExpression *node) override;
    bool visit(LSLBinaryExpression *node) override;
    bool visit(LSLLValueExpression *node) override;
    bool visit(LSLTypecastExpression *node) override;
    bool visit(LSLPrintExpression *node) override;
    bool visit(LSLFunctionExpression *node) override;

    bool visit(LSLExpressionStatement *node) override;
    bool visit(LSLJumpStatement *node) override;
    bool visit(LSLLabel *node) override;
    bool visit(LSLIfStatement *node) override;
    bool visit(LSLForStatement *node) override;
    bool visit(LSLWhileStatement *node) override;
    bool visit(LSLDoStatement *node) override;
    bool visit(LSLDeclaration *node) override;
    bool visit(LSLReturnStatement *node) override;
    bool visit(LSLStateStatement *node) override;

    void pushConstant(LSLConstant *constant);
    int32_t calculateLValueOffset(LSLLValueExpression *node);
    void storeStackToLValue(LSLLValueExpression *lvalue);
    void popLocals();
    void writeReturn();

    LSOSymbolDataMap &_mSymData;
    LSOSymbolData *_mFuncSymData = nullptr;
    /// map of label name -> position of the jump instruction. Intentionally
    /// disallows multiple jumps to match LL LSO compiler behavior.
    std::map<std::string, uint64_t> _mJumpMap;
    /// map of label symbol ptr to its location in the bytecode
    std::map<std::string, uint64_t> _mLabelMap;

  public:
    LSOBitStream mCodeBS {ENDIAN_BIG};
};

}
