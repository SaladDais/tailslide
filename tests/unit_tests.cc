#include "tailslide.hh"
#include "doctest.hh"
#include "bitstream.hh"

using namespace Tailslide;

TEST_SUITE_BEGIN("Unit tests");

TEST_CASE("Get parent slot") {
  ScriptAllocator allocator;
  ScriptContext context {
    nullptr,
    &allocator
  };
  allocator.setContext(&context);

  auto *int_const = allocator.newTracked<LSLIntegerConstant>(1);
  auto *list_const = allocator.newTracked<LSLListConstant>(nullptr);
  list_const->pushChild(allocator.newTracked<LSLIntegerConstant>(1));
  list_const->pushChild(allocator.newTracked<LSLIntegerConstant>(2));
  CHECK_EQ(int_const->getParentSlot(), -1);
  list_const->pushChild(int_const);
  CHECK_EQ(int_const->getParentSlot(), 2);
}

TEST_CASE("BitStream int writing") {
  BitStream bs_big(ENDIAN_BIG);
  bs_big << (int32_t)1 << (uint16_t)2;
  CHECK_EQ(bs_big.size(), 6);
  CHECK_EQ(bs_big.data()[0], 0x00);
  CHECK_EQ(bs_big.data()[3], 0x01);
  CHECK_EQ(bs_big.data()[5], 0x02);

  BitStream bs_little(ENDIAN_LITTLE);
  bs_little << (int32_t)1 << (uint16_t)2;
  CHECK_EQ(bs_little.size(), 6);
  CHECK_EQ(bs_little.data()[0], 0x01);
  CHECK_EQ(bs_little.data()[3], 0x00);
  CHECK_EQ(bs_little.data()[4], 0x02);
}

TEST_CASE("BitStream int reading") {
  BitStream bs_big(ENDIAN_BIG);
  int32_t val_1;
  uint16_t val_2;
  bs_big << (int32_t)1 << (uint16_t)2;
  bs_big.moveTo(0);
  bs_big >> val_1;
  bs_big >> val_2;
  CHECK_EQ(val_1, 1);
  CHECK_EQ(val_2, 2);

  BitStream bs_little(ENDIAN_LITTLE);
  bs_little << (int32_t)1 << (uint16_t)2;
  bs_big.moveTo(0);
  bs_big >> val_1;
  bs_big >> val_2;
  CHECK_EQ(val_1, 1);
  CHECK_EQ(val_2, 2);
}

TEST_CASE("Bitstream equality") {
  BitStream bs1, bs2, bs3;
  bs1 << "foobar";
  bs2 << "foobaz";
  bs3 << "foobar";

  CHECK_EQ(bs1, bs1);
  CHECK_NE(bs1, bs2);
  CHECK_EQ(bs1, bs3);
}

TEST_CASE("Bitstream duplicate") {
  BitStream bs1;
  size_t v1 = 42;
  size_t v2 = 23;

  bs1 << v1;
  auto pos = bs1.pos();
  bs1 << v2;
  bs1.moveTo(pos);

  auto bs2 = bs1.duplicate(true);
  auto bs3 = bs1.duplicate();

  size_t res1, res2;
  bs2 >> res1;
  bs3 >> res2;

  CHECK_EQ(res1, v2);
  CHECK_EQ(res2, v2);
}

TEST_CASE("Bitstream view")
{
  BitStream bs1;
  size_t v1 = 42;
  size_t v2 = 23;

  bs1 << v1;
  bs1 << v2;

  auto bs2 = bs1.makeView();

  size_t res1, res2;
  bs2 >> res1 >> res2;

  CHECK_EQ(res1, v1);
  CHECK_EQ(res2, v2);
}

TEST_CASE("Bitstream read_only")
{
  BitStream bs1, bs2;

  size_t val = 42;
  bs1 << val;

  bs2.assign(bs1.data(), (uint32_t)bs1.size(), true);

  CHECK_FALSE(bs1.isReadOnly());
  CHECK(bs2.isReadOnly());
}

TEST_CASE("LLoc comparison works correctly") {
  TailslideLType smaller {0, 1, 2, 3};
  TailslideLType bigger {1, 1, 2, 3};
  TailslideLType smaller_sameline {1, 0, 2, 3};
  CHECK(bigger > smaller);
  CHECK(bigger > smaller_sameline);
  CHECK(smaller < bigger);
  CHECK(smaller_sameline < bigger);
}

TEST_SUITE_END();
