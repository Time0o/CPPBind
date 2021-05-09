local test = require 'test_classes'

-- construction
do
  local trivial = test.test.Trivial.new()
  assert(test.test.Trivial.delete == nil)
end

assert(test.test.NonConstructible.new == nil)
assert(test.test.NonConstructible.delete == nil)
assert(test.test.ImplicitlyNonConstructible.new == nil)
assert(test.test.ImplicitlyNonConstructible.delete == nil)

-- member access
do
  local a_class = test.test.AClass.new_1()
  assert(a_class:get_state() == 0)
end

do
  local a_class = test.test.AClass.new_2(1)
  assert(a_class:get_state() == 1)

  a_class:set_state(2)
  assert(a_class:get_state() == 2)
end

test.test.AClass.set_static_state(1)
assert(test.test.AClass.get_static_state() == 1)

-- destruction
collectgarbage()
collectgarbage()

assert(test.test.AClass.get_num_destroyed() == 2)

-- copying and moving
do
  assert(test.test.CopyableClass._move == nil)

  local copyable_class = test.test.CopyableClass.new(1)
  assert(copyable_class:get_state() == 1)

  assert(test.test.CopyableClass.get_num_copied() == 0)

  copyable_class_copy = copyable_class:copy()
  assert(copyable_class_copy:get_state() == 1)

  copyable_class_copy:set_state(2)
  assert(copyable_class:get_state() == 1)
  assert(copyable_class_copy:get_state() == 2)

  assert(test.test.CopyableClass.get_num_copied() == 1)
end

do
  assert(test.test.MoveableClass._copy == nil)

  local moveable_class = test.test.MoveableClass.new(1)
  assert(moveable_class:get_state() == 1)

  assert(test.test.MoveableClass.get_num_moved() == 0)

  moveable_class_moved = moveable_class:move()
  assert(moveable_class_moved:get_state() == 1)

  assert(test.test.MoveableClass.get_num_moved() == 1)
end

-- class parameters

-- value parameters
do
  local a = test.test.ClassParameter.new(1);
  local b = test.test.ClassParameter.new(2);

  test.test.ClassParameter.set_copyable(true);

  local c = test.test.add_class(a, b);
  assert(a:get_state() == 1);
  assert(c:get_state() == 3);

  test.test.ClassParameter.set_copyable(false);
end

-- pointer parameters
do
  local a = test.test.ClassParameter.new(1)
  local b = test.test.ClassParameter.new(2)

  test.test.ClassParameter.set_copyable(true)

  local c = test.test.add_class_pointer(a, b)
  assert(a:get_state() == 3)
  assert(c:get_state() == 3)

  test.test.ClassParameter.set_copyable(false)
end

-- lvalue reference parameters
do
  local a = test.test.ClassParameter.new(1)
  local b = test.test.ClassParameter.new(2)

  test.test.ClassParameter.set_copyable(true)

  local c = test.test.add_class_lvalue_ref(a, b)
  assert(a:get_state() == 3)
  assert(c:get_state() == 3)

  test.test.ClassParameter.set_copyable(false)
end

-- rvalue reference parameters
do
  local a = test.test.ClassParameter.new(1)

  test.test.ClassParameter.set_moveable(true)

  test.test.noop_class_rvalue_ref(a)
  assert(a:was_moved())

  test.test.ClassParameter.set_moveable(false)
end
