#include <assert.h>

#include "test_constants_c.h"

int main()
{
  {
    enum test_simple_enum e;

    e = TEST_ENUM_1;
    assert(e == 1u);

    e = TEST_ENUM_2;
    assert(e == 2u);
  }

  {
    enum test_scoped_enum e;

    e = TEST_SCOPED_ENUM_SCOPED_ENUM_1;
    assert(e == 1u);

    e = TEST_SCOPED_ENUM_SCOPED_ENUM_2;
    assert(e == 2u);
  }

  assert(TEST_ANONYMOUS_ENUM_1 == 1u);
  assert(TEST_ANONYMOUS_ENUM_2 == 2u);

  assert(TEST_DUPLICATE_ENUM_1 == 1u);
  assert(TEST_DUPLICATE_ENUM_2 == 1u);

  assert(TEST_ENUM_IN_ANONYMOUS_NAMESPACE_1 == 1u);
  assert(TEST_ENUM_IN_ANONYMOUS_NAMESPACE_2 == 2u);

  assert(get_macro_const() == 1);
  assert(get_macro_expr() == ~(1 << 3));

  return 0;
}
