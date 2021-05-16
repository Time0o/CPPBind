#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <string.h>

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
  assert(fabs(test_add_double(0.1, 0.2) - 0.3) < test_get_epsilon_double());

  /* boolean parameters */
  assert(test_not_bool(1) == 0);
  assert(test_not_bool(0) == 1);

  /* enum parameters */
  assert(test_not_bool_enum(TEST_FALSE) == TEST_TRUE);
  assert(test_not_bool_enum(TEST_TRUE) == TEST_FALSE);

  assert(test_not_bool_scoped_enum(TEST_BOOLEAN_SCOPED_FALSE) == TEST_BOOLEAN_SCOPED_TRUE);
  assert(test_not_bool_scoped_enum(TEST_BOOLEAN_SCOPED_TRUE) == TEST_BOOLEAN_SCOPED_FALSE);

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
    int *iptr2 = &i2;

    assert(test_add_pointer_to_pointer(&iptr1, &iptr2) == &iptr1 && i1 == 3);
  }

  /* lvalue reference parameters */
  {
    int i1 = 1;
    int const i2 = 2;

    assert(test_add_lvalue_ref(&i1, &i2) == &i1 && i1 == 3);
  }

  /* rvalue reference parameters */
  {
    int i1 = 1;
    int const i2 = 2;
    assert(test_add_rvalue_ref(&i1, &i2) == 3);
  }

  /* no parameters */
  assert(test_one_no_parameters() == 1);

  /* unused parameters */
  assert(test_add_unused_parameters(1, -1, 2, -1) == 3);

  /* string parameters */
  assert(strcmp(test_min_string_parameters("abdd", "abcd"), "abcd") == 0);

  return 0;
}
