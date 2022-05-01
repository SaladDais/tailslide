#pragma once

#include <vector>

#include "../../lslmini.hh"
#include "../../visitor.hh"
#include "../../bitstream.hh"
#include "resource_collector.hh"

namespace Tailslide {

class MonoScriptCompiler : public ASTVisitor {
  public:
    explicit MonoScriptCompiler(ScriptAllocator *allocator) : _mAllocator(allocator) {};
    std::stringstream mCIL {};
  protected:
    virtual bool visit(LSLScript *node);
    virtual bool visit(LSLGlobalVariable *node);

    void pushLValueContainer(LSLLValueExpression *lvalue);
    void pushLValue(LSLLValueExpression *lvalue);
    void pushConstant(LSLConstant *cv);
    void pushFloatLiteral(float value);
    void storeToLValue(LSLLValueExpression *lvalue, bool push_result);
    void castTopOfStack(LSLIType from_type, LSLIType to_type);
    std::string getGlobalVarSpecifier(LSLSymbol *sym);
    std::string getLValueAccessorSpecifier(LSLLValueExpression *lvalue);

    virtual bool visit(LSLEventHandler *node);
    virtual bool visit(LSLGlobalFunction *node);
    void buildFunction(LSLASTNode *node);

    virtual bool visit(LSLExpressionStatement *node);
    virtual bool visit(LSLReturnStatement *node);
    virtual bool visit(LSLLabel *node);
    virtual bool visit(LSLJumpStatement *node);
    virtual bool visit(LSLDeclaration *node);
    virtual bool visit(LSLIfStatement *node);
    virtual bool visit(LSLForStatement *node);
    virtual bool visit(LSLWhileStatement *node);
    virtual bool visit(LSLDoStatement *node);
    virtual bool visit(LSLStateStatement *node);

    virtual bool visit(LSLConstantExpression *node);
    virtual bool visit(LSLTypecastExpression *node);
    virtual bool visit(LSLBoolConversionExpression *node);
    virtual bool visit(LSLVectorExpression *node);
    virtual bool visit(LSLQuaternionExpression *node);
    virtual bool visit(LSLLValueExpression *node);
    virtual bool visit(LSLListExpression *node);
    virtual bool visit(LSLFunctionExpression *node);
    virtual bool visit(LSLBinaryExpression *node);
    void compileBinaryExpression(int op, LSLExpression *left, LSLExpression *right, LSLIType ret_type);
    virtual bool visit(LSLUnaryExpression *node);
    virtual bool visit(LSLPrintExpression *node);

    ScriptAllocator *_mAllocator;
    MonoSymbolDataMap _mSymData {};
    LSLSymbol *_mCurrentFuncSym = nullptr;
    std::string _mScriptClassName;
    bool _mInGlobalExpr = false;
    uint32_t _mJumpNum = 0;
};

const char * const CIL_TYPE_NAMES[LST_MAX] = {
    "void",
    "int32",
    "float32",
    "string",
    "valuetype [ScriptTypes]LindenLab.SecondLife.Key",
    "class [ScriptTypes]LindenLab.SecondLife.Vector",
    "class [ScriptTypes]LindenLab.SecondLife.Quaternion",
    "class [mscorlib]System.Collections.ArrayList",
    "ERROR"
};

const char * const CIL_VALUE_TYPE_NAMES[LST_MAX] = {
    "void",
    "int32",
    "float32",
    "string",
    "valuetype [ScriptTypes]LindenLab.SecondLife.Key",
    "valuetype [ScriptTypes]LindenLab.SecondLife.Vector",
    "valuetype [ScriptTypes]LindenLab.SecondLife.Quaternion",
    "class [mscorlib]System.Collections.ArrayList",
    "ERROR"
};

const char * const CIL_BOXING_INSTRUCTIONS[LST_MAX] = {
    "ERROR!\n", // void
    "box [mscorlib]System.Int32\n",
    "box [mscorlib]System.Single\n",
    "", // string not a value type, it's already boxed.
    "box [ScriptTypes]LindenLab.SecondLife.Key\n",
    "box [ScriptTypes]LindenLab.SecondLife.Vector\n",
    "box [ScriptTypes]LindenLab.SecondLife.Quaternion\n",
    "", // list not a value type, it's already boxed.
    "ERROR!\n" // error
};

}
