#include "script_compiler.hh"
#include "../desugaring.hh"
#include <iomanip>

namespace Tailslide {

static const char *CIL_USERSCRIPT_CLASS = "class [LslUserScript]LindenLab.SecondLife.LslUserScript";
static const char *CIL_LSL_RUNTIME_CLASS = "class [LslLibrary]LindenLab.SecondLife.LslRunTime";
static const char *CIL_LSL_LIBRARY_CLASS = "class [LslLibrary]LindenLab.SecondLife.Library";
static const std::string CIL_LIST_INITIALIZER = std::string("call ") + CIL_TYPE_NAMES[LST_LIST] + " " + CIL_USERSCRIPT_CLASS + "::CreateList()";


/// compiles to the text representation of CIL assembly, based on analysis
/// of the output of LL's lscript LSL->CIL compiler circa 2009 (when public
/// lscript source had its last functional update.)
bool MonoScriptCompiler::visit(LSLScript *node) {
  LLConformantDeSugaringVisitor de_sugaring_visitor(_mAllocator, true);
  node->visit(&de_sugaring_visitor);

  MonoResourceVisitor resource_visitor(&_mSymData);
  node->visit(&resource_visitor);
  _mScriptClassName = "LSL_00000000_0000_0000_0000_000000000000";

  mCIL << ".assembly extern mscorlib {.ver 1:0:5000:0}\n"
          ".assembly extern LslLibrary {.ver 0:1:0:0}\n"
          ".assembly extern LslUserScript {.ver 0:1:0:0}\n"
          ".assembly extern ScriptTypes {.ver 0:1:0:0}\n"
          ".assembly '" << _mScriptClassName << "' {.ver 0:0:0:0}\n"
          ".class public auto ansi serializable beforefieldinit " << _mScriptClassName << " extends " << CIL_USERSCRIPT_CLASS << "\n"
          "{\n";

  // declare the global variables first
  auto *globals = node->getChild(0)->getChildren();
  for (auto *global = globals; global; global = global->getNext()) {
    if (global->getNodeType() != NODE_GLOBAL_VARIABLE)
      continue;
    auto *id = (LSLIdentifier *) global->getChild(0);
    mCIL << ".field public " << CIL_TYPE_NAMES[id->getIType()] << " '" << id->getName() << "'\n";
  }

  // now define the globals' values in the ctor for the script
  mCIL << ".method public hidebysig specialname rtspecialname instance default void .ctor () cil managed\n"
          "{\n"
          ".maxstack 500\n";

  _mInGlobalExpr = true;
  for (auto *global = globals; global; global = global->getNext()) {
    if (global->getNodeType() != NODE_GLOBAL_VARIABLE)
      continue;
    global->visit(this);
  }
  _mInGlobalExpr = false;

  // call the base constructor for the script class and return
  mCIL << "ldarg.0\n"
          "call instance void " << CIL_USERSCRIPT_CLASS << "::.ctor()\n"
          "ret\n"
          "}\n";

  // now go over the globals _again_ to pick up all the functions
  for (auto *global = globals; global; global = global->getNext()) {
    if (global->getNodeType() != NODE_GLOBAL_FUNCTION)
      continue;
    global->visit(this);
  }

  // now look at the event handlers
  node->getChild(1)->visit(this);

  mCIL << "}\n";

  return false;
}

bool MonoScriptCompiler::visit(LSLGlobalVariable *node) {
  // push a reference to `this` for the later stfld
  mCIL << "ldarg.0\n";
  auto *sym = node->getSymbol();
  auto *rhs = node->getChild(1);
  if (rhs->getNodeType() != NODE_NULL) {
    node->getChild(1)->visit(this);
  } else {
    pushConstant(sym->getType()->getDefaultValue());
  }
  mCIL << "stfld " << getGlobalVarSpecifier(sym) << "\n";
  return false;
}

/// Push things that need to be on the stack before we can read or write the ultimate
/// target of the lvalue (reference to `this`, the containing object for accessors, etc.)
void MonoScriptCompiler::pushLValueContainer(LSLLValueExpression *lvalue) {
  auto *sym = lvalue->getSymbol();
  if (sym->getSubType() == SYM_GLOBAL) {
    // push a reference to `this` since this is an attribute of the class
    // and we'll need to `ldfld`
    mCIL << "ldarg.0\n";
  }
  // have an accessor, we need to push the containing object's address!
  if (lvalue->getChild(1)) {
    pushSymbolAddress(sym);
  }
}

/// push the actual value of an lvalue onto the stack
void MonoScriptCompiler::pushLValue(LSLLValueExpression *lvalue) {
  pushLValueContainer(lvalue);
  auto *sym = lvalue->getSymbol();
  if (lvalue->getChild(1)) {
    // accessor case, containing object is already on the stack and
    // we just have to load the field.
    mCIL << "ldfld " << getLValueAccessorSpecifier(lvalue) << "\n";
  } else {
    pushSymbolValue(sym);
  }
}

void MonoScriptCompiler::pushSymbolValue(LSLSymbol *sym) {
  if (sym->getSubType() == SYM_GLOBAL) {
    // LslUserScript `this` should already on the stack, load the given field from `this`.
    mCIL << "ldfld " << getGlobalVarSpecifier(sym) << "\n";
  } else if (sym->getSubType() == SYM_LOCAL) {
    // must be a local, reference by index
    // Seems that UThreadInjector may rewrite these to ldloc.0, ldloc.1, ...
    // but we aren't aiming for conformance with its output, only lscript's.
    mCIL << "ldloc.s " << _mSymData[sym].index << "\n";
  } else {
    // event or function param
    mCIL << "ldarg.s '" << sym->getName() << "'\n";
  }
}

/// same as pushSymbolValue(), but pushes the address instead of the value
void MonoScriptCompiler::pushSymbolAddress(LSLSymbol *sym) {
  if (sym->getSubType() == SYM_GLOBAL) {
    mCIL << "ldflda " << getGlobalVarSpecifier(sym) << "\n";
  } else if (sym->getSubType() == SYM_LOCAL) {
    mCIL << "ldloca.s " << _mSymData[sym].index << "\n";
  } else {
    // event or function param
    mCIL << "ldarga.s '" << sym->getName() << "'\n";
  }
}

void MonoScriptCompiler::pushConstant(LSLConstant* cv) {
  if (!cv)
    return;
  switch(cv->getIType()) {
    case LST_INTEGER: {
      auto *int_cv = (LSLIntegerConstant *) cv;
      // pushing 0 or 1 has a single-byte form.
      if (!int_cv->getValue())
        mCIL << "ldc.i4.0\n";
      else if (int_cv->getValue() == 1)
        mCIL << "ldc.i4.1\n";
      else
        mCIL << "ldc.i4 " << int_cv->getValue() << "\n";
      return;
    }
    case LST_FLOATINGPOINT:
      pushFloatLiteral(((LSLFloatConstant *) cv)->getValue());
      return;
    case LST_STRING:
      mCIL << "ldstr \"" << escape_string(((LSLStringConstant *) cv)->getValue()) << "\"\n";
      return;
    case LST_KEY:
      mCIL << "ldstr \"" << escape_string(((LSLKeyConstant *) cv)->getValue()) << "\"\n";
      mCIL << "call " << CIL_TYPE_NAMES[LST_KEY] << " " << CIL_USERSCRIPT_CLASS << "::'CreateKey'(string)\n";
      return;
    case LST_VECTOR: {
      auto *vec_val = ((LSLVectorConstant *) cv)->getValue();
      pushFloatLiteral(vec_val->x);
      pushFloatLiteral(vec_val->y);
      pushFloatLiteral(vec_val->z);
      mCIL << "call " << CIL_TYPE_NAMES[LST_VECTOR] << " " << CIL_USERSCRIPT_CLASS << "::'CreateVector'(float32, float32, float32)\n";
      return;
    }
    case LST_QUATERNION: {
      auto *vec_val = ((LSLQuaternionConstant *) cv)->getValue();
      pushFloatLiteral(vec_val->x);
      pushFloatLiteral(vec_val->y);
      pushFloatLiteral(vec_val->z);
      pushFloatLiteral(vec_val->s);
      mCIL << "call " << CIL_TYPE_NAMES[LST_QUATERNION] << " " << CIL_USERSCRIPT_CLASS << "::'CreateQuaternion'(float32, float32, float32, float32)\n";
      return;
    }
    case LST_LIST: {
      // only know how to write the default empty list as a constant
      auto *list_val = (LSLListConstant *) cv;
      assert(!list_val->getLength());
      mCIL << "call " << CIL_TYPE_NAMES[LST_LIST] << " " << CIL_USERSCRIPT_CLASS << "::CreateList()\n";
      return;
    }
    default:
      assert(0);
  }
}

/// used for a number of cases, including pushing float constants and vec/quat components
void MonoScriptCompiler::pushFloatLiteral(float value) {
  // pushed as a double for some reason
  // use the binary hex form specified in the ECMA standard to preserve precision
  auto d_val = (double)value;
  auto *b_val = reinterpret_cast<uint8_t *>(&d_val);
  // enough room for all of the octets plus null. much less annoying than
  // the stringstream form and no C++20 so we don't have std::format
  const size_t s_val_len = 3 * 8;
  char s_val[s_val_len] = {0};
  snprintf(
      (char *)&s_val,
      s_val_len,
      "%02x %02x %02x %02x %02x %02x %02x %02x",
      b_val[0], b_val[1], b_val[2], b_val[3], b_val[4], b_val[5], b_val[6], b_val[7]
  );
  mCIL << "ldc.r8 (" << (const char*)&s_val << ")\n";
}

void MonoScriptCompiler::storeToLValue(LSLLValueExpression *lvalue, bool push_result) {
  auto *sym = lvalue->getSymbol();
  // coordinate accessor case
  if (lvalue->getChild(1)) {
    mCIL << "stfld " << getLValueAccessorSpecifier(lvalue) << "\n";
    // Expression assignments need to return their result, load what we just stored onto the stack
    // TODO: This seems really wasteful in many cases, but this is how LL's compiler does it.
    //  I guess `dup` isn't an option because of the `this` reference, but wouldn't creating a
    //  scratch local be better? Most assignments' retvals aren't used at all either,
    //  so some heuristic to elide this entirely would be nice. Common case of assignment
    //  expression directly parented to an ExpressionStatement?
    if (push_result)
      pushLValue(lvalue);
  } else if (sym->getSubType() == SYM_GLOBAL) {
    mCIL << "stfld " << getGlobalVarSpecifier(lvalue->getSymbol()) << "\n";
    // same caveat as above
    if (push_result)
      pushLValue(lvalue);
  } else {
    // We can avoid reloading the lvalue from its storage container in these cases by just duplicating
    // the result of the expression on the stack. All we need on the stack for these stores is the value.
    if (push_result)
      mCIL << "dup\n";

    if (sym->getSubType() == SYM_LOCAL) {
      mCIL << "stloc.s " << _mSymData[sym].index << "\n";
    } else {
      // event or function param
      mCIL << "starg.s '" << sym->getName() << "'\n";
    }
  }
}

void MonoScriptCompiler::castTopOfStack(LSLIType from_type, LSLIType to_type) {
  if (from_type == to_type)
    return;

  switch(to_type) {
    case LST_INTEGER:
      switch(from_type) {
        case LST_FLOATINGPOINT:
          mCIL << "call int32 " << CIL_LSL_RUNTIME_CLASS << "::ToInteger(float32)\n";
          return;
        case LST_STRING:
          mCIL << "call int32 " << CIL_LSL_RUNTIME_CLASS << "::StringToInt(string)\n";
          return;
        default:
          assert(0);
      }
    case LST_FLOATINGPOINT:
      switch(from_type) {
        case LST_INTEGER:
          mCIL << "conv.r8\n";
          return;
        case LST_STRING:
          mCIL << "call float32 " << CIL_LSL_RUNTIME_CLASS << "::StringToFloat(string)\n";
          return;
        default:
          assert(0);
      }
    case LST_STRING:
      switch (from_type) {
        case LST_LIST:
          mCIL << "call string " << CIL_LSL_RUNTIME_CLASS << "::ListToString(" << CIL_TYPE_NAMES[LST_LIST] << ")\n";
          return;
        case LST_INTEGER:
          mCIL << "call string class [mscorlib]System.Convert::ToString(" << CIL_TYPE_NAMES[LST_INTEGER] << ")\n";
          return;
        case LST_FLOATINGPOINT:
          mCIL << "call string " << CIL_LSL_RUNTIME_CLASS << "::'ToString'(" << CIL_TYPE_NAMES[from_type] << ")\n";
          return;
        default:
          mCIL << "call string " << CIL_USERSCRIPT_CLASS << "::'ToString'(" << CIL_VALUE_TYPE_NAMES[from_type] << ")\n";
          return;
      }
    case LST_KEY:
      if (from_type == LST_STRING)
        mCIL << "call " << CIL_TYPE_NAMES[LST_KEY] << " " << CIL_USERSCRIPT_CLASS << "::'CreateKey'(string)\n";
      return;
    case LST_VECTOR:
      if (from_type == LST_STRING)
        mCIL << "call " << CIL_TYPE_NAMES[LST_VECTOR] << " " << CIL_USERSCRIPT_CLASS << "::'ParseVector'(string)\n";
      return;
    case LST_QUATERNION:
      if (from_type == LST_STRING)
        mCIL << "call " << CIL_TYPE_NAMES[LST_QUATERNION] << " " << CIL_USERSCRIPT_CLASS << "::'ParseQuaternion'(string)\n";
      return;
    case LST_LIST:
      // All casts to list are the same, box if necessary and then `CreateList(object)`.
      mCIL << CIL_BOXING_INSTRUCTIONS[from_type];
      mCIL << "call " << CIL_TYPE_NAMES[LST_LIST] << " " << CIL_USERSCRIPT_CLASS << "::CreateList(object)\n";
      return;
    default:
      assert(0);
  }
}

void MonoScriptCompiler::truthCheckTopOfStack(LSLIType type) {
  switch(type) {
    case LST_INTEGER:
      return;
    case LST_FLOATINGPOINT:
      pushConstant(TYPE(LST_FLOATINGPOINT)->getDefaultValue());
      mCIL << "ceq\n"
           // TODO: LL's compiler does this, is it necessary?
           << "ldc.i4.0\n"
           << "ceq\n";
      return;
    case LST_STRING:
      pushConstant(TYPE(LST_STRING)->getDefaultValue());
      mCIL << "call bool string::op_Equality(string, string)\n"
           // TODO: LL's compiler does this, is it necessary?
           << "ldc.i4.0\n"
           << "ceq\n";
      return;
    case LST_VECTOR:
    case LST_QUATERNION:
      pushConstant(TYPE(type)->getDefaultValue());
      mCIL << "call bool " << CIL_USERSCRIPT_CLASS << "::'Equals'(" << CIL_TYPE_NAMES[type] << ", " << CIL_TYPE_NAMES[type] << ")\n"
           // TODO: LL's compiler does this, is it necessary?
           << "ldc.i4.0\n"
           << "ceq\n";
      return;
    case LST_KEY:
      mCIL << "call bool " << CIL_USERSCRIPT_CLASS <<"::'IsNonNullUuid'(" << CIL_TYPE_NAMES[LST_KEY] << ")\n";
      return;
    case LST_LIST:
      pushConstant(TYPE(LST_LIST)->getDefaultValue());
      mCIL << "call bool " << CIL_USERSCRIPT_CLASS << "::'Equals'(" << CIL_TYPE_NAMES[LST_LIST] << ", " << CIL_TYPE_NAMES[LST_LIST] << ")\n"
           << "ldc.i4.0\n"
           << "ceq\n";
      return;
    default:
      assert(0);
  }
}

std::string MonoScriptCompiler::getGlobalVarSpecifier(LSLSymbol *sym) {
  std::stringstream ss;
  ss << CIL_TYPE_NAMES[sym->getIType()] << " " << _mScriptClassName << "::'" << sym->getName() << "'";
  return ss.str();
}

/// return a specifier that allows referencing the field for the accessor with ldfld or stfld
std::string MonoScriptCompiler::getLValueAccessorSpecifier(LSLLValueExpression *lvalue) {
  auto *accessor = lvalue->getChild(1);
  auto *accessor_type_name = CIL_TYPE_NAMES[lvalue->getIType()];
  auto *obj_type_name = CIL_TYPE_NAMES[lvalue->getSymbol()->getIType()];
  auto *accessor_name = ((LSLIdentifier *) accessor)->getName();
  return std::string(accessor_type_name) + " " + obj_type_name + "::" + accessor_name;
}



bool MonoScriptCompiler::visit(LSLEventHandler *node) {
  auto *state_sym = node->getParent()->getSymbol();
  mCIL << ".method public hidebysig instance default void e" << state_sym->getName() << node->getSymbol()->getName();
  // parameter list will be handled by `buildFunction()`
  buildFunction(node);
  return false;
}

bool MonoScriptCompiler::visit(LSLGlobalFunction *node) {
  auto *node_sym = node->getSymbol();
  mCIL << ".method public hidebysig instance default " << CIL_TYPE_NAMES[node->getChild(0)->getIType()]
       << " 'g" << node_sym->getName() << "'";
  buildFunction(node);
  return false;
}

void MonoScriptCompiler::buildFunction(LSLASTNode *node) {
  // write out the parameter list
  auto *func_sym = node->getSymbol();
  auto *func_decl = func_sym->getFunctionDecl();
  mCIL << "(";
  for (auto *func_param=func_decl->getChildren(); func_param; func_param = func_param->getNext()) {
    auto *param_sym = func_param->getSymbol();
    mCIL << CIL_TYPE_NAMES[param_sym->getIType()] << " '" << param_sym->getName() << "'";
    if (func_param->getNext())
      mCIL << ", ";
  }
  mCIL << ") cil managed\n";
  mCIL << "{\n";
  mCIL << ".maxstack 500\n";
  std::string locals_string;
  for (auto param_type : _mSymData[func_sym].locals) {
    if (!locals_string.empty()) {
      locals_string += ", ";
    }
    locals_string += CIL_TYPE_NAMES[param_type];
  }
  if (!locals_string.empty())
    mCIL << ".locals init (" << locals_string << ")\n";
  _mCurrentFuncSym = node->getSymbol();
  visitChildren(node);
  _mCurrentFuncSym = nullptr;
  if (!func_sym->getAllPathsReturn())
    mCIL << "ret\n";
  mCIL << "}\n";
}



bool MonoScriptCompiler::visit(LSLDeclaration *node) {
  auto *sym = node->getSymbol();
  auto *rhs = node->getChild(1);
  if (rhs->getNodeType() != NODE_NULL) {
    node->getChild(1)->visit(this);
  } else {
    pushConstant(sym->getType()->getDefaultValue());
  }
  mCIL << "stloc.s " << _mSymData[sym].index << "\n";
  return false;
}

bool MonoScriptCompiler::visit(LSLExpressionStatement* node) {
  auto *expr = node->getChild(0);
  expr->visit(this);
  if (expr->getIType())
    mCIL << "pop\n";
  return false;
}

bool MonoScriptCompiler::visit(LSLReturnStatement* node) {
  node->getChild(0)->visit(this);
  mCIL << "ret\n";
  return false;
}

bool MonoScriptCompiler::visit(LSLLabel* node) {
  // TODO: right now this roughly matches LL's behavior, but label names
  //  should be mangled to prevent collisions.
  mCIL << "'ul" << node->getSymbol()->getName() << "':\n";
  return false;
}

bool MonoScriptCompiler::visit(LSLJumpStatement* node) {
  // TODO: right now this roughly matches LL's behavior, but label names
  //  should be mangled to prevent collisions.
  mCIL << "br 'ul" << node->getSymbol()->getName() << "'\n";
  return false;
}

bool MonoScriptCompiler::visit(LSLIfStatement* node) {
  auto jump_past_true_num = _mJumpNum++;
  uint32_t jump_past_false_num = 0;
  auto *false_node = node->getChild(2);
  if (false_node->getNodeType() != NODE_NULL)
    jump_past_false_num = _mJumpNum++;

  auto *check_expr = node->getChild(0);
  check_expr->visit(this);
  truthCheckTopOfStack(check_expr->getIType());
  mCIL << "brfalse LabelTempJump" << jump_past_true_num << "\n";
  node->getChild(1)->visit(this);
  if (false_node->getNodeType() != NODE_NULL) {
    mCIL << "br LabelTempJump" << jump_past_false_num << "\n";
    mCIL << "LabelTempJump" << jump_past_true_num << ":\n";
    false_node->visit(this);
    mCIL << "LabelTempJump" << jump_past_false_num << ":\n";
  } else {
    mCIL << "LabelTempJump" << jump_past_true_num << ":\n";
  }
  return false;
}

bool MonoScriptCompiler::visit(LSLForStatement* node) {
  for(auto *init_expr=node->getChild(0)->getChildren(); init_expr; init_expr=init_expr->getNext()) {
    init_expr->visit(this);
    if (init_expr->getIType() != LST_NULL)
      mCIL << "pop\n";
  }
  auto jump_to_start_num = _mJumpNum++;
  auto jump_to_end_num = _mJumpNum++;
  mCIL << "LabelTempJump" << jump_to_start_num << ":\n";
  // run the check expression, exiting the loop if it fails
  auto *check_expr = node->getChild(1);
  check_expr->visit(this);
  truthCheckTopOfStack(check_expr->getIType());
  mCIL << "brfalse LabelTempJump" << jump_to_end_num << "\n";
  // run the body of the loop
  node->getChild(3)->visit(this);
  // run the increment expressions
  for(auto *incr_expr=node->getChild(2)->getChildren(); incr_expr; incr_expr=incr_expr->getNext()) {
    incr_expr->visit(this);
    if (incr_expr->getIType() != LST_NULL)
      mCIL << "pop\n";
  }
  // jump back up to the check expression at the top
  mCIL << "br LabelTempJump" << jump_to_start_num << "\n";
  mCIL << "LabelTempJump" << jump_to_end_num << ":\n";
  return false;
}

bool MonoScriptCompiler::visit(LSLWhileStatement* node) {
  auto jump_to_start_num = _mJumpNum++;
  auto jump_to_end_num = _mJumpNum++;
  mCIL << "LabelTempJump" << jump_to_start_num << ":\n";
  // run the check expression, exiting the loop if it fails
  auto *check_expr = node->getChild(0);
  check_expr->visit(this);
  truthCheckTopOfStack(check_expr->getIType());
  mCIL << "brfalse LabelTempJump" << jump_to_end_num << "\n";
  // run the body of the loop
  node->getChild(1)->visit(this);
  // jump back up to the check expression at the top
  mCIL << "br LabelTempJump" << jump_to_start_num << "\n";
  mCIL << "LabelTempJump" << jump_to_end_num << ":\n";
  return false;
}

bool MonoScriptCompiler::visit(LSLDoStatement* node) {
  auto jump_to_start_num = _mJumpNum++;
  mCIL << "LabelTempJump" << jump_to_start_num << ":\n";
  // run the body of the loop
  node->getChild(0)->visit(this);
  // run the check expression, jumping back up if it succeeds
  auto *check_expr = node->getChild(1);
  check_expr->visit(this);
  truthCheckTopOfStack(check_expr->getIType());
  mCIL << "brtrue LabelTempJump" << jump_to_start_num << "\n";
  return false;
}

bool MonoScriptCompiler::visit(LSLStateStatement *node) {
  mCIL << "ldarg.0\n"
       << "ldstr \"" << escape_string(node->getSymbol()->getName()) << "\"\n"
       << "call instance void " << CIL_USERSCRIPT_CLASS << "::ChangeState(string)\n";
  pushConstant(_mCurrentFuncSym->getType()->getDefaultValue());
  mCIL << "ret\n";
  return false;
}



bool MonoScriptCompiler::visit(LSLConstantExpression *node) {
  pushConstant(node->getConstantValue());
  return false;
}

bool MonoScriptCompiler::visit(LSLTypecastExpression *node) {
  auto *child_expr = node->getChild(0);
  assert(child_expr);
  child_expr->visit(this);
  castTopOfStack(child_expr->getIType(), node->getIType());
  return false;
}

bool MonoScriptCompiler::visit(LSLVectorExpression *node) {
  visitChildren(node);
  mCIL << "call " << CIL_TYPE_NAMES[LST_VECTOR] << " " << CIL_USERSCRIPT_CLASS << "::'CreateVector'(float32, float32, float32)\n";
  return false;
}

bool MonoScriptCompiler::visit(LSLQuaternionExpression *node) {
  visitChildren(node);
  mCIL << "call " << CIL_TYPE_NAMES[LST_QUATERNION] << " " << CIL_USERSCRIPT_CLASS << "::'CreateQuaternion'(float32, float32, float32, float32)\n";
  return false;
}

bool MonoScriptCompiler::visit(LSLLValueExpression *node) {
  pushLValue(node);
  return false;
}

bool MonoScriptCompiler::visit(LSLListExpression *node) {
  // LL's compiler pushes lists in a different order in globexprs for some reason,
  // maybe something about order of evaluation being important there.
  // match their behavior so it's less annoying to compare output.
  if (_mInGlobalExpr) {
    mCIL << CIL_LIST_INITIALIZER << "\n";
    for (auto *child = node->getChildren(); child; child = child->getNext()) {
      child->visit(this);
      mCIL << CIL_BOXING_INSTRUCTIONS[child->getIType()]
           << "call " << CIL_TYPE_NAMES[LST_LIST] << " " << CIL_USERSCRIPT_CLASS << "::Append(" << CIL_TYPE_NAMES[LST_LIST] << ", object)\n";
    }
  } else {
    // list elements get evaluated and pushed FIRST
    for (auto *child = node->getChildren(); child; child = child->getNext()) {
      child->visit(this);
      mCIL << CIL_BOXING_INSTRUCTIONS[child->getIType()];
    }
    // then they get added to the list
    mCIL << CIL_LIST_INITIALIZER << "\n";
    for (auto *child = node->getChildren(); child; child = child->getNext()) {
      mCIL << "call " << CIL_TYPE_NAMES[LST_LIST] << " " << CIL_USERSCRIPT_CLASS << "::Prepend(object, " << CIL_TYPE_NAMES[LST_LIST] << ")\n";
    }
  }
  return false;
}

bool MonoScriptCompiler::visit(LSLFunctionExpression *node) {
  auto *func_sym = node->getSymbol();
  // this will be a method on the script instance, push `this` onto the stack
  if (func_sym->getSubType() != SYM_BUILTIN)
    mCIL << "ldarg.0\n";

  // push the argument expressions
  visitChildren(node);

  if (func_sym->getSubType() == SYM_BUILTIN) {
    mCIL << "call " << CIL_TYPE_NAMES[node->getIType()] << " "
         << CIL_LSL_LIBRARY_CLASS << "::'" << func_sym->getName() << "'";
  } else {
    mCIL << "call instance " << CIL_TYPE_NAMES[node->getIType()] << " class "
         << _mScriptClassName + "::'g" + func_sym->getName() + "'";
  }

  // write in the functions' expected parameter types
  auto *func_decl = func_sym->getFunctionDecl();
  mCIL << "(";
  for (auto *func_param=func_decl->getChildren(); func_param; func_param = func_param->getNext()) {
    mCIL << CIL_TYPE_NAMES[func_param->getIType()];
    if (func_param->getNext())
      mCIL << ", ";
  }
  mCIL << ")\n";
  return false;
}


// An operation that is performed via a method call on the UserScript class
typedef struct {
  LSLIType left;
  LSLIType right;
} SimpleBinaryOperationInfo;

const std::map<int, std::pair<const char *, std::vector<SimpleBinaryOperationInfo> > > SIMPLE_BINARY_OPS = {
    {'+', {"Add", {
        {LST_STRING, LST_STRING},
        {LST_VECTOR, LST_VECTOR},
        {LST_QUATERNION, LST_QUATERNION},
    }}},
    {'-', {"Subtract", {
        {LST_INTEGER, LST_INTEGER},
        // not simple, float64 args and return!
        // {LST_FLOATINGPOINT, LST_FLOATINGPOINT},
        {LST_VECTOR, LST_VECTOR},
        {LST_QUATERNION, LST_QUATERNION},
    }}},
    {'*', {"Multiply", {
        {LST_VECTOR, LST_FLOATINGPOINT},
        {LST_FLOATINGPOINT, LST_VECTOR},
        {LST_VECTOR, LST_VECTOR},
        {LST_VECTOR, LST_QUATERNION},
        {LST_QUATERNION, LST_QUATERNION},
    }}},
    {'/', {"Divide", {
        {LST_INTEGER, LST_INTEGER},
        // not simple, float64 args and return!
        // {LST_FLOATINGPOINT, LST_FLOATINGPOINT},
        {LST_VECTOR, LST_FLOATINGPOINT},
        {LST_VECTOR, LST_QUATERNION},
        {LST_QUATERNION, LST_QUATERNION},
    }}},
    {'%', {"Modulo", {
        {LST_INTEGER, LST_INTEGER},
        {LST_VECTOR, LST_VECTOR},
    }}},
    {EQ, {"Equals", {
        {LST_VECTOR, LST_VECTOR},
        {LST_QUATERNION, LST_QUATERNION},
        {LST_LIST, LST_LIST},
    }}},
    {NEQ, {"NotEquals", {
        // TODO: desugar `a != b` to `!(a == b)` where possible?
        {LST_LIST, LST_LIST},
    }}},
    {SHIFT_LEFT, {"ShiftLeft", {
        {LST_INTEGER, LST_INTEGER},
    }}},
    {SHIFT_RIGHT, {"ShiftRight", {
        {LST_INTEGER, LST_INTEGER},
    }}},
};

bool MonoScriptCompiler::visit(LSLBinaryExpression *node) {
  int op = node->getOperation();
  auto *left = (LSLExpression *)node->getChild(0);
  auto *right = (LSLExpression *)node->getChild(1);

  if (op == '=') {
    // we're going to store, so we may need a reference to `this` on top of the stack
    auto *lvalue = (LSLLValueExpression *) left;
    pushLValueContainer(lvalue);
    right->visit(this);
    // store to the lvalue and push the lvalue back onto the stack
    storeToLValue(lvalue, true);
    return false;
  } else if (op == MUL_ASSIGN) {
    // The only expression that gets left like this is the busted `int *= float` case.
    // That expression is busted and not the same as `int = int * float`, obviously,
    // but we need to support it.
    auto *lvalue = (LSLLValueExpression *) left;
    pushLValueContainer(lvalue);
    right->visit(this);
    lvalue->visit(this);
    // cast the integer lvalue to a float first
    castTopOfStack(left->getIType(), right->getIType());
    mCIL << "mul\n";
    // cast it back to an integer after the operation
    if (lvalue->getIType() == LST_INTEGER && right->getIType() == LST_FLOATINGPOINT)
      castTopOfStack(LST_FLOATINGPOINT, LST_INTEGER);
    // This will return the wrong type because things expect a float. Use of the retval
    // will probably cause a crash.
    storeToLValue(lvalue, true);
    return false;
  }

  compileBinaryExpression(op, left, right, node->getIType());
  return false;
}

bool MonoScriptCompiler::compileBinaryExpression(int op, LSLExpression *left, LSLExpression *right, LSLIType ret_type) {
  auto left_type = left->getIType();
  auto right_type = right->getIType();

  auto simple_op = SIMPLE_BINARY_OPS.find(op);
  // this is an operation that uses the simplified method call form
  if (simple_op != SIMPLE_BINARY_OPS.end()) {
    // walk through the type pairs this method has a method call form for
    for (auto simple_op_combo : ((*simple_op).second.second)) {
      if (left_type != simple_op_combo.left || right_type != simple_op_combo.right)
        continue;
      right->visit(this);
      left->visit(this);
      // right is first argument due to reversed order of evaluation in LSL
      mCIL << "call " << CIL_TYPE_NAMES[ret_type] << " " << CIL_USERSCRIPT_CLASS << "::'"
           << simple_op->second.first << "'(" << CIL_TYPE_NAMES[right_type] << ", " << CIL_TYPE_NAMES[left_type] << ")\n";
      return false;
    }
  }

  // these are the only ones that needs to do codegen interleaved with visitation
  if (op != BOOLEAN_AND && op != NEQ) {
    right->visit(this);
    left->visit(this);
  }

  // handle all the operations that couldn't be handled via templating together a method call
  switch(op) {
    case '+': {
      if (right_type == LST_LIST && left_type != LST_LIST) {
        // prepend whatever this is to the right list
        mCIL << CIL_BOXING_INSTRUCTIONS[left_type];
        mCIL << "call " << CIL_TYPE_NAMES[LST_LIST] << " " << CIL_USERSCRIPT_CLASS << "::Prepend(" << CIL_TYPE_NAMES[LST_LIST] << ", object)\n";
        return false;
      } else if (left_type == LST_LIST) {
        // append to the left list (will also join lists)
        mCIL << "call " << CIL_TYPE_NAMES[LST_LIST] << " " << CIL_USERSCRIPT_CLASS << "::Append("
             << CIL_VALUE_TYPE_NAMES[right_type] << ", " << CIL_VALUE_TYPE_NAMES[LST_LIST] << ")\n";
        return false;
      }

      switch (left_type) {
        case LST_INTEGER:
        case LST_FLOATINGPOINT:
          mCIL << "add\n"; return false;
        default:
          assert(0);
      }
    }
    case '-': {
      switch (left_type) {
        case LST_FLOATINGPOINT:
          mCIL << "call float64 " << CIL_USERSCRIPT_CLASS << "::'Subtract'(float64, float64)\n"; return false;
        default:
          assert(0);
      }
    }
    case '*': {
      switch (left_type) {
        case LST_INTEGER:
        case LST_FLOATINGPOINT:
          mCIL << "mul\n"; return false;
        default:
          assert(0);
      }
    }
    case '/': {
      switch (left_type) {
        case LST_FLOATINGPOINT:
          mCIL << "call float64 " << CIL_USERSCRIPT_CLASS << "::'Divide'(float64, float64)\n"; return false;
        default:
          assert(0);
      }
    }
    case EQ: {
      switch (right_type) {
        case LST_INTEGER:
        case LST_FLOATINGPOINT:
          mCIL << "ceq\n"; return false;
        case LST_STRING:
          // note the key == string and string == key asymmetry here...
          // left is top of stack, so convert left to a string if it isn't one already
          castTopOfStack(left_type, right_type);
          mCIL << "call bool valuetype [mscorlib]System.String::op_Equality(string, string)\n";
          return false;
        case LST_KEY:
          // these really should have been pre-casted if necessary, but this is what LL's compiler does
          castTopOfStack(left_type, right_type);
          mCIL << "call int32 " << CIL_USERSCRIPT_CLASS << "::'Equals'(" << CIL_TYPE_NAMES[LST_KEY] << ", " << CIL_TYPE_NAMES[LST_KEY] << ")\n";
          return false;
        default:
          assert(0);
      }
    }
    case NEQ:
      compileBinaryExpression(EQ, left, right, ret_type);
      // check if result == 0
      mCIL << "ldc.i4.0\n"
           << "ceq\n";
      return false;
    // not very nice, but operands are swapped from how CIL would like them
    case GEQ:
      mCIL << "cgt\n"
           << "ldc.i4.0\n"
           << "ceq\n";
      return false;
    case LEQ:
      mCIL << "clt\n"
           << "ldc.i4.0\n"
           << "ceq\n";
      return false;
    case '>':
      mCIL << "clt\n";
      return false;
    case '<':
      mCIL << "cgt\n";
      return false;
    case BOOLEAN_AND:
      // we have to manually visit in this case because we need to interleave
      // our codegen with the code of the expressions
      right->visit(this);
      // push whether this returned false
      // necessary because everything EXCEPT 0 is truthy!
      mCIL << "ldc.i4.0\n"
           << "ceq\n";

      left->visit(this);
      mCIL << "ldc.i4.0\n"
           << "ceq\n";

      // binary OR the results together and compare against zero
      // will push whether neither had the false bit set
      mCIL << "or\n"
           << "ldc.i4.0\n"
           << "ceq\n";
      return false;
    case BOOLEAN_OR:
      // binary OR the sides together and compare against zero
      mCIL << "or\n"
           << "ldc.i4.0\n"
           << "ceq\n"
           // TODO: LL's codegen compares against zero again. Copy & paste error in their code?
           << "ldc.i4.0\n"
           << "ceq\n";
      return false;
    case '&':
      mCIL << "and\n";
      return false;
    case '|':
      mCIL << "or\n";
      return false;
    case '^':
      mCIL << "xor\n";
      return false;
    default:
      assert(0);
  }
  assert(0);
  return false;
}

bool MonoScriptCompiler::visit(LSLUnaryExpression *node) {
  auto *child_expr = node->getChild(0);
  int op = node->getOperation();

  if (op == DEC_POST_OP || op == INC_POST_OP) {
    // We need to keep the original value of the expression on the stack.
    auto *lvalue = (LSLLValueExpression *) child_expr;
    pushLValue(lvalue);

    // may need the containing object ref for for the subsequent store
    pushLValueContainer(lvalue);
    // then load another copy of the value to do the operation (wasteful!)
    pushLValue(lvalue);
    // push "one" for the given type
    pushConstant(lvalue->getType()->getOneValue());
    if (op == DEC_POST_OP) {
      mCIL << "sub\n";
    } else {
      mCIL << "add\n";
    }

    // TODO: this store + push, then subsequent pop is totally unnecessary, but matches what LL's
    //  compiler does. investigate using no-push form of storeToLValue().
    storeToLValue(lvalue, true);
    mCIL << "pop\n";
    return false;
  } else if (op == INC_PRE_OP || op == DEC_PRE_OP) {
    auto *lvalue = (LSLLValueExpression *) child_expr;
    pushLValueContainer(lvalue);
    pushLValue(lvalue);
    pushConstant(lvalue->getType()->getOneValue());
    if (op == DEC_PRE_OP) {
      mCIL << "sub\n";
    } else {
      mCIL << "add\n";
    }
    storeToLValue(lvalue, true);
    return false;
  } else {
    auto child_type = child_expr->getIType();
    child_expr->visit(this);
    switch (op) {
      case '-':
        switch(child_type) {
          case LST_INTEGER:
          case LST_FLOATINGPOINT:
            mCIL << "neg\n";
            return false;
          case LST_QUATERNION:
          case LST_VECTOR:
            mCIL << "call " << CIL_TYPE_NAMES[child_type] << " " << CIL_USERSCRIPT_CLASS << "::'Negate'(" << CIL_TYPE_NAMES[child_type] << ")\n";
            return false;
          default:
            assert(0);
        }
      case '!': {
        mCIL << "ldc.i4.0\n"
             << "ceq\n";
        return false;
      }
      case '~': {
        mCIL << "not\n";
        return false;
      }
      default:
        assert(0);
    }
  }
  assert(0);
  return false;
}

bool MonoScriptCompiler::visit(LSLPrintExpression *node) {
  auto *child_expr = (LSLExpression *) node->getChild(0);
  child_expr->visit(this);
  // TOOD: void expressions????
  castTopOfStack(child_expr->getIType(), LST_STRING);
  mCIL << "call void " << CIL_LSL_LIBRARY_CLASS << "::Print(string)\n";
  return false;
}

}
