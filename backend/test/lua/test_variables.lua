require 'test_variables'

assert(test.get_unsigned_constexpr() == 1)
assert(test.get_double_constexpr() == 1.0)

assert(test.get_int_var() == 1)

test.set_int_var(2)
assert(test.get_int_var() == 2)
