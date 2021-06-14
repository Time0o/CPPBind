#include <assert.h>

#include "test_polymorphism_c.h"

int main()
{
  struct test_derived_1 derived_1 = test_derived_1_new();
  struct test_derived_2 derived_2 = test_derived_2_new();

  {
    struct test_base_1 base_1_arr[2];
    base_1_arr[0] = test_derived_1_cast_to_const_base_1(&derived_1);
    base_1_arr[1] = test_derived_2_cast_to_const_base_1(&derived_2);

    assert(test_base_1_func_1(&base_1_arr[0]) == 1);
    assert(test_base_1_func_1(&base_1_arr[1]) == 2);
  }

  {
    struct test_base_2 base_2_arr[2];
    base_2_arr[0] = test_derived_1_cast_to_const_base_2(&derived_1);
    base_2_arr[1] = test_derived_2_cast_to_const_base_2(&derived_2);

    assert(test_base_2_func_2(&base_2_arr[0]) == -1);
    assert(test_base_2_func_2(&base_2_arr[1]) == -2);
  }

  test_derived_1_delete(&derived_1);
  test_derived_2_delete(&derived_2);

  return 0;
}
