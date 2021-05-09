local test = require 'test.parameters'

-- integer parameters
assert(test.test.add_signed_int(test.test.MIN_SIGNED_INT + 1, -1)
       == test.test.MIN_SIGNED_INT)

assert(test.test.add_signed_int(test.test.MAX_SIGNED_INT - 1, 1)
       == test.test.MAX_SIGNED_INT)

assert(test.test.add_unsigned_int(test.test.MAX_UNSIGNED_INT - 1, 1)
       == test.test.MAX_UNSIGNED_INT)

assert(test.test.add_large_signed_int(test.test.MIN_LARGE_SIGNED_INT + 1, -1)
       == test.test.MIN_LARGE_SIGNED_INT)

assert(test.test.add_large_signed_int(test.test.MAX_LARGE_SIGNED_INT - 1, 1)
       == test.test.MAX_LARGE_SIGNED_INT)

-- floating point parameters
assert(math.abs(test.test.add_double(0.1, 0.2) - 0.3) < test.test.EPSILON_DOUBLE)

-- boolean parameters
assert(test.test.not_bool(false) == true)
assert(test.test.not_bool(true) == false)

-- enum parameters
assert(test.test.not_bool_enum(test.test.FALSE) == test.test.TRUE)
assert(test.test.not_bool_enum(test.test.TRUE) == test.test.FALSE)

assert(test.test.not_bool_scoped_enum(test.test.BOOLEAN_SCOPED_FALSE) == test.test.BOOLEAN_SCOPED_TRUE)
assert(test.test.not_bool_scoped_enum(test.test.BOOLEAN_SCOPED_TRUE) == test.test.BOOLEAN_SCOPED_FALSE)

-- pointer parameters
do
  local iptr1 = test.test.util.int_new(1):own()
  local iptr2 = test.test.util.int_new(2):own()

  local iptr_res, iptr1_res = test.test.add_pointer(iptr1, iptr2)
  assert(iptr1_res == 3)
  assert(test.test.util.int_deref(iptr1) == 3)
  assert(test.test.util.int_deref(iptr_res) == 3)
end

do
  local iptr1 = test.test.util.int_new(1):own()
  local iptr2 = test.test.util.int_new(2):own()

  local iptr_ptr1 = test.test.util.int_ptr_new(iptr1):own()
  local iptr_ptr2 = test.test.util.int_ptr_new(iptr2):own()

  local iptr_ptr_res = test.test.add_pointer_to_pointer(iptr_ptr1, iptr_ptr2)
  local iptr_res = test.test.util.int_ptr_deref(iptr_ptr_res)
  assert(test.test.util.int_deref(iptr1) == 3)
  assert(test.test.util.int_deref(iptr_res) == 3)
end

-- lvalue reference parameters
do
  local i1 = 1
  local i2 = 2

  res, i1_res = test.test.add_lvalue_ref(i1, i2)
  assert(i1_res == 3)
  assert(res == 3)
end

-- no parameters
assert(test.test.one_no_parameters() == 1)

-- unused parameters
assert(test.test.add_unused_parameters(1, -1, 2, -1) == 3)

-- string parameters
assert(test.test.min_string_parameters("abdd", "abcd") == "abcd")
