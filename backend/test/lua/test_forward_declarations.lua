require 'test_forward_declarations'

assert(test.add_1(1, 2) == 3)
assert(test.add_2(1, 2, 3) == 6)
assert(test.add_3 == nil)

do
  local adder = test.Adder.new()

  assert(adder:add(1, 2) == 3)
end
