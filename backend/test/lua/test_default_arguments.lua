local test = require 'test.default_arguments'

assert(test.test.pow_default_arguments() == test.test.get_e())
assert(test.test.pow_default_arguments(10.0) == 10.0)
assert(test.test.pow_default_arguments(test.test.get_e(), 2) == test.test.get_e() * test.test.get_e())
assert(test.test.pow_default_arguments(test.test.get_e(), 2, true) == 7.0)
assert(test.test.pow_default_arguments(test.test.get_e(), 2, true, test.test.ROUND_UPWARD) == 8.0)

assert(test.test.default_large_signed() == test.test.LARGE_SIGNED)
assert(test.test.default_constexpr_function_call() == 1)
assert(test.test.default_function_call() == 1)
assert(test.test.default_ref(0) == 42);
assert(test.test.default_record():get_value() == 42)
assert(test.test.default_template_unsigned_int() == 1)
assert(test.test.default_template_after_parameter_pack_unsigned_int_int_int(1, 2) == 1)
