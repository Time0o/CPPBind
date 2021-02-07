#include <assert.h>

#include "test_inheritance_c.h"

int main()
{
  struct test_derived *derived = test_derived_new();
  struct test_base_1 *base1 = (struct test_base_1 *)derived;
  struct test_base_2 *base2 = (struct test_base_2 *)derived;

  assert(test_derived_func_1(derived) == 1);
  assert(test_base_1_func_1(base1) == 1);

  assert(test_derived_func_1_pure_virtual(derived) == 1);
  assert(test_base_1_func_1_pure_virtual(base1) == 1);

  assert(test_derived_func_2(derived) == 3);
  assert(test_base_2_func_2(base2) == 3);

  assert(test_derived_func_2_pure_virtual(derived) == 2);
  assert(test_base_2_func_2_pure_virtual(base2) == 2);

  test_derived_delete(derived);

  return 0;
}
