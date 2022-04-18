#include "bytecode_format.hh"
#include "lso_compiler.hh"

namespace Tailslide {

bool LSOCompilerVisitor::visit(LSLScript *node) {
  // allocate all of the script memory and fill with zeros
  mScriptBS.makeSpace(TOTAL_LSO_MEMORY);
  // write the first few registers whose values are mostly
  // always the same during compilation
  writeRegister(LREG_TM, TOTAL_LSO_MEMORY);
  writeRegister(LREG_IP, 0);
  writeRegister(LREG_VN, LSO_VERSION_NUM);
  writeRegister(LREG_BP, TOTAL_LSO_MEMORY - 1);
  writeRegister(LREG_SP, TOTAL_LSO_MEMORY - 1);
  // Need to queue up a state_entry event for when the script starts
  // TODO: LSO v1 compatibility
  writeEventRegister(LREG_NCE, LSOH_STATE_ENTRY);
  writeEventRegister(LREG_NER, LSOH_STATE_ENTRY);
  // record where the variables start
  // TODO: This will break if we're writing v1 bytecode, the registers end earlier!
  writeRegister(LREG_GVR, LSO_REGISTER_OFFSETS[LREG_MAX]);

  // Collect all the global variables
  auto *globals = node->getChild(0)->getChildren();
  while (globals != nullptr) {
    auto *gs_var_slot = globals->getChild(0);
    // this globalstorage holds a variable, not a function
    if (gs_var_slot->getNodeType() != NODE_NULL) {
      gs_var_slot->visit(this);
    }
    globals = globals->getNext();
  }
  _mHeapManager.writeTerminalBlock();

  // Write in the global variables themselves
  mScriptBS.moveTo(LSO_REGISTER_OFFSETS[LREG_MAX]);
  mScriptBS.writeRawData(_mGlobalVarManager.mGlobalsBS.data(), _mGlobalVarManager.mGlobalsBS.size());

  // mark the end of the globals as the start of the functions
  writeRegister(LREG_GFR, mScriptBS.pos());

  // mark the start of the state entries, just say we have 0 states for now
  writeRegister(LREG_SR, mScriptBS.pos());
  mScriptBS << (uint32_t)0;
  // enough null padding for a null state
  // TODO: fill this in.
  mScriptBS.moveBy(31);

  // mark where the heap starts
  writeRegister(LREG_HR, mScriptBS.pos());
  mScriptBS.writeRawData(_mHeapManager.mHeapBS.data(), _mHeapManager.mHeapBS.size());
  // mark the top of the heap
  writeRegister(LREG_HP, mScriptBS.pos());
  return false;
}

void LSOCompilerVisitor::writeRegister(LSORegisters reg, uint32_t val) {
  ScopedBitStreamSeek seek(mScriptBS, LSO_REGISTER_OFFSETS[reg]);
  mScriptBS << val;
}

void LSOCompilerVisitor::writeEventRegister(LSORegisters reg, uint64_t val) {
  // TODO: v1 compatibility
  ScopedBitStreamSeek seek(mScriptBS, LSO_REGISTER_OFFSETS[reg]);
  mScriptBS << val;
}



LSLASTNode *resolve_sa_identifier(LSLASTNode *rvalue) {
  // try to figure out where the rvalue in the lvalue was actually declared
  while (rvalue && rvalue->getNodeSubType() == NODE_LVALUE_EXPRESSION) {
    auto *sym = rvalue->getSymbol();
    // This lvalue references a builtin symbol, we don't expect to be able to
    // follow that to a variable declaration, so we can break.
    if (sym->getSubType() == SYM_BUILTIN)
      break;
    // get the node where the referenced symbol was declared
    auto *decl_node = sym->getVarDecl();
    // this should always be set except in the builtin case, which we handled.
    assert(decl_node);
    // get the rvalue of the node where this global was declared.
    rvalue = decl_node->getChild(1);
  }
  return rvalue;
}

bool LSOCompilerVisitor::visit(LSLGlobalVariable *node) {
  // Weird special case for if we have only an lvalue reference in the rvalue
  // for compatibility with LL's LSO compiler. If the lvalue ultimately references
  // another global that has no rvalue, we must fill the global variable's data field
  // with all zeros and not create anything on the heap (if this is a heap type)
  //
  // for example:
  // rotation foo; rotation bar = foo; rotation baz = bar;
  // has foo == <0, 0, 0, 1> but bar and baz == <0, 0, 0, 0>
  // note that this is _not_ the case under Mono.
  // TODO: might make sense to have LSO-specific constant value propagation that accounts
  //  for this global lvalue weirdness as much as possible. Constant folding decisions or
  //  always true / always false warnings may be wrong otherwise.
  //  otherwise LSO compilation must always have optimization off and use SL-strict
  //  global validation because the semantics are closer to Mono. That might be an OK tradeoff.
  auto *rvalue = node->getChild(1);
  auto *sym = node->getSymbol();
  // Specifically take the symbol constant value by default and not the rvalue's since it
  // should have any automatic casts applied.
  auto *cv = sym->getConstantValue();
  if (rvalue->getNodeSubType() == NODE_LVALUE_EXPRESSION) {
    rvalue = resolve_sa_identifier(rvalue);

    // After following the lvalues all the way up the chain we hit a global
    // declaration with no rvalue
    if (!rvalue || rvalue->getNodeType() == NODE_NULL) {
      // invoke the stupid special case that fills the variable with all zeros
      // and skips to the next field. For heap types, this will crash your script
      // if you ever try to use the value since 0 is not a valid heap index.
      _mGlobalVarManager.writePlaceholder(sym->getIType());
      return false;
    }
  } else if (rvalue->getNodeSubType() == NODE_LIST_EXPRESSION) {
    // Need to do similar logic to resolve SAIdentifiers inside lists, which means
    // we can't use the existing list constant value, it might not be correct according
    // to our SAIdentifier propagation rules! Note that we can skip the "undefined rvalue"
    // case above because SAIdentifiers with no rvalue are not allowed in lists.
    //
    // This is needed to handle cases like `float f = 1; list l = [f];` where the list
    // is meant to contain a single integer under LSO, due to the integer rvalue.
    auto *list_children = rvalue->getChildren();
    auto *new_list_cv = _mAllocator->newTracked<LSLListConstant>(nullptr);
    while (list_children != nullptr) {
      auto *child_cv = resolve_sa_identifier(list_children)->getConstantValue();
      assert(child_cv);
      new_list_cv->pushChild(child_cv->copy(_mAllocator));
      list_children = list_children->getNext();
    }
    cv = new_list_cv;
  }

  _mGlobalVarManager.writeVar(cv, sym->getName());

  return false;
}


void LSOGlobalVarManager::writeVar(LSLConstant *constant, const char *name) {
  assert(constant != nullptr);
  // data offset, type, name
  mGlobalsBS << (uint32_t)6 << constant->getIType() << '\0';
  switch(constant->getIType()) {
    case LST_INTEGER:
      mGlobalsBS << ((LSLIntegerConstant *) constant)->getValue();
      break;
    case LST_FLOATINGPOINT:
      mGlobalsBS << ((LSLFloatConstant *) constant)->getValue();
      break;
    case LST_KEY:
    case LST_STRING:
      mGlobalsBS << _mHeapManager->writeConstant((LSLStringConstant *) constant);
      break;
    case LST_VECTOR:
      mGlobalsBS << *((LSLVectorConstant *) constant)->getValue();
      break;
    case LST_QUATERNION:
      mGlobalsBS << *((LSLQuaternionConstant *) constant)->getValue();
      break;
    case LST_LIST:
      mGlobalsBS << _mHeapManager->writeConstant((LSLListConstant *) constant);
      break;
    default:
      // just write all zeros
      mGlobalsBS.moveBy((int32_t)LSO_TYPE_DATA_SIZES[constant->getIType()], true);
  }
}

void LSOGlobalVarManager::writePlaceholder(LSLIType type) {
  mGlobalsBS << (uint32_t)6 << type << '\0';
  mGlobalsBS.moveBy((int32_t)LSO_TYPE_DATA_SIZES[type], true);
}


uint32_t LSOHeapManager::writeConstant(LSLConstant *constant) {
  // first byte on the heap has an index of 1, not 0. 0 is completely invalid.
  uint32_t heap_idx = mHeapBS.pos() + 1;
  LSLIType itype = constant->getIType();
  switch(itype) {
    case LST_INTEGER:
      writeHeader(4, LST_INTEGER);
      mHeapBS << ((LSLIntegerConstant *) constant)->getValue();
      break;
    case LST_FLOATINGPOINT:
      writeHeader(4, LST_FLOATINGPOINT);
      mHeapBS << ((LSLFloatConstant *) constant)->getValue();
      break;
    case LST_KEY:
    case LST_STRING:
    {
      auto *str_val = ((LSLStringConstant *) constant)->getValue();
      auto str_len = strlen(str_val);

      // Keys are always added to the heap as strings in the global case.
      // they can only be added as LST_KEY in the runtime list expression case
      // which we don't need to handle.
      writeHeader(str_len + 1, LST_STRING);
      mHeapBS.writeRawData((uint8_t *) str_val, str_len);
      mHeapBS << '\0';
      break;
    }
    case LST_VECTOR:
      writeHeader(12, LST_VECTOR);
      mHeapBS << *((LSLVectorConstant *) constant)->getValue();
      break;
    case LST_QUATERNION:
      writeHeader(16, LST_QUATERNION);
      mHeapBS << *((LSLQuaternionConstant *) constant)->getValue();
      break;
    case LST_LIST: {
      auto *list_val = ((LSLListConstant *)constant);
      // each child is also created on the heap, and gets its own entry with an index.
      uint32_t list_len = list_val->getLength();
      uint32_t children_required_bytes = (sizeof(uint32_t) * list_len);
      // need space for each child's index as well as the leading child count
      writeHeader(sizeof(uint32_t) + children_required_bytes, LST_LIST);
      mHeapBS << (uint32_t)list_len;

      // skip by for now and start creating the heap entries for the children
      uint32_t list_children_start = mHeapBS.pos();
      mHeapBS.moveBy((int32_t) children_required_bytes, true);

      std::vector<uint32_t> child_idxs;
      auto *child = list_val->getValue();
      while(child != nullptr) {
        // write the child, noting where on the heap it was written
        child_idxs.emplace_back(writeConstant(child));
        child = (LSLConstant*) child->getNext();
      }

      {
        // temporarily seek back to the start of the start of the list's
        // heap value and write in the child IDs
        ScopedBitStreamSeek seek(mHeapBS, list_children_start);
        for (auto child_idx: child_idxs) {
          mHeapBS << child_idx;
        }
      }
      break;
    }
    case LST_NULL:
      writeHeader(0, LST_NULL);
      break;
    case LST_ERROR:
    case LST_MAX:
      assert(0);
  }
  return heap_idx;
}

void LSOHeapManager::writeHeader(uint32_t size, LSLIType type, uint16_t ref_count) {
  // create an appropriately-sized entry with a refcount of 1
  mHeapBS << size << type << (uint16_t)ref_count;
}

uint32_t LSOHeapManager::writeTerminalBlock() {
  uint16_t start_pos = mHeapBS.pos();
  writeHeader(TOTAL_LSO_MEMORY, LST_NULL, 0);
  return start_pos + 1;
}


}
