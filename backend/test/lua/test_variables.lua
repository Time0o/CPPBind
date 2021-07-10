require 'test_variables'

assert(test.ENUM_1 == 1)
assert(test.ENUM_2 == 2)

assert(test.SCOPED_ENUM_SCOPED_ENUM_1 == 1)
assert(test.SCOPED_ENUM_SCOPED_ENUM_2 == 2)

assert(test.ANONYMOUS_ENUM_1 == 1)
assert(test.ANONYMOUS_ENUM_2 == 2)

assert(test.ENUM_IN_ANONYMOUS_NAMESPACE_1 == 1)
assert(test.ENUM_IN_ANONYMOUS_NAMESPACE_2 == 2)

assert(test.get_unsigned_constexpr() == 1)
assert(test.get_double_constexpr() == 1.0)

assert(test.get_int_var() == 1)

test.set_int_var(2)
assert(test.get_int_var() == 2)

assert(get_macro_const() == 1)
assert(get_macro_expr() == ~(1 << 3))
