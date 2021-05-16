#include <assert.h>
#include <limits.h>

#include "test_default_arguments_c.h"

int main()
{
  assert(test_pow_default_arguments(test_get_e(), 2, 1, TEST_ROUND_UPWARD) == 8.0);

  assert(test_default_large_signed(LLONG_MAX) == LLONG_MAX);
  assert(test_default_large_unsigned(UINT_MAX) == UINT_MAX);
  assert(test_default_constexpr_function_call(test_func_constexpr()) == test_func_constexpr());
  assert(test_default_function_call(test_func()) == test_func());

  int a;
  test_default_ref(&a, 42);
  assert(a == 42);

  {
    struct test_a_class a_class = test_a_class_new(42);
    struct test_a_class a_class_copy = test_default_record(&a_class);
    assert(test_a_class_get_value(&a_class_copy) == 42);

    test_a_class_delete(&a_class);
    test_a_class_delete(&a_class_copy);
  }

  assert(test_default_template_unsigned_int(1u) == 1u);
  assert(test_default_template_after_parameter_pack_unsigned_int_int_int(1, 2, 3u) == 3u);

  return 0;
}
