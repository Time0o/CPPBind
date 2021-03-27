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

  /* record templates */
  {
    struct test_any_stack_int any_stack = test_any_stack_int_new();

    test_any_stack_int_push_int(&any_stack, 1);
    test_any_stack_int_push_double(&any_stack, 3.14);

    assert(test_any_stack_int_pop_double(&any_stack) == 3.14);
    assert(test_any_stack_int_pop_int(&any_stack) == 1);

    assert(test_any_stack_int_empty(&any_stack));

    test_any_stack_int_delete(&any_stack);
  }

  return 0;
}
