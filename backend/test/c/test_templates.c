#include <assert.h>
#include <math.h>

#include "test_templates_c.h"

int main()
{
  /* function templates */
  assert(test_add_int(1, 2) == 3);
  assert(test_add_long(1, 2) == 3);

  /* multiple template parameters */
  assert(test_add_int(1, 2) == 3);
  assert(test_mult_int_long(2, 3) == 6);
  assert(test_mult_long_int(2, 3) == 6);

  /* non-type template parameters */
  assert(test_inc_int_1(1) == 2);
  assert(test_inc_int_2(1) == 3);
  assert(test_inc_int_3(1) == 4);

  /* default template arguments */
  assert(test_pi_int() == 3);
  assert(test_pi_double() == TEST_PI);

  /* template parameter packs */
  assert(test_sum_int_int(1, 2) == 3);
  assert(test_sum_int_int_long_long(1, 2, 3, 4) == 10);

  return 0;
}
