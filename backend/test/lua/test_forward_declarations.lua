local test = require 'test_forward_declarations'

assert(test.test_add(1, 2) == 3)

do
  local adder = test.TestAdder.new()

  assert(adder:add(1, 2) == 3)
end
