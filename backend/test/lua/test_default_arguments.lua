require 'test_default_arguments'

assert(test.pow_default_arguments() == test.get_e())
assert(test.pow_default_arguments(10.0) == 10.0)
assert(test.pow_default_arguments(test.get_e(), 2) == test.get_e() * test.get_e())
assert(test.pow_default_arguments(test.get_e(), 2, true) == 7.0)
assert(test.pow_default_arguments(test.get_e(), 2, true, test.ROUND_UPWARD) == 8.0)

assert(test.default_large_signed() == test.LARGE_SIGNED)
assert(test.default_constexpr_function_call() == 1)
assert(test.default_function_call() == 1)
assert(test.default_ref(0) == 42);
assert(test.default_record():get_value() == 42)
assert(test.default_template_unsigned_int() == 1)
assert(test.default_template_after_parameter_pack_unsigned_int_int_int(1, 2) == 1)
