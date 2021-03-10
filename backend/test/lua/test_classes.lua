local test = require 'test_classes'

-- construction
do
  local trivial = test.TestTrivial.new()
  assert(test.TestTrivial.delete == nil)
end

do
  assert(test.TestNonConstructible.new == nil)
  assert(test.TestNonConstructible.delete == nil)
end

-- member access
do
  local a_class = test.TestAClass.new_1()
  assert(a_class:get_state() == 0)
end

do
  local a_class = test.TestAClass.new_2(1)
  assert(a_class:get_state() == 1)

  a_class:set_state(2)
  assert(a_class:get_state() == 2)
end

test.TestAClass.set_static_state(1)
assert(test.TestAClass.get_static_state() == 1)

-- destruction
collectgarbage()
collectgarbage()

assert(test.TestAClass.get_num_destroyed() == 2)

-- copying and moving
do
  assert(test.TestCopyableClass._move == nil)

  local copyable_class = test.TestCopyableClass.new(1)
  assert(copyable_class:get_state() == 1)

  assert(test.TestCopyableClass.get_num_copied() == 0)

  copyable_class_copy = copyable_class:copy()
  assert(copyable_class_copy:get_state() == 1)

  copyable_class_copy:set_state(2)
  assert(copyable_class:get_state() == 1)
  assert(copyable_class_copy:get_state() == 2)

  assert(test.TestCopyableClass.get_num_copied() == 1)
end

do
  assert(test.TestMoveableClass._copy == nil)

  local moveable_class = test.TestMoveableClass.new(1)
  assert(moveable_class:get_state() == 1)

  assert(test.TestMoveableClass.get_num_moved() == 0)

  moveable_class_moved = moveable_class:move()
  assert(moveable_class_moved:get_state() == 1)

  assert(test.TestMoveableClass.get_num_moved() == 1)
end

-- class parameters

-- value parameters
do
  local a = test.TestClassParameter.new(1);
  local b = test.TestClassParameter.new(2);

  test.TestClassParameter.set_copyable(true);

  local c = test.test_add_class(a, b);
  assert(a:get_state() == 1);
  assert(c:get_state() == 3);

  test.TestClassParameter.set_copyable(false);
end

-- pointer parameters
do
  local a = test.TestClassParameter.new(1)
  local b = test.TestClassParameter.new(2)

  test.TestClassParameter.set_copyable(true)

  local c = test.test_add_class_pointer(a, b)
  assert(a:get_state() == 3)
  assert(c:get_state() == 3)

  test.TestClassParameter.set_copyable(false)
end

-- lvalue reference parameters
do
  local a = test.TestClassParameter.new(1)
  local b = test.TestClassParameter.new(2)

  test.TestClassParameter.set_copyable(true)

  local c = test.test_add_class_lvalue_ref(a, b)
  assert(a:get_state() == 3)
  assert(c:get_state() == 3)

  test.TestClassParameter.set_copyable(false)
end

-- rvalue reference parameters
do
  local a = test.TestClassParameter.new(1)

  test.TestClassParameter.set_moveable(true)

  test.test_noop_class_rvalue_ref(a)
  assert(a:was_moved())

  test.TestClassParameter.set_moveable(false)
end
