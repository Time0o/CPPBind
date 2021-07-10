require 'test_classes'

-- construction
do
  local trivial = test.Trivial.new()
  assert(test.Trivial.delete == nil)
end

assert(test.NonConstructible.new == nil)
assert(test.NonConstructible.delete == nil)
assert(test.ImplicitlyNonConstructible.new == nil)
assert(test.ImplicitlyNonConstructible.delete == nil)

-- member access
do
  local a_class = test.AClass.new_1()
  assert(a_class:get_state() == 0)
end

collectgarbage()
collectgarbage()

assert(test.AClass.get_num_destroyed() == 1)

do
  local a_class = test.AClass.new_2(1)
  assert(a_class:get_state() == 1)

  a_class:set_state(2)
  assert(a_class:get_state() == 2)

  a_class:delete()
end

assert(test.AClass.get_num_destroyed() == 2)

test.AClass.set_static_state(1)
assert(test.AClass.get_static_state() == 1)

-- copying and moving
do
  assert(test.CopyableClass._move == nil)

  local copyable_class = test.CopyableClass.new(1)
  assert(copyable_class:get_state() == 1)

  assert(test.CopyableClass.get_num_copied() == 0)

  copyable_class_copy = copyable_class:copy()
  assert(copyable_class_copy:get_state() == 1)

  copyable_class_copy:set_state(2)
  assert(copyable_class:get_state() == 1)
  assert(copyable_class_copy:get_state() == 2)

  assert(test.CopyableClass.get_num_copied() == 1)
end

do
  assert(test.MoveableClass._copy == nil)

  local moveable_class = test.MoveableClass.new(1)
  assert(moveable_class:get_state() == 1)

  assert(test.MoveableClass.get_num_moved() == 0)

  moveable_class_moved = moveable_class:move()
  assert(moveable_class_moved:get_state() == 1)

  assert(test.MoveableClass.get_num_moved() == 1)
end

-- class parameters

-- value parameters
do
  local a = test.ClassParameter.new(1);
  local b = test.ClassParameter.new(2);

  test.ClassParameter.set_copyable(true);

  local c = test.add_class(a, b);
  assert(a:get_state() == 1);
  assert(c:get_state() == 3);

  test.ClassParameter.set_copyable(false);
end

-- pointer parameters
do
  local a = test.ClassParameter.new(1)
  local b = test.ClassParameter.new(2)

  test.ClassParameter.set_copyable(true)

  local c = test.add_class_pointer(a, b)
  assert(a:get_state() == 3)
  assert(c:get_state() == 3)

  test.ClassParameter.set_copyable(false)
end

-- lvalue reference parameters
do
  local a = test.ClassParameter.new(1)
  local b = test.ClassParameter.new(2)

  test.ClassParameter.set_copyable(true)

  local c = test.add_class_lvalue_ref(a, b)
  assert(a:get_state() == 3)
  assert(c:get_state() == 3)

  test.ClassParameter.set_copyable(false)
end

-- rvalue reference parameters
do
  local a = test.ClassParameter.new(1)

  test.ClassParameter.set_moveable(true)

  test.noop_class_rvalue_ref(a)
  assert(a:was_moved())

  test.ClassParameter.set_moveable(false)
end
