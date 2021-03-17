#include <assert.h>
#include <limits.h>

#include "test_default_arguments_c.h"

int main()
{
  assert(test_pow_default_arguments(TEST_E, 2, 1, TEST_ROUND_UPWARD) == 8.0);

  assert(test_default_large_signed(LLONG_MAX) == LLONG_MAX);
  assert(test_default_large_unsigned(UINT_MAX) == UINT_MAX);
  assert(test_default_constexpr_function_call(1) == 1);
  assert(test_default_function_call(1) == 1);
  assert(test_default_template_unsigned_int(1u) == 1u);
  assert(test_default_template_after_parameter_pack_unsigned_int_int_int(1, 2, 3u) == 3u);

  return 0;
}
