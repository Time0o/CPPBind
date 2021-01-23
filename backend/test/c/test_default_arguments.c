#include <assert.h>

#include "test_default_arguments_c.h"

int main()
{
  assert(test_pow_default_arguments(TEST_E, 2, true, TEST_ROUND_UPWARD) == 8.0);

  return 0;
}
