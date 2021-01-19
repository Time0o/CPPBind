#include <assert.h>

#include "test_variables_c.h"

int main()
{
  assert(TEST_ENUM_1 == 1);
  assert(TEST_ENUM_2 == 2);

  assert(TEST_ANONYMOUS_ENUM_1 == 1);
  assert(TEST_ANONYMOUS_ENUM_2 == 2);

  assert(TEST_SCOPED_ENUM_SCOPED_ENUM_1 == 1);
  assert(TEST_SCOPED_ENUM_SCOPED_ENUM_2 == 2);

  assert(TEST_CONSTEXPR_1 == 1);
  assert(TEST_STATIC_CONST_1 == 1);
}
