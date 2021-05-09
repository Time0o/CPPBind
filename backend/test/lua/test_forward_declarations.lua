local test = require 'test.forward_declarations'

assert(test.test.add_1(1, 2) == 3)
assert(test.test.add_2(1, 2, 3) == 6)
assert(test.test.add_3 == nil)

do
  local adder = test.test.Adder.new()

  assert(adder:add(1, 2) == 3)
end
