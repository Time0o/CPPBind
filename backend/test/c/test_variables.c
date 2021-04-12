#include <assert.h>

#include "test_variables_c.h"

int main()
{
  assert(TEST_ENUM_1 == 1u);
  assert(TEST_ENUM_2 == 2u);

  assert(TEST_ANONYMOUS_ENUM_1 == 1u);
  assert(TEST_ANONYMOUS_ENUM_2 == 2u);

  assert(TEST_SCOPED_ENUM_SCOPED_ENUM_1 == 1u);
  assert(TEST_SCOPED_ENUM_SCOPED_ENUM_2 == 2u);

  assert(TEST_UNSIGNED_CONSTEXPR_1 == 1u);
  assert(TEST_DOUBLE_CONSTEXPR_1 == 1.0);

  assert(M_MACRO_CONST == 1);
  assert(M_MACRO_EXPR == ~(1 << 3));

  return 0;
}
