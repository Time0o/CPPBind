local test = require 'test_default_arguments'

assert(test.test_pow_default_arguments() == test.TEST_E)
assert(test.test_pow_default_arguments(10.0) == 10.0)
assert(test.test_pow_default_arguments(test.TEST_E, 2) == test.TEST_E * test.TEST_E)
assert(test.test_pow_default_arguments(test.TEST_E, 2, true) == 7.0)
assert(test.test_pow_default_arguments(test.TEST_E, 2, true, test.TEST_ROUND_UPWARD) == 8.0)

assert(test.test_default_large_signed() == test.TEST_LARGE_SIGNED)
assert(test.test_default_constexpr_function_call() == 1)
assert(test.test_default_function_call() == 1)
--assert(test.test_default_ref(0) == 42);
--assert(test.test_default_record():get_value() == 42)
--assert(test.test_default_template_unsigned_int() == 1)
--assert(test.test_default_template_after_parameter_pack_unsigned_int_int_int(1, 2) == 1)
