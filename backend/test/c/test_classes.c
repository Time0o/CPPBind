#include <assert.h>
#include <stddef.h>

#include "test_classes_c.h"

void *test_non_constructible_new()
{ return NULL; }

void test_a_class_set_state_private(int state)
{ }

int main()
{
  /* construction and deletion */
  {
    void *trivial = test_trivial_new();
    bind_delete(trivial);
  }

  /* member access */
  {
    void *a_class = test_a_class_new_1();

    assert(test_a_class_get_state(a_class) == 0);

    bind_delete(a_class);
    assert(test_a_class_get_num_destroyed() == 1);
  }

  {
    void *a_class = test_a_class_new_2(1);

    assert(test_a_class_get_state(a_class) == 1);

    test_a_class_set_state(a_class, 2);
    assert(test_a_class_get_state(a_class) == 2);

    bind_delete(a_class);
    assert(test_a_class_get_num_destroyed() == 2);
  }

  test_a_class_set_static_state(1);
  assert(test_a_class_get_static_state() == 1);

  /* copying and moving */
  {
    void *copyable_class = test_copyable_class_new(1);
    assert(test_copyable_class_get_state(copyable_class) == 1);

    assert(test_copyable_class_get_num_copied() == 0);

    void *copyable_class_copy = bind_copy(copyable_class);
    assert(test_copyable_class_get_state(copyable_class_copy) == 1);

    test_copyable_class_set_state(copyable_class_copy, 2);
    assert(test_copyable_class_get_state(copyable_class) == 1);
    assert(test_copyable_class_get_state(copyable_class_copy) == 2);

    bind_delete(copyable_class);
    bind_delete(copyable_class_copy);

    assert(test_copyable_class_get_num_copied() == 1);
  }

  {
    void *moveable_class = test_moveable_class_new(1);

    assert(test_moveable_class_get_state(moveable_class) == 1);

    assert(test_moveable_class_get_num_moved() == 0);

    void *moveable_class_moved = bind_move(moveable_class);
    assert(test_moveable_class_get_state(moveable_class_moved) == 1);

    bind_delete(moveable_class);
    bind_delete(moveable_class_moved);

    assert(test_moveable_class_get_num_moved() == 1);
  }

  /* class parameters */

  /* value parameters */
  {
    void *a = test_class_parameter_new(1);
    void *b = test_class_parameter_new(2);

    test_class_parameter_set_copyable(true);

    void *c = test_add_class(a, b);
    assert(test_class_parameter_get_state(a) == 1);
    assert(test_class_parameter_get_state(c) == 3);

    test_class_parameter_set_copyable(false);

    bind_delete(a);
    bind_delete(b);
    bind_delete(c);
  }

  /* pointer parameters */
  {
    void *a = test_class_parameter_new(1);
    void *b = test_class_parameter_new(2);

    void *c = test_add_class_pointer(a, b);
    assert(test_class_parameter_get_state(a) == 3);
    assert(test_class_parameter_get_state(c) == 3);

    bind_delete(a);
    bind_delete(b);
    bind_delete(c);
  }

  /* lvalue reference parameters */
  {
    void *a = test_class_parameter_new(1);
    void *b = test_class_parameter_new(2);

    void *c = test_add_class_lvalue_ref(a, b);
    assert(test_class_parameter_get_state(a) == 3);
    assert(test_class_parameter_get_state(c) == 3);

    bind_delete(a);
    bind_delete(b);
    bind_delete(c);
  }

  /* rvalue reference parameters */
  {
    void *a = test_class_parameter_new(1);

    test_class_parameter_set_moveable(true);

    test_noop_class_rvalue_ref(a);
    assert(test_class_parameter_was_moved(a));

    test_class_parameter_set_moveable(false);

    bind_delete(a);
  }

  return 0;
}
