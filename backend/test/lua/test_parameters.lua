local test = require 'test_parameters'

-- integer parameters
assert(test.test_add_signed_int(test.TEST_MIN_SIGNED_INT + 1, -1)
       == test.TEST_MIN_SIGNED_INT)

assert(test.test_add_signed_int(test.TEST_MAX_SIGNED_INT - 1, 1)
       == test.TEST_MAX_SIGNED_INT)

assert(test.test_add_unsigned_int(test.TEST_MAX_UNSIGNED_INT - 1, 1)
       == test.TEST_MAX_UNSIGNED_INT)

assert(test.test_add_large_signed_int(test.TEST_MIN_LARGE_SIGNED_INT + 1, -1)
       == test.TEST_MIN_LARGE_SIGNED_INT)

assert(test.test_add_large_signed_int(test.TEST_MAX_LARGE_SIGNED_INT - 1, 1)
       == test.TEST_MAX_LARGE_SIGNED_INT)

-- floating point parameters
assert(math.abs(test.test_add_double(0.1, 0.2) - 0.3) < test.TEST_EPSILON_DOUBLE)

-- boolean parameters
assert(test.test_not_bool(false) == true)
assert(test.test_not_bool(true) == false)

-- enum parameters
assert(test.test_not_bool_enum(test.TEST_BOOLEAN_TRUE) == test.TEST_BOOLEAN_FALSE)
assert(test.test_not_bool_enum(test.TEST_BOOLEAN_FALSE) == test.TEST_BOOLEAN_TRUE)

-- pointer parameters
do
  local iptr1 = test.test_util_int_new(1)
  local iptr2 = test.test_util_int_new(2)

  local iptr_res = test.test_add_pointer(iptr1, iptr2)
  assert(test.test_util_int_deref(iptr1) == 3)
  assert(test.test_util_int_deref(iptr_res) == 3)

  iptr1:_own()
  iptr2:_own()
end

do
  local bptr_true = test.test_util_bool_enum_new(test.TEST_BOOLEAN_TRUE)
  local bptr_false = test.test_util_bool_enum_new(test.TEST_BOOLEAN_FALSE)

  local bptr_not_true = test.test_not_bool_enum_pointer(bptr_true)
  assert(test.test_util_bool_enum_deref(bptr_true) == test.TEST_BOOLEAN_FALSE)
  assert(test.test_util_bool_enum_deref(bptr_not_true) == test.TEST_BOOLEAN_FALSE)

  local bptr_not_false = test.test_not_bool_enum_pointer(bptr_false)
  assert(test.test_util_bool_enum_deref(bptr_false) == test.TEST_BOOLEAN_TRUE)
  assert(test.test_util_bool_enum_deref(bptr_not_false) == test.TEST_BOOLEAN_TRUE)

  bptr_true:_own()
  bptr_false:_own()
end

-- lvalue reference parameters
do
  local i1 = 1
  local i2 = 2

  assert(test.test_add_lvalue_ref(i1, i2) == 3)
end

do
  local b_true = test.TEST_BOOLEAN_TRUE
  local b_false = test.TEST_BOOLEAN_FALSE

  assert(test.test_not_bool_enum_lvalue_ref(b_true) == test.TEST_BOOLEAN_FALSE)
  assert(test.test_not_bool_enum_lvalue_ref(b_false) == test.TEST_BOOLEAN_TRUE)
end

-- rvalue reference parameters
assert(test.test_add_rvalue_ref(1, 2) == 3)

assert(test.test_not_bool_enum_rvalue_ref(test.TEST_BOOLEAN_TRUE)
       == test.TEST_BOOLEAN_FALSE)

assert(test.test_not_bool_enum_rvalue_ref(test.TEST_BOOLEAN_FALSE)
       == test.TEST_BOOLEAN_TRUE)

-- unused parameters
assert(test.test_add_unused_parameters(1, -1, 2, -1) == 3)
