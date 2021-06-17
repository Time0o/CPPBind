local test = require 'test_templates'

-- function templates
assert(test.test.add_int(1, 2) == 3)
assert(test.test.add_long(1, 2) == 3)

---- multiple template parameters
assert(test.test.add_int(1, 2) == 3)
assert(test.test.mult_int_long(2, 3) == 6)
assert(test.test.mult_long_int(2, 3) == 6)

---- non-type template parameters
assert(test.test.inc_int_1(1) == 2)
assert(test.test.inc_int_2(1) == 3)
assert(test.test.inc_int_3(1) == 4)

-- default template arguments
assert(test.test.pi_int() == 3)
assert(test.test.pi_double() == test.test.get_pi())

-- template parameter packs
assert(test.test.sum_int_int(1, 2) == 3)
assert(test.test.sum_int_int_long_long(1, 2, 3, 4) == 10)

-- record templates
do
  local any_stack = test.test.AnyStackInt.new()

  any_stack:push_int(1)
  any_stack:push_double(3.14)

  assert(any_stack:pop_double() == 3.14)
  assert(any_stack:pop_int() == 1)

  assert(any_stack:empty())
end
