#include "tailslide.hh"
#include "doctest.hh"

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

TEST_SUITE_END();
