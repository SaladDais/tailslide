// This file was originally part of the Luau programming language and is licensed under MIT License;
#include "tailslide.hh"
#include "doctest.hh"
#include "testutils.hh"
#include "passes/lso/lso_compiler.hh"
#include "passes/lso/bytecode_format.hh"

using namespace Tailslide;

TEST_SUITE_BEGIN("LSO");

TEST_CASE("Simple register initialization") {
  auto script = runConformance("constprop.lsl");
  LSOCompilerVisitor visitor;
  script->script->visit(&visitor);
  LSOBitStream script_bs(std::move(visitor.mScriptBS));

  script_bs.moveTo(LSORegisterOffsets[LREG_VN]);
  uint32_t version_num;
  script_bs >> version_num;
  CHECK_EQ(version_num, LSO_VERSION_NUM);
}

TEST_CASE("Simple heap compilation") {
  ScopedScriptParser parser(nullptr);

  // create a list and write it (and its contents) to the heap
  auto *list_const = parser.allocator.newTracked<LSLListConstant>(nullptr);
  list_const->pushChild(parser.allocator.newTracked<LSLIntegerConstant>(1));
  auto *vec_child = parser.allocator.newTracked<LSLVectorConstant>(1, 2, 3);
  list_const->pushChild(vec_child);
  LSOHeapManager heap_manager;
  heap_manager.writeConstant(list_const);

  LSOBitStream heap_bs(std::move(heap_manager.mHeapBS));
  CHECK_EQ(heap_bs.size(), 52);
  CHECK_EQ(heap_bs.pos(), 52);
  heap_bs.moveTo(0);
  uint32_t size, child_1_idx, child_2_idx;
  LSLIType type;
  uint16_t ref_count;
  heap_bs >> size >> type >> ref_count >> child_1_idx >> child_2_idx;
  // 2 elements, so two 32-bit heap ptrs
  CHECK_EQ(size, 8);
  CHECK_EQ(type, LST_LIST);
  CHECK_EQ(ref_count, 1);
  CHECK_EQ(child_1_idx, 15);
  CHECK_EQ(child_2_idx, 26);

  // move to the vector value on the heap
  float z;
  const uint32_t vec_pos = 26 + 4 + 1 + 2;
  heap_bs.moveTo(vec_pos);
  Vector3 v;
  heap_bs >> v;
  // make sure the vector matches what we meant to write
  CHECK_EQ(*vec_child->getValue(), v);

  // z comes first under LSO serialization
  heap_bs.moveTo(vec_pos);
  heap_bs >> z;
  CHECK_EQ(z, 3.0f);
}

TEST_SUITE_END();
