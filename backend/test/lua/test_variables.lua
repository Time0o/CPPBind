local test = require 'test_variables'

assert(test.test.ENUM_1 == 1)
assert(test.test.ENUM_2 == 2)

assert(test.test.SCOPED_ENUM_SCOPED_ENUM_1 == 1)
assert(test.test.SCOPED_ENUM_SCOPED_ENUM_2 == 2)

assert(test.test.ANONYMOUS_ENUM_1 == 1)
assert(test.test.ANONYMOUS_ENUM_2 == 2)

assert(test.test.ENUM_IN_ANONYMOUS_NAMESPACE_1 == 1)
assert(test.test.ENUM_IN_ANONYMOUS_NAMESPACE_2 == 2)

assert(test.test.UNSIGNED_CONSTEXPR == 1)
assert(test.test.DOUBLE_CONSTEXPR == 1.0)

assert(test.test.INT_VAR == 1)

test.test.INT_VAR = 2
assert(test.test.INT_VAR == 2)

-- XXX macros
