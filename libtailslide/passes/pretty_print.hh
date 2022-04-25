#ifndef TAILSLIDE_PRETTY_PRINT_HH
#define TAILSLIDE_PRETTY_PRINT_HH

#include <sstream>
#include "../lslmini.hh"
#include "../visitor.hh"

namespace Tailslide {

struct PrettyPrintOpts {
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
    : _mOpts(ctx), mTabs(0)
    {}

    int mTabs;

    virtual bool visit(LSLASTNode *node);
    virtual bool visit(LSLIdentifier *node);
    virtual bool visit(LSLType *node);
    virtual bool visit(LSLGlobalVariable *node);
    virtual bool visit(LSLIntegerConstant *node);
    virtual bool visit(LSLFloatConstant *node);
    virtual bool visit(LSLStringConstant *node);
    virtual bool visit(LSLKeyConstant *node);
    virtual bool visit(LSLListConstant *node);
    virtual bool visit(LSLVectorConstant *node);
    virtual bool visit(LSLQuaternionConstant *node);
    virtual bool visit(LSLGlobalFunction *node);
    virtual bool visit(LSLParamList *node);
    virtual bool visit(LSLState *node);
    virtual bool visit(LSLEventHandler *node);
    virtual bool visit(LSLStatement *node);
    virtual bool visit(LSLCompoundStatement *node);
    virtual bool visit(LSLStateStatement *node);
    virtual bool visit(LSLJumpStatement *node);
    virtual bool visit(LSLLabel *node);
    virtual bool visit(LSLReturnStatement *node);
    virtual bool visit(LSLIfStatement *node);
    virtual bool visit(LSLForStatement *node);
    virtual bool visit(LSLDoStatement *node);
    virtual bool visit(LSLWhileStatement *node);
    virtual bool visit(LSLDeclaration *node);
    virtual bool visit(LSLExpression *node);
    virtual bool visit(LSLLValueExpression *node);
    virtual bool visit(LSLTypecastExpression *node);
    virtual bool visit(LSLPrintExpression *node);
    virtual bool visit(LSLFunctionExpression *node);
    virtual bool visit(LSLVectorExpression *node);
    virtual bool visit(LSLQuaternionExpression *node);
    virtual bool visit(LSLListExpression *node);
    virtual bool visit(LSLGlobalStorage *node);
    virtual bool visit(LSLScript *node);

    void prettifySiblingsSep(LSLASTNode *node, const char *separator);
    void prettifyCoordinateMembers(LSLASTNode *node);
    void doTabs();
    std::stringstream mStream {};
protected:
    const PrettyPrintOpts &_mOpts;
};

}

#endif //TAILSLIDE_PRETTY_PRINT_HH
