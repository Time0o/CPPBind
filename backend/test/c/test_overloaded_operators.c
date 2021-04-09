#include <assert.h>

#include "test_overloaded_operators_c.h"

int main()
{
  {
    struct test_integer i1 = test_integer_new(2);
    assert(test_integer_deref(&i1) == 2);

    struct test_integer i2 = test_integer_new(3);
    assert(test_integer_deref(&i2) == 3);

    struct test_integer i3 = test_mult(&i1, &i2);
    assert(test_integer_deref(&i3) == 6);

    struct test_integer i1_inc = test_integer_inc_pre(&i1);
    assert(test_integer_deref(&i1_inc) == 3);
    assert(test_integer_deref(&i1) == 3);

    struct test_integer i2_inc = test_integer_inc_post(&i2);
    assert(test_integer_deref(&i2_inc) == 3);
    assert(test_integer_deref(&i2) == 4);

    test_integer_delete(&i1);
    test_integer_delete(&i2);
    test_integer_delete(&i3);
    test_integer_delete(&i1_inc);
    test_integer_delete(&i2_inc);
  }

  {
    struct test_pointer pointer = test_pointer_new(1);

    struct test_pointee pointee1 = test_pointer_deref(&pointer);
    assert(test_pointee_get_state(&pointee1) == 1);

    test_pointee_set_state(&pointee1, 2);
    struct test_pointee pointee2 = test_pointer_deref(&pointer);
    assert(test_pointee_get_state(&pointee2) == 2);

    test_pointer_delete(&pointer);
    test_pointee_delete(&pointee1);
    test_pointee_delete(&pointee2);
  }

  {
    struct test_class_with_callable_member class =
      test_class_with_callable_member_new();

    assert(test_class_with_callable_member_sum(&class, 1, 2) == 3);

    test_class_with_callable_member_delete(&class);
  }

  return 0;
}
