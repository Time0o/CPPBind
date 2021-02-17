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
    void *b_true = test_util_bool_enum_new(TEST_BOOLEAN_TRUE);
    void *b_false = test_util_bool_enum_new(TEST_BOOLEAN_FALSE);

    void *b_not_true = test_not_bool_enum_pointer(b_true);
    assert(test_util_bool_enum_deref(b_true) == TEST_BOOLEAN_FALSE);
    assert(test_util_bool_enum_deref(b_not_true) == TEST_BOOLEAN_FALSE);

    void *b_not_false = test_not_bool_enum_pointer(b_false);
    assert(test_util_bool_enum_deref(b_false) == TEST_BOOLEAN_TRUE);
    assert(test_util_bool_enum_deref(b_not_false) == TEST_BOOLEAN_TRUE);

    _own(b_true);
    _own(b_false);
    _delete(b_true);
    _delete(b_false);
    _delete(b_not_true);
    _delete(b_not_false);
  }

  /* lvalue reference parameters */
  {
    int i1 = 1;
    int const i2 = 2;

    assert(test_add_lvalue_ref(&i1, &i2) == &i1 && i1 == 3);
  }

  {
    void *b_true = test_util_bool_enum_new(TEST_BOOLEAN_TRUE);
    void *b_false = test_util_bool_enum_new(TEST_BOOLEAN_FALSE);

    void *b_not_true = test_not_bool_enum_lvalue_ref(b_true);
    assert(test_util_bool_enum_deref(b_true) == TEST_BOOLEAN_FALSE);
    assert(test_util_bool_enum_deref(b_not_true) == TEST_BOOLEAN_FALSE);

    void *b_not_false = test_not_bool_enum_lvalue_ref(b_false);
    assert(test_util_bool_enum_deref(b_false) == TEST_BOOLEAN_TRUE);
    assert(test_util_bool_enum_deref(b_not_false) == TEST_BOOLEAN_TRUE);

    _own(b_true);
    _own(b_false);
    _delete(b_true);
    _delete(b_false);
    _delete(b_not_true);
    _delete(b_not_false);
  }

  /* rvalue reference parameters */
  {
    int i1 = 1;
    int const i2 = 2;
    assert(test_add_rvalue_ref(&i1, &i2) == 3);
  }

  {
    void *b_true = test_util_bool_enum_new(TEST_BOOLEAN_TRUE);
    void *b_false = test_util_bool_enum_new(TEST_BOOLEAN_FALSE);

    assert(test_not_bool_enum_rvalue_ref(b_true) == TEST_BOOLEAN_FALSE);
    assert(test_not_bool_enum_rvalue_ref(b_false) == TEST_BOOLEAN_TRUE);

    _own(b_true);
    _own(b_false);
    _delete(b_true);
    _delete(b_false);
  }

  /* unused parameters */
  assert(test_add_unused_parameters(1, -1, 2, -1) == 3);

  return 0;
}
