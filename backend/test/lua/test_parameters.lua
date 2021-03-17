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
  local iptr1 = test.test_util_int_new(1):own()
  local iptr2 = test.test_util_int_new(2):own()

  local iptr_res, iptr1_res = test.test_add_pointer(iptr1, iptr2)
  assert(iptr1_res == 3)
  assert(test.test_util_int_deref(iptr1) == 3)
  assert(test.test_util_int_deref(iptr_res) == 3)
end

do
  local iptr1 = test.test_util_int_new(1):own()
  local iptr2 = test.test_util_int_new(2):own()

  local iptr_ptr1 = test.test_util_int_ptr_new(iptr1):own()
  local iptr_ptr2 = test.test_util_int_ptr_new(iptr2):own()

  local iptr_ptr_res = test.test_add_pointer_to_pointer(iptr_ptr1, iptr_ptr2)
  local iptr_res = test.test_util_int_ptr_deref(iptr_ptr_res)
  assert(test.test_util_int_deref(iptr1) == 3)
  assert(test.test_util_int_deref(iptr_res) == 3)
end

do
  local bptr_true = test.test_util_bool_enum_new(test.TEST_BOOLEAN_TRUE):own()

  local bptr_not_true, bptr_true_res = test.test_not_bool_enum_pointer(bptr_true)
  assert(bptr_true_res == test.TEST_BOOLEAN_FALSE)
  assert(test.test_util_bool_enum_deref(bptr_true) == test.TEST_BOOLEAN_FALSE)
  assert(test.test_util_bool_enum_deref(bptr_not_true) == test.TEST_BOOLEAN_FALSE)
end

do
  local bptr_true = test.test_util_bool_enum_new(test.TEST_BOOLEAN_TRUE):own()
  local bptr_res = test.test_util_bool_enum_new(test.TEST_BOOLEAN_TRUE):own()

  local bptr_ret = test.test_not_bool_enum_pointer_to_const(bptr_true, bptr_res)
  assert(test.test_util_bool_enum_deref(bptr_res) == test.TEST_BOOLEAN_FALSE)
  assert(test.test_util_bool_enum_deref(bptr_ret) == test.TEST_BOOLEAN_TRUE)
end

do
  local bptr_true = test.test_util_bool_enum_new(test.TEST_BOOLEAN_TRUE):own()
  local bptr_ptr_true = test.test_util_bool_enum_ptr_new(bptr_true):own()

  local bptr_ptr_not_true = test.test_not_bool_enum_pointer_to_pointer(bptr_ptr_true)
  local bptr_not_true = test.test_util_bool_enum_ptr_deref(bptr_ptr_not_true)
  assert(test.test_util_bool_enum_deref(bptr_true) == test.TEST_BOOLEAN_FALSE)
  assert(test.test_util_bool_enum_deref(bptr_not_true) == test.TEST_BOOLEAN_FALSE)
end

-- lvalue reference parameters
do
  local i1 = 1
  local i2 = 2

  res, i1_res = test.test_add_lvalue_ref(i1, i2)
  assert(i1_res == 3)
  assert(res == 3)
end

do
  local b_true = test.TEST_BOOLEAN_TRUE

  res, b_true_res = test.test_not_bool_enum_lvalue_ref(b_true)
  assert(b_true_res == test.TEST_BOOLEAN_FALSE)
  assert(res == test.TEST_BOOLEAN_FALSE)
end

do
  local b_true = test.TEST_BOOLEAN_TRUE

  assert(test.test_not_bool_enum_lvalue_ref_to_const(b_true) == test.TEST_BOOLEAN_FALSE)
end

-- XXX reference to pointer

-- rvalue reference parameters
assert(test.test_add_rvalue_ref(1, 2) == 3)

assert(test.test_not_bool_enum_rvalue_ref(test.TEST_BOOLEAN_TRUE)
       == test.TEST_BOOLEAN_FALSE)

assert(test.test_not_bool_enum_rvalue_ref(test.TEST_BOOLEAN_FALSE)
       == test.TEST_BOOLEAN_TRUE)

-- no parameters
assert(test.test_one_no_parameters() == 1)

-- unused parameters
assert(test.test_add_unused_parameters(1, -1, 2, -1) == 3)

-- string parameters
assert(test.test_min_string_parameters("abdd", "abcd") == "abcd")
