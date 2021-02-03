#include <assert.h>
#include <stddef.h>

#include "test_classes_c.h"

struct test_non_constructible *test_non_constructible_new()
{ return NULL; }

void test_a_class_set_state_private(int state)
{ }

int main()
{
  {
    struct test_trivial *trivial = test_trivial_new();
    test_trivial_delete(trivial);
  }

  {
    struct test_a_class *a_class = test_a_class_new_1();

    assert(test_a_class_get_state(a_class) == 0);

    test_a_class_delete(a_class);
    assert(test_a_class_get_num_destroyed() == 1);
  }

  {
    struct test_a_class *a_class = test_a_class_new_2(1);

    assert(test_a_class_get_state(a_class) == 1);

    test_a_class_set_state(a_class, 2);
    assert(test_a_class_get_state(a_class) == 2);

    test_a_class_delete(a_class);
    assert(test_a_class_get_num_destroyed() == 2);
  }

  test_a_class_set_static_state(1);
  assert(test_a_class_get_static_state() == 1);

  return 0;
}
