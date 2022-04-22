#include "bytecode_compiler.hh"

namespace Tailslide {

inline int32_t calculate_jump_operand(uint64_t operand_pos, uint64_t target_pos) {
  return (int32_t)(target_pos - (operand_pos + sizeof(uint32_t)));
}

void LSOBytecodeCompiler::buildFunction(LSLASTNode *node) {
  auto &func_sym_data = _mSymData[node->getSymbol()];

  visitChildren(node);

  for (auto &jump_pair : _mJumpMap) {
    // Seek back to where the jump operand starts
    ScopedBitStreamSeek seek(mCodeBS, jump_pair.second);
    auto label_pos = _mLabelMap[jump_pair.first];
    // Write in the jump target relative to the end of the 32-bit operand
    mCodeBS << calculate_jump_operand(jump_pair.second, label_pos);
  }

  if (!func_sym_data.has_trailing_return)
    mCodeBS << LOPC_RETURN;
}

bool LSOBytecodeCompiler::visit(LSLEventHandler *node) {
  buildFunction(node);
  return false;
}

bool LSOBytecodeCompiler::visit(LSLGlobalFunction *node) {
  buildFunction(node);
  return false;
}

bool LSOBytecodeCompiler::visit(LSLConstantExpression *node) {
  auto *constant = node->getChild(0);
  mCodeBS << LSO_TYPE_LITERAL_PUSH_OPCODE[constant->getIType()];
  constant->visit(this);
  return false;
}

bool LSOBytecodeCompiler::visit(LSLConstant *node) {
  // TODO: add some kind of de-sugaring visitor for LSO to turn vec / quat / key / list
  //  constants back into expressions of the appropriate type.
  throw std::runtime_error(std::string("Bad constant type ") + node->getNodeName());
  return false;
}

bool LSOBytecodeCompiler::visit(LSLStringConstant *node) {
  auto *str = node->getValue();
  mCodeBS.writeRawData((const uint8_t*)str, strlen(str) + 1);
  return false;
}

bool LSOBytecodeCompiler::visit(LSLIntegerConstant *node) {
  mCodeBS << node->getValue();
  return false;
}

bool LSOBytecodeCompiler::visit(LSLFloatConstant *node) {
  mCodeBS << node->getValue();
  return false;
}

bool LSOBytecodeCompiler::visit(LSLExpressionStatement *node) {
  auto *expr = node->getChild(0);
  expr->visit(this);
  mCodeBS << LSO_TYPE_POP_OPCODE[expr->getIType()];
  return false;
}

bool LSOBytecodeCompiler::visit(LSLJumpStatement *node) {
  mCodeBS << LOPC_JUMP;
  _mJumpMap[node->getSymbol()->getName()] = mCodeBS.pos();
  // we don't know where the label is in the bytecode yet, so just put 0
  mCodeBS << (uint32_t)0;
  return false;
}

bool LSOBytecodeCompiler::visit(LSLLabel *node) {
  _mLabelMap[node->getSymbol()->getName()] = mCodeBS.pos();
  return false;
}

bool LSOBytecodeCompiler::visit(LSLIfStatement* node) {
  auto *expr = node->getChild(0);
  auto *true_branch = node->getChild(1);
  auto *false_branch = node->getChild(2);

  expr->visit(this);
  mCodeBS << LOPC_JUMPNIF << expr->getIType();
  uint64_t jump_to_false_pos = mCodeBS.pos();
  // fill with 0 until we know where the branch ends.
  mCodeBS << (int32_t)0;

  true_branch->visit(this);
  uint64_t true_branch_end = mCodeBS.pos();
  if (false_branch->getNodeType() != NODE_NULL) {
    // need to jump over the code for the false case after hitting the end of true
    mCodeBS << LOPC_JUMP;
    uint64_t jump_to_end_pos = mCodeBS.pos();
    mCodeBS << (int32_t)0;
    true_branch_end = mCodeBS.pos();

    false_branch->visit(this);
    uint64_t false_branch_end_pos = mCodeBS.pos();

    {
      // go back and write in how to jump over the false branch
      ScopedBitStreamSeek seek(mCodeBS, jump_to_end_pos);
      mCodeBS << calculate_jump_operand(jump_to_end_pos, false_branch_end_pos);
    }
  }

  {
    // go back and write in how to jump over the true branch
    ScopedBitStreamSeek seek(mCodeBS, jump_to_false_pos);
    mCodeBS << calculate_jump_operand(jump_to_false_pos, true_branch_end);
  }
  return false;
}

bool LSOBytecodeCompiler::visit(LSLForStatement *node) {
  // initializer expressions come first and are run unconditionally
  auto *init_expr = node->getChild(0)->getChildren();
  while (init_expr != nullptr) {
    init_expr->visit(this);
    mCodeBS << LSO_TYPE_POP_OPCODE[init_expr->getIType()];
    init_expr = init_expr->getNext();
  }

  // top of the loop has conditional jump to break the loop
  auto check_pos = mCodeBS.pos();
  auto *check_expr = node->getChild(1);
  check_expr->visit(this);
  mCodeBS << LOPC_JUMPNIF << check_expr->getIType();
  uint64_t false_jump_pos = mCodeBS.pos();
  mCodeBS << (int32_t)0;

  // followed by the body
  node->getChild(3)->visit(this);

  // followed by the increment expression list
  auto *incr_expr = node->getChild(2)->getChildren();
  while (incr_expr != nullptr) {
    incr_expr->visit(this);
    mCodeBS << LSO_TYPE_POP_OPCODE[incr_expr->getIType()];
    incr_expr = incr_expr->getNext();
  }

  // then the jump back to the check expression at the top
  mCodeBS << LOPC_JUMP << calculate_jump_operand(mCodeBS.pos(), check_pos);

  auto loop_end_pos = mCodeBS.pos();
  {
    // go back and add the actual pos of end of the loop to the conditional jump
    ScopedBitStreamSeek seek(mCodeBS, false_jump_pos);
    mCodeBS << calculate_jump_operand(false_jump_pos, loop_end_pos);
  }

  return false;
}

bool LSOBytecodeCompiler::visit(LSLWhileStatement *node) {
  // top of the loop has conditional jump to break the loop
  auto check_pos = mCodeBS.pos();
  auto *check_expr = node->getChild(0);
  check_expr->visit(this);
  mCodeBS << LOPC_JUMPNIF << check_expr->getIType();
  uint64_t false_jump_pos = mCodeBS.pos();
  mCodeBS << (int32_t)0;

  // followed by the body
  node->getChild(1)->visit(this);

  // then the jump back to the check expression at the top
  mCodeBS << LOPC_JUMP << calculate_jump_operand(mCodeBS.pos(), check_pos);

  auto loop_end_pos = mCodeBS.pos();
  {
    // go back and add the actual pos of end of the loop to the conditional jump
    ScopedBitStreamSeek seek(mCodeBS, false_jump_pos);
    mCodeBS << calculate_jump_operand(false_jump_pos, loop_end_pos);
  }

  return false;
}

bool LSOBytecodeCompiler::visit(LSLDoStatement *node) {
  auto start_pos = mCodeBS.pos();
  // body first
  node->getChild(0)->visit(this);
  // then evaluate the condition
  auto *check_expr = node->getChild(1);
  check_expr->visit(this);
  // then the conditional jump back to the top of the body
  mCodeBS << LOPC_JUMPIF << check_expr->getIType() << calculate_jump_operand(mCodeBS.pos(), start_pos);
  return false;
}

}
