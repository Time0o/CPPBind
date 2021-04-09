local test = require 'test_forward_declarations'

assert(test.test_add_1(1, 2) == 3)
assert(test.test_add_2(1, 2, 3) == 6)
assert(test.test_add_3 == nil)

do
  local adder = test.TestAdder.new()

  assert(adder:add(1, 2) == 3)
end
