#ifndef TAILSLIDE_VISITOR_HH
#define TAILSLIDE_VISITOR_HH

#include "lslmini.hh"

namespace Tailslide {

class ASTVisitor {
  public:
    virtual bool visit(LSLASTNode *node) { return true; }
    virtual bool visit(LSLASTNullNode *node) { return false; }
    virtual bool visit(LSLASTNodeList *node) { return true; };
    virtual bool visit(LSLScript *node) {
      return visit((LSLASTNode *)node);
    };
    virtual bool visit(LSLGlobalStorage *node) {
      return visit((LSLASTNode *)node);
    };
    virtual bool visit(LSLIdentifier *node) {
      return visit((LSLASTNode *)node);
    };
    virtual bool visit(LSLGlobalVariable *node) {
      return visit((LSLASTNode *)node);
    };
    virtual bool visit(LSLConstant *node) {
      return visit((LSLASTNode *)node);
    };
    virtual bool visit(LSLIntegerConstant *node) {
      return visit((LSLConstant *)node);
    };
    virtual bool visit(LSLFloatConstant *node) {
      return visit((LSLConstant *)node);
    };
    virtual bool visit(LSLStringConstant *node) {
      return visit((LSLConstant *)node);
    };
    virtual bool visit(LSLKeyConstant *node) {
      return visit((LSLConstant *)node);
    };
    virtual bool visit(LSLListConstant *node) {
      return visit((LSLConstant *)node);
    };
    virtual bool visit(LSLVectorConstant *node) {
      return visit((LSLConstant *)node);
    };
    virtual bool visit(LSLQuaternionConstant *node) {
      return visit((LSLConstant *)node);
    };
    virtual bool visit(LSLGlobalFunction *node) {
      return visit((LSLASTNode *)node);
    };
    virtual bool visit(LSLParamList *node) {
      return visit((LSLASTNode *)node);
    };
    virtual bool visit(LSLFunctionDec *node) {
      return visit((LSLParamList *)node);
    };
    virtual bool visit(LSLEventDec *node) {
      return visit((LSLParamList *)node);
    };
    virtual bool visit(LSLState *node) {
      return visit((LSLASTNode *)node);
    };
    virtual bool visit(LSLEventHandler *node) {
      return visit((LSLASTNode *)node);
    };
    virtual bool visit(LSLStatement *node) {
      return visit((LSLASTNode *)node);
    };
    virtual bool visit(LSLCompoundStatement *node) {
      return visit((LSLStatement *)node);
    };
    virtual bool visit(LSLExpressionStatement *node) {
      return visit((LSLStatement *)node);
    };
    virtual bool visit(LSLStateStatement *node) {
      return visit((LSLStatement *)node);
    };
    virtual bool visit(LSLJumpStatement *node) {
      return visit((LSLStatement *)node);
    };
    virtual bool visit(LSLLabel *node) {
      return visit((LSLStatement *)node);
    };
    virtual bool visit(LSLReturnStatement *node) {
      return visit((LSLStatement *)node);
    };
    virtual bool visit(LSLIfStatement *node) {
      return visit((LSLStatement *)node);
    };
    virtual bool visit(LSLForStatement *node) {
      return visit((LSLStatement *)node);
    };
    virtual bool visit(LSLDoStatement *node) {
      return visit((LSLStatement *)node);
    };
    virtual bool visit(LSLWhileStatement *node) {
      return visit((LSLStatement *)node);
    };
    virtual bool visit(LSLDeclaration *node) {
      return visit((LSLStatement *)node);
    };
    virtual bool visit(LSLExpression *node) {
      return visit((LSLASTNode *)node);
    };
    virtual bool visit(LSLBinaryExpression *node) {
      return visit((LSLExpression *)node);
    };
    virtual bool visit(LSLUnaryExpression *node) {
      return visit((LSLExpression *)node);
    };
    virtual bool visit(LSLConstantExpression *node) {
      return visit((LSLExpression *)node);
    };
    virtual bool visit(LSLParenthesisExpression *node) {
      return visit((LSLExpression *)node);
    };
    virtual bool visit(LSLTypecastExpression *node) {
      return visit((LSLExpression *)node);
    };
    virtual bool visit(LSLPrintExpression *node) {
      return visit((LSLExpression *)node);
    };
    virtual bool visit(LSLFunctionExpression *node) {
      return visit((LSLExpression *)node);
    };
    virtual bool visit(LSLVectorExpression *node) {
      return visit((LSLExpression *)node);
    };
    virtual bool visit(LSLQuaternionExpression *node) {
      return visit((LSLExpression *)node);
    };
    virtual bool visit(LSLListExpression *node) {
      return visit((LSLExpression *)node);
    };
    virtual bool visit(LSLLValueExpression *node) {
      return visit((LSLExpression *)node);
    };
    virtual bool visit(LSLType *node) {
      return visit((LSLASTNode*)node);
    }

    virtual bool visitSpecific(LSLASTNode *node);
    void visitChildren(LSLASTNode *node);
    // only used for depth-first visitors
    virtual bool beforeDescend(LSLASTNode *node) {return true;}
    virtual bool isDepthFirst() {return false;}
};

class DepthFirstASTVisitor: public ASTVisitor {
    virtual bool isDepthFirst() {return true;}
};

}

#endif //TAILSLIDE_VISITOR_HH
