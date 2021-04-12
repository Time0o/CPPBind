local test = require 'test_variables'

assert(test.TEST_ENUM_1 == 1)
assert(test.TEST_ENUM_2 == 2)

assert(test.TEST_ANONYMOUS_ENUM_1 == 1)
assert(test.TEST_ANONYMOUS_ENUM_2 == 2)

assert(test.TEST_SCOPED_ENUM_SCOPED_ENUM_1 == 1)
assert(test.TEST_SCOPED_ENUM_SCOPED_ENUM_2 == 2)

assert(test.TEST_UNSIGNED_CONSTEXPR_1 == 1)
assert(test.TEST_DOUBLE_CONSTEXPR_1 == 1.0)

assert(test.MACRO_CONST == 1)
assert(test.MACRO_EXPR == ~(1 << 3))
