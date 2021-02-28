local test = require 'test_templates'

-- function templates
assert(test.test_add_int(1, 2) == 3)
assert(test.test_add_long(1, 2) == 3)

---- multiple template parameters
assert(test.test_add_int(1, 2) == 3)
assert(test.test_mult_int_long(2, 3) == 6)
assert(test.test_mult_long_int(2, 3) == 6)

---- non-type template parameters
assert(test.test_inc_int_1(1) == 2)
assert(test.test_inc_int_2(1) == 3)
assert(test.test_inc_int_3(1) == 4)

-- default template arguments
assert(test.test_pi_int() == 3)
assert(test.test_pi_double() == test.TEST_PI)

-- template parameter packs
assert(test.test_sum_int_int(1, 2) == 3)
assert(test.test_sum_int_int_long_long(1, 2, 3, 4) == 10)

-- record templates
do
  local any_stack = test.TestAnyStackInt.new()

  any_stack:push_int(1)
  any_stack:push_double(3.14)

  assert(any_stack:pop_double() == 3.14)
  assert(any_stack:pop_int() == 1)

  assert(any_stack:empty())
end
