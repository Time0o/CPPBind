local test = require 'test_overloaded_operators'

do
  local pointer = test.TestPointer.new(1)

  local pointee1 = pointer:deref()
  assert(pointee1:get_state() == 1)

  pointee1:set_state(2)
  local pointee2 = pointer:deref()
  assert(pointee2:get_state() == 2)
end

do
  local pointer = test.TestPointer.new(1)

  local pointee1 = pointer:access()
  assert(pointee1:get_state() == 1)

  pointee1:set_state(2)
  local pointee2 = pointer:access()
  assert(pointee2:get_state() == 2)
end

do
  local class = test.TestClassWithCallableMember.new()

  assert(class:sum(1, 2) == 3)
end
