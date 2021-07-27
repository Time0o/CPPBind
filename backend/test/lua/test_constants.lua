require 'test_constants'

assert(test.ENUM_1 == 1)
assert(test.ENUM_2 == 2)

assert(test.SCOPED_ENUM_SCOPED_ENUM_1 == 1)
assert(test.SCOPED_ENUM_SCOPED_ENUM_2 == 2)

assert(test.ANONYMOUS_ENUM_1 == 1)
assert(test.ANONYMOUS_ENUM_2 == 2)

assert(test.DUPLICATE_ENUM_1 == 1)
assert(test.DUPLICATE_ENUM_2 == 1)

assert(test.ENUM_IN_ANONYMOUS_NAMESPACE_1 == 1)
assert(test.ENUM_IN_ANONYMOUS_NAMESPACE_2 == 2)

assert(get_macro_const() == 1)
assert(get_macro_expr() == ~(1 << 3))
