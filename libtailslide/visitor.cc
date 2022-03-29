#include "lslmini.hh"
#include "visitor.hh"

namespace Tailslide {


bool ASTVisitor::visit_specific(LLASTNode *node) {
  switch(node->get_node_type()) {
    case NODE_NODE:
      return visit(node);
    case NODE_NULL:
      return visit((LLASTNullNode *)node);
    case NODE_SCRIPT:
      return visit((LLScriptScript *)node);
    case NODE_GLOBAL_STORAGE:
      return visit((LLScriptGlobalStorage *)node);
    case NODE_GLOBAL_FUNCTION:
      return visit((LLScriptGlobalFunction *)node);
    case NODE_GLOBAL_VARIABLE:
      return visit((LLScriptGlobalVariable *)node);
    case NODE_IDENTIFIER:
      return visit((LLScriptIdentifier *)node);
    case NODE_CONSTANT: {
      switch(node->get_node_sub_type()) {
        case NODE_INTEGER_CONSTANT:
          return visit((LLScriptIntegerConstant *)node);
        case NODE_FLOAT_CONSTANT:
          return visit((LLScriptFloatConstant *)node);
        case NODE_STRING_CONSTANT:
          return visit((LLScriptStringConstant *)node);
        case NODE_VECTOR_CONSTANT:
          return visit((LLScriptVectorConstant *)node);
        case NODE_QUATERNION_CONSTANT:
          return visit((LLScriptQuaternionConstant *)node);
        case NODE_LIST_CONSTANT:
          return visit((LLScriptListConstant *)node);
        default:
          return visit((LLScriptConstant *)node);
      }
    }
    case NODE_FUNCTION_DEC:
      return visit((LLScriptFunctionDec *)node);
    case NODE_EVENT_DEC:
      return visit((LLScriptEventDec *)node);
    case NODE_FOR_EXPRESSION_LIST:
      return visit((LLScriptForExpressionList *)node);
    case NODE_STATE:
      return visit((LLScriptState *)node);
    case NODE_EVENT_HANDLER:
      return visit((LLScriptEventHandler *)node);
    case NODE_STATEMENT:
      switch(node->get_node_sub_type()) {
        case NODE_COMPOUND_STATEMENT:
          return visit((LLScriptCompoundStatement *)node);
        case NODE_RETURN_STATEMENT:
          return visit((LLScriptReturnStatement *)node);
        case NODE_LABEL:
          return visit((LLScriptLabel *)node);
        case NODE_JUMP_STATEMENT:
          return visit((LLScriptJumpStatement *)node);
        case NODE_IF_STATEMENT:
          return visit((LLScriptIfStatement *)node);
        case NODE_FOR_STATEMENT:
          return visit((LLScriptForStatement *)node);
        case NODE_DO_STATEMENT:
          return visit((LLScriptDoStatement *)node);
        case NODE_WHILE_STATEMENT:
          return visit((LLScriptWhileStatement *)node);
        case NODE_DECLARATION:
          return visit((LLScriptDeclaration *)node);
        case NODE_STATE_STATEMENT:
          return visit((LLScriptStateStatement *)node);
        default:
          return visit((LLScriptStatement *)node);
      }
    case NODE_EXPRESSION:
      switch(node->get_node_sub_type()) {
        case NODE_TYPECAST_EXPRESSION:
          return visit((LLScriptTypecastExpression *)node);
        case NODE_PRINT_EXPRESSION:
          return visit((LLScriptPrintExpression *)node);
        case NODE_FUNCTION_EXPRESSION:
          return visit((LLScriptFunctionExpression *)node);
        case NODE_VECTOR_EXPRESSION:
          return visit((LLScriptVectorExpression *)node);
        case NODE_QUATERNION_EXPRESSION:
          return visit((LLScriptQuaternionExpression *)node);
        case NODE_LIST_EXPRESSION:
          return visit((LLScriptListExpression *)node);
        case NODE_LVALUE_EXPRESSION:
          return visit((LLScriptLValueExpression *)node);
        case NODE_PARENTHESIS_EXPRESSION:
          return visit((LLScriptParenthesisExpression *)node);
        case NODE_BINARY_EXPRESSION:
          return visit((LLScriptBinaryExpression *)node);
        case NODE_UNARY_EXPRESSION:
          return visit((LLScriptUnaryExpression *)node);
        case NODE_CONSTANT_EXPRESSION:
          return visit((LLScriptConstantExpression *)node);
        default:
          return visit((LLScriptExpression *)node);
      }
    case NODE_TYPE:
      return visit((LLScriptType *)node);
  }
  return visit(node);
}

void ASTVisitor::visit_children(LLASTNode *node) {
  LLASTNode *child = node->get_children();

  while (child != nullptr) {
    LLASTNode *next = child->get_next();
    assert(child != next);
    assert(child != node);
    child->visit(this);
    child = next;
  }
}

}
