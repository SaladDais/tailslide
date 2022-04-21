#include "bytecode_compiler.hh"

namespace Tailslide {

void LSOBytecodeCompiler::buildFunction(LSLASTNode *node) {
  auto &func_sym_data = _mSymData[node->getSymbol()];

  visitChildren(node);

  for (auto &jump_pair : _mJumpMap) {
    // Seek back to where the jump operand starts
    ScopedBitStreamSeek seek(mCodeBS, jump_pair.second);
    auto label_pos = _mLabelMap[jump_pair.first];
    // Write in the jump target relative to the end of the 32-bit operand
    mCodeBS << (int32_t)(label_pos - (jump_pair.second + sizeof(uint32_t)));
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

}
