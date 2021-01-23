local test = require 'test_default_arguments'

assert(test.test_pow_default_arguments() == test.TEST_E)
assert(test.test_pow_default_arguments(10.0) == 10.0)
assert(test.test_pow_default_arguments(test.TEST_E, 2) == test.TEST_E * test.TEST_E)
assert(test.test_pow_default_arguments(test.TEST_E, 2, true) == 7.0)
assert(test.test_pow_default_arguments(test.TEST_E, 2, true, test.TEST_ROUND_UPWARD) == 8.0)
