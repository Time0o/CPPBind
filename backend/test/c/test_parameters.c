#include <assert.h>
#include <math.h>
#include <stddef.h>

#include "test_parameters_c.h"

int main()
{
  /* integer parameters */
  assert(test_add_signed_int(TEST_MIN_SIGNED_INT + 1, -1)
         == TEST_MIN_SIGNED_INT);

  assert(test_add_signed_int(TEST_MAX_SIGNED_INT - 1, 1)
         == TEST_MAX_SIGNED_INT);

  assert(test_add_unsigned_int(TEST_MAX_UNSIGNED_INT - 1u, 1u)
         == TEST_MAX_UNSIGNED_INT);

  assert(test_add_large_signed_int(TEST_MIN_LARGE_SIGNED_INT + 1, -1)
         == TEST_MIN_LARGE_SIGNED_INT);

  assert(test_add_large_signed_int(TEST_MAX_LARGE_SIGNED_INT - 1, 1)
         == TEST_MAX_LARGE_SIGNED_INT);

  /* floating point parameters */
  assert(fabs(test_add_double(0.1, 0.2) - 0.3) < TEST_EPSILON_DOUBLE);

  /* boolean parameters */
  assert(test_not_bool(true) == false);
  assert(test_not_bool(false) == true);

  /* enum parameters */
  assert(test_not_bool_enum(TEST_BOOLEAN_TRUE) == TEST_BOOLEAN_FALSE);
  assert(test_not_bool_enum(TEST_BOOLEAN_FALSE) == TEST_BOOLEAN_TRUE);

  /* pointer parameters */
  {
    int i1 = 1;
    int const i2 = 2;

    assert(test_add_pointer(&i1, &i2) == &i1 && i1 == 3);
  }

  {
    int i1 = 1;
    int *iptr1 = &i1;
    int i2 = 2;
    int * const iptr2 = &i2;

    assert(test_add_pointer_to_pointer(&iptr1, &iptr2) == &iptr1 && i1 == 3);
  }

  {
    void *bptr_true = bind_own(test_util_bool_enum_new(TEST_BOOLEAN_TRUE));

    void *bptr_not_true = test_not_bool_enum_pointer(bptr_true);
    assert(test_util_bool_enum_deref(bptr_true) == TEST_BOOLEAN_FALSE);
    assert(test_util_bool_enum_deref(bptr_not_true) == TEST_BOOLEAN_FALSE);

    bind_delete(bptr_true);
    bind_delete(bptr_not_true);
  }

  {
    void *bptr_true = bind_own(test_util_bool_enum_new(TEST_BOOLEAN_TRUE));
    void *bptr_res = bind_own(test_util_bool_enum_new(TEST_BOOLEAN_TRUE));

    void *bptr_ret = test_not_bool_enum_pointer_to_const(bptr_true, bptr_res);
    assert(test_util_bool_enum_deref(bptr_res) == TEST_BOOLEAN_FALSE);
    assert(test_util_bool_enum_deref(bptr_ret) == TEST_BOOLEAN_TRUE);

    bind_delete(bptr_true);
    bind_delete(bptr_res);
    bind_delete(bptr_ret);
  }

  {
    void *bptr_true = bind_own(test_util_bool_enum_new(TEST_BOOLEAN_TRUE));
    void *bptr_ptr_true = bind_own(test_util_bool_enum_ptr_new(bptr_true));

    void *bptr_ptr_not_true = test_not_bool_enum_pointer_to_pointer(bptr_ptr_true);
    void *bptr_not_true = test_util_bool_enum_ptr_deref(bptr_ptr_not_true);
    assert(test_util_bool_enum_deref(bptr_true) == TEST_BOOLEAN_FALSE);
    assert(test_util_bool_enum_deref(bptr_not_true) == TEST_BOOLEAN_FALSE);

    bind_delete(bptr_true);
    bind_delete(bptr_ptr_true);
    bind_delete(bptr_ptr_not_true);
    bind_delete(bptr_not_true);
  }

  /* lvalue reference parameters */
  {
    int i1 = 1;
    int const i2 = 2;

    assert(test_add_lvalue_ref(&i1, &i2) == &i1 && i1 == 3);
  }

  {
    void *bptr_true = bind_own(test_util_bool_enum_new(TEST_BOOLEAN_TRUE));

    void *bptr_not_true = test_not_bool_enum_lvalue_ref(bptr_true);
    assert(test_util_bool_enum_deref(bptr_true) == TEST_BOOLEAN_FALSE);
    assert(test_util_bool_enum_deref(bptr_not_true) == TEST_BOOLEAN_FALSE);

    bind_delete(bptr_true);
    bind_delete(bptr_not_true);
  }

  {
    void *bptr_true = bind_own(test_util_bool_enum_new(TEST_BOOLEAN_TRUE));

    assert(test_not_bool_enum_lvalue_ref_to_const(bptr_true) == TEST_BOOLEAN_FALSE);

    bind_delete(bptr_true);
  }

  {
    void *bptr_true = test_util_bool_enum_new(TEST_BOOLEAN_TRUE);
    void *bptr_ptr_true = bind_own(test_util_bool_enum_ptr_new(bptr_true));

    void *bptr_not_true = bind_own(test_not_bool_enum_lvalue_ref_to_pointer(bptr_ptr_true));
    assert(test_util_bool_enum_deref(bptr_not_true) == TEST_BOOLEAN_FALSE);

    bind_delete(bptr_true);
    bind_delete(bptr_ptr_true);
    bind_delete(bptr_not_true);
  }

  /* rvalue reference parameters */
  {
    int i1 = 1;
    int const i2 = 2;
    assert(test_add_rvalue_ref(&i1, &i2) == 3);
  }

  {
    void *b_true = bind_own(test_util_bool_enum_new(TEST_BOOLEAN_TRUE));

    assert(test_not_bool_enum_rvalue_ref(b_true) == TEST_BOOLEAN_FALSE);

    bind_delete(b_true);
  }

  /* unused parameters */
  assert(test_add_unused_parameters(1, -1, 2, -1) == 3);

  return 0;
}
