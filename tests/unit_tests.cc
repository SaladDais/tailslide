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
  allocator.set_context(&context);

  auto *int_const = allocator.new_tracked<LSLIntegerConstant>(1);
  auto *list_const = allocator.new_tracked<LSLListConstant>(nullptr);
  list_const->push_child(allocator.new_tracked<LSLIntegerConstant>(1));
  list_const->push_child(allocator.new_tracked<LSLIntegerConstant>(2));
  CHECK_EQ(int_const->get_parent_slot(), -1);
  list_const->push_child(int_const);
  CHECK_EQ(int_const->get_parent_slot(), 2);
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
  bs_big.move_to(0);
  bs_big >> val_1;
  bs_big >> val_2;
  CHECK_EQ(val_1, 1);
  CHECK_EQ(val_2, 2);

  BitStream bs_little(ENDIAN_LITTLE);
  bs_little << (int32_t)1 << (uint16_t)2;
  bs_big.move_to(0);
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
  bs1.move_to(pos);

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

  auto bs2 = bs1.make_view();

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

  bs2.assign(bs1.data(), bs1.size(), true);

  CHECK_FALSE(bs1.is_read_only());
  CHECK(bs2.is_read_only());
}

TEST_SUITE_END();
