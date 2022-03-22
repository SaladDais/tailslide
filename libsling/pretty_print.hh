#ifndef LSLINT_PRETTY_PRINT_HH
#define LSLINT_PRETTY_PRINT_HH

#include <sstream>
#include "lslmini.hh"

class PrettyPrintOpts {
public:
    bool minify_whitespace;
    bool mangle_local_names;
    bool mangle_func_names;
    bool mangle_global_names;
    bool show_unmangled;
    bool obfuscate_numbers;
};

class PrettyPrintVisitor: public ASTVisitor {
public:
    explicit PrettyPrintVisitor(const PrettyPrintOpts& ctx)
    : opts(ctx), tabs(0)
    {}

    int tabs;

    virtual bool visit(LLASTNode *node);
    virtual bool visit(LLScriptIdentifier *node);
    virtual bool visit(LLScriptType *node);
    virtual bool visit(LLScriptGlobalVariable *node);
    virtual bool visit(LLScriptIntegerConstant *node);
    virtual bool visit(LLScriptFloatConstant *node);
    virtual bool visit(LLScriptStringConstant *node);
    virtual bool visit(LLScriptListConstant *node);
    virtual bool visit(LLScriptVectorConstant *node);
    virtual bool visit(LLScriptQuaternionConstant *node);
    virtual bool visit(LLScriptGlobalFunction *node);
    virtual bool visit(LLScriptParamList *node);
    virtual bool visit(LLScriptForExpressionList *node);
    virtual bool visit(LLScriptState *node);
    virtual bool visit(LLScriptEventHandler *node);
    virtual bool visit(LLScriptStatement *node);
    virtual bool visit(LLScriptCompoundStatement *node);
    virtual bool visit(LLScriptStateStatement *node);
    virtual bool visit(LLScriptJumpStatement *node);
    virtual bool visit(LLScriptLabel *node);
    virtual bool visit(LLScriptReturnStatement *node);
    virtual bool visit(LLScriptIfStatement *node);
    virtual bool visit(LLScriptForStatement *node);
    virtual bool visit(LLScriptDoStatement *node);
    virtual bool visit(LLScriptWhileStatement *node);
    virtual bool visit(LLScriptDeclaration *node);
    virtual bool visit(LLScriptExpression *node);
    virtual bool visit(LLScriptTypecastExpression *node);
    virtual bool visit(LLScriptPrintExpression *node);
    virtual bool visit(LLScriptFunctionExpression *node);
    virtual bool visit(LLScriptVectorExpression *node);
    virtual bool visit(LLScriptQuaternionExpression *node);
    virtual bool visit(LLScriptListExpression *node);
    virtual bool visit(LLScriptGlobalStorage *node);
    virtual bool visit(LLScriptScript *node);
    virtual bool visit(LLScriptSimpleAssignable *node);

    void prettify_siblings_sep(LLASTNode *node, const char *separator);
    void do_tabs();
    static std::string escape_str(const char *str);
    std::stringstream stream {};
protected:
    const PrettyPrintOpts &opts;
};

#endif //LSLINT_PRETTY_PRINT_HH
