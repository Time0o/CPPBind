require 'test_parameters'

-- integer parameters
assert(test.add_signed_int(test.MIN_SIGNED_INT + 1, -1)
       == test.MIN_SIGNED_INT)

assert(test.add_signed_int(test.MAX_SIGNED_INT - 1, 1)
       == test.MAX_SIGNED_INT)

assert(test.add_unsigned_int(test.MAX_UNSIGNED_INT - 1, 1)
       == test.MAX_UNSIGNED_INT)

assert(test.add_large_signed_int(test.MIN_LARGE_SIGNED_INT + 1, -1)
       == test.MIN_LARGE_SIGNED_INT)

assert(test.add_large_signed_int(test.MAX_LARGE_SIGNED_INT - 1, 1)
       == test.MAX_LARGE_SIGNED_INT)

-- floating point parameters
assert(math.abs(test.add_double(0.1, 0.2) - 0.3) < test.get_epsilon_double())

-- boolean parameters
assert(test.not_bool(false) == true)
assert(test.not_bool(true) == false)

-- enum parameters
assert(test.not_bool_enum(test.FALSE) == test.TRUE)
assert(test.not_bool_enum(test.TRUE) == test.FALSE)

assert(test.not_bool_scoped_enum(test.BOOLEAN_SCOPED_FALSE) == test.BOOLEAN_SCOPED_TRUE)
assert(test.not_bool_scoped_enum(test.BOOLEAN_SCOPED_TRUE) == test.BOOLEAN_SCOPED_FALSE)

-- pointer parameters
do
  local iptr1 = test.util.int_new(1):own()
  local iptr2 = test.util.int_new(2):own()

  local iptr_res, _ = test.add_pointer(iptr1, iptr2)

  assert(_ == nil)

  assert(test.util.int_deref(iptr_res) == 3)
  assert(test.util.int_deref(iptr1) == 3)
end

do
  local i1 = 1
  local i2 = 2

  local _, i1_res = test.add_pointer(i1, i2)

  assert(i1_res == 3)
end

do
  local iptr1 = test.util.int_new(1):own()
  local iptr2 = test.util.int_new(2):own()

  local iptr_ptr1 = test.util.int_ptr_new(iptr1):own()
  local iptr_ptr2 = test.util.int_ptr_new(iptr2):own()

  local iptr_ptr_res = test.add_pointer_to_pointer(iptr_ptr1, iptr_ptr2)
  local iptr_res = test.util.int_ptr_deref(iptr_ptr_res)
  assert(test.util.int_deref(iptr1) == 3)
  assert(test.util.int_deref(iptr_res) == 3)
end

-- lvalue reference parameters
do
  local i1 = 1
  local i2 = 2

  res, i1_res = test.add_lvalue_ref(i1, i2)
  assert(i1_res == 3)
  assert(res == 3)
end

-- rvalue reference parameters
do
  local i1 = 1
  local i2 = 2

  assert(test.add_rvalue_ref(i1, i2))
end

-- no parameters
assert(test.one_no_parameters() == 1)

-- unused parameters
assert(test.add_unused_parameters(1, -1, 2, -1) == 3)

-- string parameters
assert(test.min_string_parameters("abdd", "abcd") == "abcd")
