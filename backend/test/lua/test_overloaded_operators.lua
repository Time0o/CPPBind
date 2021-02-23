local test = require 'test_overloaded_operators'

do
  local pointer = test.TestPointer.new(1)

  local pointee1 = pointer:operator_arrow()
  assert(pointee1:get_state() == 1)

  pointee1:set_state(2)
  local pointee2 = pointer:operator_arrow()
  assert(pointee2:get_state() == 2)
end
