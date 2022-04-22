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
    bool visit(LSLConstant *node) override;
    bool visit(LSLIntegerConstant *node) override;
    bool visit(LSLFloatConstant *node) override;
    bool visit(LSLStringConstant *node) override;
    bool visit(LSLExpressionStatement *node) override;
    bool visit(LSLJumpStatement *node) override;
    bool visit(LSLLabel *node) override;
    bool visit(LSLIfStatement *node) override;
    bool visit(LSLForStatement *node) override;
    bool visit(LSLWhileStatement *node) override;
    bool visit(LSLDoStatement *node) override;

    LSOSymbolDataMap &_mSymData;
    /// map of label name -> position of the jump instruction. Intentionally
    /// disallows multiple jumps to match LL LSO compiler behavior.
    std::map<std::string, uint64_t> _mJumpMap;
    /// map of label symbol ptr to its location in the bytecode
    std::map<std::string, uint64_t> _mLabelMap;

  public:
    LSOBitStream mCodeBS {ENDIAN_BIG};
};

}
