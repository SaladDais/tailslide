#include "bytecode_format.hh"
#include "lso_compiler.hh"

namespace Tailslide {

bool LSOCompilerVisitor::visit(LSLScript *node) {
  // allocate all of the script memory and fill with zeros
  mScriptBS.makeSpace(TOTAL_LSO_MEMORY);
  // write the first few registers whose values are mostly
  // always the same during compilation
  mScriptBS
      << (uint32_t)TOTAL_LSO_MEMORY // TM
      << (uint32_t)0 // IP
      << (uint32_t)LSO_VERSION_NUM // VN
      << (uint32_t)TOTAL_LSO_MEMORY - 1 // BP
      << (uint32_t)TOTAL_LSO_MEMORY - 1 // SP
      ;
  // Need to queue up a state_entry event for when the script starts
  mScriptBS.moveTo(LSORegisterOffsets[LREG_NCE]);
  mScriptBS << (uint64_t)LSOH_STATE_ENTRY;
  return false;
}

uint32_t LSOHeapManager::writeConstant(LSLConstant *constant) {
  uint32_t start_pos = mHeapBS.pos();
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

      writeHeader(str_len + 1, itype);
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
      // each child is also created on the heap, and gets its own ID.
      uint32_t required_bytes = sizeof(uint32_t) * list_val->getLength();
      writeHeader(required_bytes, LST_LIST);

      uint32_t list_val_start = mHeapBS.pos();
      // skip by for now and start creating the heap entries for the children
      mHeapBS.moveBy((int32_t) required_bytes, true);

      std::vector<uint32_t> child_positions;
      auto *child = list_val->getValue();
      while(child != nullptr) {
        // write the child, noting where on the heap it was written
        child_positions.emplace_back(writeConstant(child));
        child = (LSLConstant*) child->getNext();
      }

      {
        // temporarily seek back to the start of the start of the list's
        // heap value and write in the child IDs
        ScopedBitStreamSeek seek(mHeapBS, list_val_start);
        for (auto child_idx: child_positions) {
          mHeapBS << child_idx;
        }
      }
    }
    case LST_NULL:
      writeHeader(0, LST_NULL);
      break;
    case LST_ERROR:
    case LST_MAX:
      assert(0);
  }
  return start_pos;
}

void LSOHeapManager::writeHeader(uint32_t size, LSLIType type) {
  // create an appropriately-sized entry with a refcount of 1
  mHeapBS << size << type << (uint16_t) 1;
}

}
