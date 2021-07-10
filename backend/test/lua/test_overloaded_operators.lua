require 'test_overloaded_operators'

do
  local i1 = test.Integer.new(2)
  assert(i1:deref() == 2)

  local i2 = test.Integer.new(3)
  assert(i2:deref() == 3)

  local i3 = test.mult(i1, i2)
  assert(i3:deref() == 6)

  local i1_inc = i1:inc_pre()
  assert(i1_inc:deref() == 3)
  assert(i1:deref() == 3)

  local i2_inc = i2:inc_post()
  assert(i2_inc:deref() == 3)
  assert(i2:deref() == 4)

  local i0 = test.Integer.new(0)
  assert(i0:cast_bool() == false)
  assert(i1:cast_bool() == true)
end

do
  local pointer = test.Pointer.new(1)

  local pointee1 = pointer:deref()
  assert(pointee1:get_state() == 1)

  pointee1:set_state(2)
  local pointee2 = pointer:deref()
  assert(pointee2:get_state() == 2)
end

do
  local pointer = test.Pointer.new(1)

  local pointee1 = pointer:access()
  assert(pointee1:get_state() == 1)

  pointee1:set_state(2)
  local pointee2 = pointer:access()
  assert(pointee2:get_state() == 2)
end

do
  local class = test.ClassWithCallableMember.new()

  assert(class:sum(1, 2) == 3)
end
