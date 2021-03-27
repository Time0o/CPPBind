#include <assert.h>

#include "test_overloaded_operators_c.h"

int main()
{
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
