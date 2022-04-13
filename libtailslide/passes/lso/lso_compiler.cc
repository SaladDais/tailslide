#include "bytecode_format.hh"
#include "lso_compiler.hh"

namespace Tailslide {

bool LSOCompilerVisitor::visit(LSLScript *node) {
  // allocate all of the script memory and fill with zeros
  script_bs.make_space(TOTAL_LSO_MEMORY);
  // write the first few registers whose values are mostly
  // always the same during compilation
  script_bs
      << (uint32_t)TOTAL_LSO_MEMORY // TM
      << (uint32_t)0 // IP
      << (uint32_t)LSO_VERSION_NUM // VN
      << (uint32_t)TOTAL_LSO_MEMORY - 1 // BP
      << (uint32_t)TOTAL_LSO_MEMORY - 1 // SP
      ;
  // Need to queue up a state_entry event for when the script starts
  script_bs.move_to(LSORegisterOffsets[LREG_NCE]);
  script_bs << (uint64_t)LSOH_STATE_ENTRY;
  return false;
}

uint32_t LSOHeapManager::writeConstant(LSLConstant *constant) {
  uint32_t start_pos = heap_bs.pos();
  LST_TYPE itype = constant->get_itype();
  switch(itype) {
    case LST_INTEGER:
      writeHeader(4, LST_INTEGER);
      heap_bs << ((LSLIntegerConstant *)constant)->get_value();
      break;
    case LST_FLOATINGPOINT:
      writeHeader(4, LST_FLOATINGPOINT);
      heap_bs << ((LSLFloatConstant *)constant)->get_value();
      break;
    case LST_KEY:
    case LST_STRING:
    {
      auto *str_val = ((LSLStringConstant *)constant)->get_value();
      auto str_len = strlen(str_val);

      writeHeader(str_len + 1, itype);
      heap_bs.write_raw_data((uint8_t *)str_val, str_len);
      heap_bs << '\0';
      break;
    }
    case LST_VECTOR:
      writeHeader(12, LST_VECTOR);
      heap_bs << *((LSLVectorConstant *)constant)->get_value();
      break;
    case LST_QUATERNION:
      writeHeader(16, LST_QUATERNION);
      heap_bs << *((LSLQuaternionConstant *)constant)->get_value();
      break;
    case LST_LIST: {
      auto *list_val = ((LSLListConstant *)constant);
      // each child is also created on the heap, and gets its own ID.
      uint32_t required_bytes = sizeof(uint32_t) * list_val->get_length();
      writeHeader(required_bytes, LST_LIST);

      uint32_t list_val_start = heap_bs.pos();
      // skip by for now and start creating the heap entries for the children
      heap_bs.move_by((int32_t)required_bytes, true);

      std::vector<uint32_t> child_positions;
      auto *child = list_val->get_value();
      while(child != nullptr) {
        // write the child, noting where on the heap it was written
        child_positions.emplace_back(writeConstant(child));
        child = (LSLConstant*)child->get_next();
      }

      {
        // temporarily seek back to the start of the start of the list's
        // heap value and write in the child IDs
        ScopedBitStreamSeek _seek(heap_bs, list_val_start);
        for (auto child_idx: child_positions) {
          heap_bs << child_idx;
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

void LSOHeapManager::writeHeader(uint32_t size, LST_TYPE type) {
  // create an appropriately-sized entry with a refcount of 1
  heap_bs << size << type << (uint16_t) 1;
}

}
