#include "bytecode_compiler.hh"

namespace Tailslide {

inline int32_t calculate_jump_operand(uint64_t operand_pos, uint64_t target_pos) {
  return (int32_t)(target_pos - (operand_pos + sizeof(uint32_t)));
}

void LSOBytecodeCompiler::buildFunction(LSLASTNode *node) {
  auto *sym = node->getSymbol();
  _mFuncSymData = &_mSymData[sym];

  visitChildren(node);

  for (auto &jump_pair : _mJumpMap) {
    // Seek back to where the jump operand starts
    ScopedBitStreamSeek seek(mCodeBS, jump_pair.second);
    auto label_pos = _mLabelMap[jump_pair.first];
    // Write in the jump target relative to the end of the 32-bit operand
    mCodeBS << calculate_jump_operand(jump_pair.second, label_pos);
  }

  if (!sym->getAllPathsReturn()) {
    writeReturn();
  }
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
  pushConstant(node->getChild(0)->getConstantValue());
  return false;
}

bool LSOBytecodeCompiler::visit(LSLVectorExpression *node) {
  // the children just get pushed as floats from left to right,
  // so we can use the base child visitation logic.
  return true;
}

bool LSOBytecodeCompiler::visit(LSLQuaternionExpression *node) {
  // same as above
  return true;
}

bool LSOBytecodeCompiler::visit(LSLListExpression *node) {
  auto *child = node->getChildren();
  while (child) {
    child->visit(this);
    mCodeBS << LOPC_PUSHARGB << child->getIType();
    child = child->getNext();
  }
  mCodeBS << LOPC_STACKTOL << (uint32_t)node->getNumChildren();
  return false;
}

bool LSOBytecodeCompiler::visit(LSLUnaryExpression *node) {
  int op = node->getOperation();
  auto *lhs = node->getChild(0);
  auto lhs_itype = lhs->getIType();
  auto lhs_type = lhs->getType();

  lhs->visit(this);
  switch(op) {
    case '-': mCodeBS << LOPC_NEG << lhs_itype; break;
    // integer-only, doesn't take a type.
    case '!': mCodeBS << LOPC_BOOLNOT; break;
    case '~': mCodeBS << LOPC_BITNOT; break;
    case DEC_POST_OP:
    case INC_POST_OP:
      pushConstant(lhs_type->getOneValue());
      // need to push another copy onto the stack so we can keep the original
      lhs->visit(this);
      if (op == INC_POST_OP)
        mCodeBS << LOPC_ADD;
      else
        mCodeBS << LOPC_SUB;
      mCodeBS << pack_lso_types(lhs_itype, lhs_itype);
      storeStackToLValue((LSLLValueExpression *) lhs);
      // pop the mutated lvalue off the stack, old lvalue should now be on top.
      mCodeBS << LSO_TYPE_POP_OPCODE[lhs_itype];
    default:
      return false;
  }
  return false;
}

bool LSOBytecodeCompiler::visit(LSLBinaryExpression *node) {
  int op = node->getOperation();
  auto *lhs = node->getChild(0);
  auto *rhs = node->getChild(1);
  auto lhs_type = lhs->getIType();
  auto rhs_type = rhs->getIType();
  auto packed_types = pack_lso_types(lhs_type, rhs_type);
  if (op == '=') {
    rhs->visit(this);
    storeStackToLValue((LSLLValueExpression *) lhs);
    return false;
  } else if (op == MUL_ASSIGN) {
    // Must be that pesky `int *= float` which we can't decouple. Replicate the broken behavior.
    rhs->visit(this);
    lhs->visit(this);
    mCodeBS << LOPC_MUL << pack_lso_types(LST_FLOATINGPOINT, LST_FLOATINGPOINT);
    storeStackToLValue((LSLLValueExpression *) lhs);
    return false;
  }

  rhs->visit(this);
  lhs->visit(this);
  switch(op) {
    case '+': mCodeBS << LOPC_ADD << packed_types; break;
    case '-': mCodeBS << LOPC_SUB << packed_types; break;
    case '*': mCodeBS << LOPC_MUL << packed_types; break;
    case '/': mCodeBS << LOPC_DIV << packed_types; break;
    case '%': mCodeBS << LOPC_MOD << packed_types; break;
    case EQ:  mCodeBS << LOPC_EQ << packed_types;  break;
    case NEQ: mCodeBS << LOPC_NEQ << packed_types; break;
    case GEQ: mCodeBS << LOPC_GEQ << packed_types; break;
    case LEQ: mCodeBS << LOPC_LEQ << packed_types; break;
    case '<': mCodeBS << LOPC_LESS << packed_types; break;
    case '>': mCodeBS << LOPC_GREATER << packed_types; break;
    // these have no type argument since they only work on ints
    case '|': mCodeBS << LOPC_BITOR; break;
    case '&': mCodeBS << LOPC_BITAND; break;
    case '^': mCodeBS << LOPC_BITXOR; break;
    case BOOLEAN_AND: mCodeBS << LOPC_BOOLAND; break;
    case BOOLEAN_OR: mCodeBS << LOPC_BOOLOR; break;
    default:
      break;
  }
  return false;
}

bool LSOBytecodeCompiler::visit(LSLLValueExpression *node) {
  if (node->getSymbol()->getSubType() == SYM_GLOBAL) {
    switch(node->getIType()) {
      case LST_INTEGER:
      case LST_FLOATINGPOINT:
        mCodeBS << LOPC_PUSHG;
        break;
      case LST_STRING:
      case LST_KEY:
        mCodeBS << LOPC_PUSHGS;
        break;
      case LST_VECTOR:
        mCodeBS << LOPC_PUSHGV;
        break;
      case LST_QUATERNION:
        mCodeBS << LOPC_PUSHGQ;
        break;
      case LST_LIST:
        mCodeBS << LOPC_PUSHGL;
        break;
      case LST_ERROR:
      case LST_MAX:
      case LST_NULL:
        return false;
    }
  } else {
    switch(node->getIType()) {
      case LST_INTEGER:
      case LST_FLOATINGPOINT:
        mCodeBS << LOPC_PUSH;
        break;
      case LST_STRING:
      case LST_KEY:
        mCodeBS << LOPC_PUSHS;
        break;
      case LST_VECTOR:
        mCodeBS << LOPC_PUSHV;
        break;
      case LST_QUATERNION:
        mCodeBS << LOPC_PUSHQ;
        break;
      case LST_LIST:
        mCodeBS << LOPC_PUSHL;
        break;
      case LST_ERROR:
      case LST_MAX:
      case LST_NULL:
        return false;
    }
  }
  mCodeBS << calculateLValueOffset(node);
  return false;
}

bool LSOBytecodeCompiler::visit(LSLTypecastExpression *node) {
  auto *expr = node->getChild(0);
  expr->visit(this);
  mCodeBS << LOPC_CAST << pack_lso_types(expr->getIType(), node->getIType());
  return false;
}

bool LSOBytecodeCompiler::visit(LSLPrintExpression *node) {
  auto *expr = node->getChild(0);
  expr->visit(this);
  mCodeBS << LOPC_PRINT << expr->getIType();
  return false;
}

bool LSOBytecodeCompiler::visit(LSLFunctionExpression *node) {
  auto *func_sym = node->getSymbol();
  auto &func_sym_data = _mSymData[func_sym];

  // need to push empty space onto the stack for the callee to write in the retval
  switch(func_sym->getIType()) {
    // most only need 4 bytes of space
    case LST_INTEGER:
    case LST_FLOATINGPOINT:
    case LST_STRING:
    case LST_KEY:
    case LST_LIST:
      mCodeBS << LOPC_PUSHE;
      break;
    case LST_VECTOR:
      mCodeBS << LOPC_PUSHEV;
      break;
    case LST_QUATERNION:
      mCodeBS << LOPC_PUSHEQ;
      break;
    // void and friends need no space for a retval.
    default:
      break;
  }
  // TODO: what's this empty for?
  mCodeBS << LOPC_PUSHE;
  // keep old base pointer on stack so caller can pop it on return
  mCodeBS << LOPC_PUSHBP;
  // function calls are `identifier, [arg1, arg2, ...]`
  auto *child_expr = node->getChild(1);
  while (child_expr != nullptr) {
    // push arguments on to the stack
    child_expr->visit(this);
    child_expr = child_expr->getNext();
  }
  // make space for locals
  mCodeBS << LOPC_PUSHARGE << (func_sym_data.size - func_sym_data.offset);
  // calculate and set the new base pointer to locals
  mCodeBS << LOPC_PUSHSP;
  mCodeBS << LOPC_PUSHARGI << func_sym_data.size;
  mCodeBS << LOPC_ADD << pack_lso_types(LST_INTEGER, LST_INTEGER);
  mCodeBS << LOPC_POPBP;
  if (func_sym->getSubType() == SYM_BUILTIN)
    mCodeBS << LOPC_CALLLIB_TWO_BYTE << (uint16_t)func_sym_data.index;
  else
    mCodeBS << LOPC_CALL << (uint32_t)func_sym_data.index;
  return false;
}

bool LSOBytecodeCompiler::visit(LSLExpressionStatement *node) {
  auto *expr = node->getChild(0);
  expr->visit(this);
  if (auto pop_opcode = LSO_TYPE_POP_OPCODE[expr->getIType()])
    mCodeBS << pop_opcode;
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
    if (auto pop_opcode = LSO_TYPE_POP_OPCODE[init_expr->getIType()])
      mCodeBS << pop_opcode;
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
    if (auto pop_opcode = LSO_TYPE_POP_OPCODE[incr_expr->getIType()])
      mCodeBS << pop_opcode;
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

bool LSOBytecodeCompiler::visit(LSLDeclaration *node) {
  auto *expr = node->getChild(1);
  auto *var_type = node->getSymbol()->getType();
  if (expr->getNodeType() != NODE_NULL) {
    expr->visit(this);
  } else {
    auto *default_type = var_type;
    // This gets pushed as an int in the default case, even though it
    // would normally use pushargf.
    if (default_type->getIType() == LST_FLOATINGPOINT) {
      default_type = TYPE(LST_INTEGER);
    }
    pushConstant(default_type->getDefaultValue());
  }

  // use the appropriate opcode to load the top of the stack into a local offset
  mCodeBS << LSO_TYPE_LOAD_LOCAL_OPCODE[var_type->getIType()] << _mSymData[node->getSymbol()].offset;
  return false;
}

bool LSOBytecodeCompiler::visit(LSLReturnStatement *node) {
  auto *expr = node->getChild(0);
  // To match LL's LSO compiler we need to _not_ auto-cast when the expression
  // is promotable to the return type but not an exact match. Yuck.
  if (expr->getNodeSubType() == NODE_TYPECAST_EXPRESSION) {
    auto *cast_expr = (LSLTypecastExpression *) expr;
    if (cast_expr->getSynthesized())
      expr = cast_expr->getChild(0);
  }
  auto var_size = LSO_TYPE_DATA_SIZES[expr->getIType()];
  if (var_size) {
    expr->visit(this);
    // write in the retval above the locals
    int32_t retval_offset = -8 - (int32_t)var_size;
    mCodeBS << LSO_TYPE_LOAD_LOCAL_OPCODE[expr->getIType()] << retval_offset;
  }
  writeReturn();
  return false;
}

bool LSOBytecodeCompiler::visit(LSLStateStatement *node) {
  popLocals();
  mCodeBS << LOPC_STATE << (uint32_t)_mSymData[node->getSymbol()].index;
  return false;
}


void LSOBytecodeCompiler::pushConstant(LSLConstant *constant) {
  switch(constant->getIType()) {
    case LST_INTEGER:
      mCodeBS << LOPC_PUSHARGI << ((LSLIntegerConstant *) constant)->getValue();
      break;
    case LST_FLOATINGPOINT:
      mCodeBS << LOPC_PUSHARGF << ((LSLFloatConstant *) constant)->getValue();
      break;
    // will be disambiguated in the list case by PUSH_ARGB below.
    case LST_STRING:
    case LST_KEY: {
      auto *str = ((LSLStringConstant *) constant)->getValue();
      mCodeBS << LOPC_PUSHARGS;
      mCodeBS.writeRawData((uint8_t *)str, strlen(str) + 1);
      break;
    }
    case LST_VECTOR:
      mCodeBS << LOPC_PUSHARGV << *((LSLVectorConstant *) constant)->getValue();
      break;
    case LST_QUATERNION:
      mCodeBS << LOPC_PUSHARGQ << *((LSLQuaternionConstant *) constant)->getValue();
      break;
    case LST_LIST: {
      auto *list_cv = (LSLListConstant *)constant;
      auto *list_child = constant->getChildren();
      while (list_child != nullptr) {
        // push the constant, then its type so STACKTOL knows what's actually on the stack.
        pushConstant((LSLConstant *)list_child);
        mCodeBS << LOPC_PUSHARGB << list_child->getIType();
        list_child = list_child->getNext();
      }
      mCodeBS << LOPC_STACKTOL << (uint32_t)list_cv->getLength();
      break;
    }
    case LST_ERROR:
    case LST_MAX:
    case LST_NULL:
      break;
  }
}

void LSOBytecodeCompiler::popLocals() {
  // pop locals in reverse order
  for (auto i = _mFuncSymData->locals.rbegin(); i != _mFuncSymData->locals.rend(); ++i) {
    mCodeBS << LSO_TYPE_POP_OPCODE[*i];
  }
  // then the arguments that're above the locals on the stack
  for (auto i = _mFuncSymData->function_args.rbegin(); i != _mFuncSymData->function_args.rend(); ++i) {
    mCodeBS << LSO_TYPE_POP_OPCODE[*i];
  }
}

void LSOBytecodeCompiler::writeReturn() {
  popLocals();
  mCodeBS << LOPC_RETURN;
}

int32_t LSOBytecodeCompiler::calculateLValueOffset(LSLLValueExpression *node) {
  char accessor;
  auto *sym = node->getSymbol();
  auto &sym_data = _mSymData[sym];
  auto offset = (int32_t)sym_data.offset;

  auto *accessor_id = (LSLIdentifier *) node->getChild(1);
  if (accessor_id)
    accessor = accessor_id->getName()[0];
  else
    return offset;

  int32_t accessor_offset;
  // offsets for locals are relative to the top of the stack, which means that
  // x comes is at 0 even though it's "last" if reading the binary data from left
  // to right.
  switch(accessor) {
    case 'x': accessor_offset = 0; break;
    case 'y': accessor_offset = 4; break;
    case 'z': accessor_offset = 8; break;
    case 's': accessor_offset = 12; break;
    default:
      return offset;
  }
  // Conversely, offsets for globals are relative to the start of the global.
  if (sym->getSubType() == SYM_GLOBAL) {
    // only coordinates with float members can be accessed via accessors.
    accessor_offset = ((int32_t)sym_data.size - (int32_t)sizeof(float) - accessor_offset);
  }
  return offset + accessor_offset;
}

void LSOBytecodeCompiler::storeStackToLValue(LSLLValueExpression *lvalue) {
  if (lvalue->getSymbol()->getSubType() == SYM_GLOBAL) {
    switch(lvalue->getIType()) {
      case LST_INTEGER:
      case LST_FLOATINGPOINT:
        mCodeBS << LOPC_STOREG;
        break;
      case LST_STRING:
      case LST_KEY:
        mCodeBS << LOPC_STOREGS;
        break;
      case LST_VECTOR:
        mCodeBS << LOPC_STOREGV;
        break;
      case LST_QUATERNION:
        mCodeBS << LOPC_STOREGQ;
        break;
      case LST_LIST:
        mCodeBS << LOPC_STOREGL;
        break;
      case LST_ERROR:
      case LST_MAX:
      case LST_NULL:
        return;
    }
  } else {
    switch(lvalue->getIType()) {
      case LST_INTEGER:
      case LST_FLOATINGPOINT:
        mCodeBS << LOPC_STORE;
        break;
      case LST_STRING:
      case LST_KEY:
        mCodeBS << LOPC_STORES;
        break;
      case LST_VECTOR:
        mCodeBS << LOPC_STOREV;
        break;
      case LST_QUATERNION:
        mCodeBS << LOPC_STOREQ;
        break;
      case LST_LIST:
        mCodeBS << LOPC_STOREL;
        break;
      case LST_ERROR:
      case LST_MAX:
      case LST_NULL:
        return;
    }
  }
  mCodeBS << calculateLValueOffset(lvalue);
}

}
