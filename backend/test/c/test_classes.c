#include <assert.h>
#include <stddef.h>

#include "test_classes_c.h"

struct test_non_constructible *test_non_constructible_new()
{ return NULL; }

void test_a_class_set_state_private(int state)
{ }

int main()
{
  /* construction and deletion */
  {
    struct test_trivial *trivial = test_trivial_new();
    _delete(trivial);
  }

  /* member access */
  {
    struct test_a_class *a_class = test_a_class_new_1();

    assert(test_a_class_get_state(a_class) == 0);

    _delete(a_class);
    assert(test_a_class_get_num_destroyed() == 1);
  }

  {
    struct test_a_class *a_class = test_a_class_new_2(1);

    assert(test_a_class_get_state(a_class) == 1);

    test_a_class_set_state(a_class, 2);
    assert(test_a_class_get_state(a_class) == 2);

    _delete(a_class);
    assert(test_a_class_get_num_destroyed() == 2);
  }

  test_a_class_set_static_state(1);
  assert(test_a_class_get_static_state() == 1);

  /* copying and moving */
  {
    void *copyable_class = test_copyable_class_new(1);
    assert(test_copyable_class_get_state(copyable_class) == 1);

    assert(test_copyable_class_get_num_copied() == 0);

    void *copyable_class_copy = _copy(copyable_class);
    assert(test_copyable_class_get_state(copyable_class_copy) == 1);

    test_copyable_class_set_state(copyable_class_copy, 2);
    assert(test_copyable_class_get_state(copyable_class) == 1);
    assert(test_copyable_class_get_state(copyable_class_copy) == 2);

    _delete(copyable_class);
    _delete(copyable_class_copy);

    assert(test_copyable_class_get_num_copied() == 1);
  }

  {
    void *moveable_class = test_moveable_class_new(1);

    assert(test_moveable_class_get_state(moveable_class) == 1);

    assert(test_moveable_class_get_num_moved() == 0);

    void *moveable_class_moved = _move(moveable_class);
    assert(test_moveable_class_get_state(moveable_class_moved) == 1);

    _delete(moveable_class);
    _delete(moveable_class_moved);

    assert(test_moveable_class_get_num_moved() == 1);
  }

  return 0;
}
