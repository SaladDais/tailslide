#ifndef SLING_VISITOR_HH
#define SLING_VISITOR_HH

#include "lslmini.hh"

namespace Sling {

class ASTVisitor {
  public:
    virtual bool visit(LLASTNode *node) { return true; }
    virtual bool visit(LLASTNullNode *node) { return false; }
    virtual bool visit(LLScriptScript *node) {
      return visit((LLASTNode *)node);
    };
    virtual bool visit(LLScriptGlobalStorage *node) {
      return visit((LLASTNode *)node);
    };
    virtual bool visit(LLScriptIdentifier *node) {
      return visit((LLASTNode *)node);
    };
    virtual bool visit(LLScriptGlobalVariable *node) {
      return visit((LLASTNode *)node);
    };
    virtual bool visit(LLScriptConstant *node) {
      return visit((LLASTNode *)node);
    };
    virtual bool visit(LLScriptIntegerConstant *node) {
      return visit((LLScriptConstant *)node);
    };
    virtual bool visit(LLScriptFloatConstant *node) {
      return visit((LLScriptConstant *)node);
    };
    virtual bool visit(LLScriptStringConstant *node) {
      return visit((LLScriptConstant *)node);
    };
    virtual bool visit(LLScriptListConstant *node) {
      return visit((LLScriptConstant *)node);
    };
    virtual bool visit(LLScriptVectorConstant *node) {
      return visit((LLScriptConstant *)node);
    };
    virtual bool visit(LLScriptQuaternionConstant *node) {
      return visit((LLScriptConstant *)node);
    };
    virtual bool visit(LLScriptGlobalFunction *node) {
      return visit((LLASTNode *)node);
    };
    virtual bool visit(LLScriptParamList *node) {
      return visit((LLASTNode *)node);
    };
    virtual bool visit(LLScriptFunctionDec *node) {
      return visit((LLScriptParamList *)node);
    };
    virtual bool visit(LLScriptEventDec *node) {
      return visit((LLScriptParamList *)node);
    };
    virtual bool visit(LLScriptForExpressionList *node) {
      return visit((LLASTNode *)node);
    };
    virtual bool visit(LLScriptState *node) {
      return visit((LLASTNode *)node);
    };
    virtual bool visit(LLScriptEventHandler *node) {
      return visit((LLASTNode *)node);
    };
    virtual bool visit(LLScriptStatement *node) {
      return visit((LLASTNode *)node);
    };
    virtual bool visit(LLScriptCompoundStatement *node) {
      return visit((LLScriptStatement *)node);
    };
    virtual bool visit(LLScriptStateStatement *node) {
      return visit((LLScriptStatement *)node);
    };
    virtual bool visit(LLScriptJumpStatement *node) {
      return visit((LLScriptStatement *)node);
    };
    virtual bool visit(LLScriptLabel *node) {
      return visit((LLScriptStatement *)node);
    };
    virtual bool visit(LLScriptReturnStatement *node) {
      return visit((LLScriptStatement *)node);
    };
    virtual bool visit(LLScriptIfStatement *node) {
      return visit((LLScriptStatement *)node);
    };
    virtual bool visit(LLScriptForStatement *node) {
      return visit((LLScriptStatement *)node);
    };
    virtual bool visit(LLScriptDoStatement *node) {
      return visit((LLScriptStatement *)node);
    };
    virtual bool visit(LLScriptWhileStatement *node) {
      return visit((LLScriptStatement *)node);
    };
    virtual bool visit(LLScriptDeclaration *node) {
      return visit((LLScriptStatement *)node);
    };
    virtual bool visit(LLScriptExpression *node) {
      return visit((LLASTNode *)node);
    };
    virtual bool visit(LLScriptBinaryExpression *node) {
      return visit((LLScriptExpression *)node);
    };
    virtual bool visit(LLScriptUnaryExpression *node) {
      return visit((LLScriptExpression *)node);
    };
    virtual bool visit(LLScriptConstantExpression *node) {
      return visit((LLScriptExpression *)node);
    };
    virtual bool visit(LLScriptParenthesisExpression *node) {
      return visit((LLScriptExpression *)node);
    };
    virtual bool visit(LLScriptTypecastExpression *node) {
      return visit((LLScriptExpression *)node);
    };
    virtual bool visit(LLScriptPrintExpression *node) {
      return visit((LLScriptExpression *)node);
    };
    virtual bool visit(LLScriptFunctionExpression *node) {
      return visit((LLScriptExpression *)node);
    };
    virtual bool visit(LLScriptVectorExpression *node) {
      return visit((LLScriptExpression *)node);
    };
    virtual bool visit(LLScriptQuaternionExpression *node) {
      return visit((LLScriptExpression *)node);
    };
    virtual bool visit(LLScriptListExpression *node) {
      return visit((LLScriptExpression *)node);
    };
    virtual bool visit(LLScriptLValueExpression *node) {
      return visit((LLScriptExpression *)node);
    };
    virtual bool visit(LLScriptType *node) {
      return visit((LLASTNode*)node);
    }

    virtual bool visit_specific(LLASTNode *node);
    void visit_children(LLASTNode *node);
    // only used for depth-first visitors
    virtual bool before_descend(LLASTNode *node) {return true;}
    virtual bool is_depth_first() {return false;}
};

class DepthFirstASTVisitor: public ASTVisitor {
    virtual bool is_depth_first() {return true;}
};

}

#endif //SLING_VISITOR_HH
