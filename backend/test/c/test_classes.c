#include <assert.h>
#include <stddef.h>

#include "test_classes_c.h"

void test_non_constructible_new() {}
void test_non_constructible_delete() {}

void test_implicitly_non_constructible_new() {}
void test_implicitly_non_constructible_delete() {}

void test_a_class_set_state_private() {}

int main()
{
  /* construction and deletion */
  {
    struct test_trivial trivial = test_trivial_new();
    test_trivial_delete(&trivial);
  }

  /* member access */
  {
    struct test_a_class a_class = test_a_class_new_1();

    assert(test_a_class_get_state(&a_class) == 0);

    test_a_class_delete(&a_class);
    assert(test_a_class_get_num_destroyed() == 1);
  }

  {
    struct test_a_class a_class = test_a_class_new_2(1);

    assert(test_a_class_get_state(&a_class) == 1);

    test_a_class_set_state(&a_class, 2);
    assert(test_a_class_get_state(&a_class) == 2);

    test_a_class_delete(&a_class);
    assert(test_a_class_get_num_destroyed() == 2);
  }

  test_a_class_set_static_state(1);
  assert(test_a_class_get_static_state() == 1);

  /* copying and moving */
  {
    struct test_copyable_class copyable_class = test_copyable_class_new(1);
    assert(test_copyable_class_get_state(&copyable_class) == 1);

    assert(test_copyable_class_get_num_copied() == 0);

    struct test_copyable_class copyable_class_copy =
      test_copyable_class_copy(&copyable_class);
    assert(test_copyable_class_get_state(&copyable_class_copy) == 1);

    assert(test_copyable_class_get_num_copied() == 1);

    test_copyable_class_set_state(&copyable_class_copy, 2);
    assert(test_copyable_class_get_state(&copyable_class) == 1);
    assert(test_copyable_class_get_state(&copyable_class_copy) == 2);

    struct test_copyable_class copyable_class_copy_assigned =
      test_copyable_class_new(0);

    test_copyable_class_copy_assign(&copyable_class_copy_assigned,
                                    &copyable_class);
    assert(test_copyable_class_get_state(&copyable_class_copy_assigned) == 1);

    assert(test_copyable_class_get_num_copied() == 2);

    test_copyable_class_set_state(&copyable_class_copy_assigned, 2);
    assert(test_copyable_class_get_state(&copyable_class) == 1);
    assert(test_copyable_class_get_state(&copyable_class_copy_assigned) == 2);

    test_copyable_class_delete(&copyable_class);
    test_copyable_class_delete(&copyable_class_copy);
    test_copyable_class_delete(&copyable_class_copy_assigned);
  }

  {
    struct test_moveable_class moveable_class =
      test_moveable_class_new(1);
    assert(test_moveable_class_get_state(&moveable_class) == 1);

    assert(test_moveable_class_get_num_moved() == 0);

    struct test_moveable_class moveable_class_moved =
      test_moveable_class_move(&moveable_class);
    assert(test_moveable_class_get_state(&moveable_class_moved) == 1);

    assert(test_moveable_class_get_num_moved() == 1);

    struct test_moveable_class moveable_class_move_assigned =
      test_moveable_class_new(0);

    test_moveable_class_move_assign(&moveable_class_move_assigned,
                                    &moveable_class_moved);
    assert(test_moveable_class_get_state(&moveable_class_move_assigned) == 1);

    assert(test_moveable_class_get_num_moved() == 2);

    test_moveable_class_delete(&moveable_class);
    test_moveable_class_delete(&moveable_class_moved);
    test_moveable_class_delete(&moveable_class_move_assigned);
  }

  /* class parameters */

  /* value parameters */
  {
    struct test_class_parameter a = test_class_parameter_new(1);
    struct test_class_parameter b = test_class_parameter_new(2);

    test_class_parameter_set_copyable(1);

    struct test_class_parameter c = test_add_class(&a, &b);
    assert(test_class_parameter_get_state(&a) == 1);
    assert(test_class_parameter_get_state(&c) == 3);

    test_class_parameter_set_copyable(0);

    test_class_parameter_delete(&a);
    test_class_parameter_delete(&b);
    test_class_parameter_delete(&c);
  }

  /* pointer parameters */
  {
    struct test_class_parameter a = test_class_parameter_new(1);
    struct test_class_parameter b = test_class_parameter_new(2);

    struct test_class_parameter c = test_add_class_pointer(&a, &b);
    assert(test_class_parameter_get_state(&a) == 3);
    assert(test_class_parameter_get_state(&c) == 3);

    test_class_parameter_delete(&a);
    test_class_parameter_delete(&b);
    test_class_parameter_delete(&c);
  }

  /* lvalue reference parameters */
  {
    struct test_class_parameter a = test_class_parameter_new(1);
    struct test_class_parameter b = test_class_parameter_new(2);

    struct test_class_parameter c = test_add_class_lvalue_ref(&a, &b);
    assert(test_class_parameter_get_state(&a) == 3);
    assert(test_class_parameter_get_state(&c) == 3);

    test_class_parameter_delete(&a);
    test_class_parameter_delete(&b);
    test_class_parameter_delete(&c);
  }

  /* rvalue reference parameters */
  {
    struct test_class_parameter a = test_class_parameter_new(1);

    test_class_parameter_set_moveable(1);

    test_noop_class_rvalue_ref(&a);
    assert(test_class_parameter_was_moved(&a));

    test_class_parameter_set_moveable(0);

    test_class_parameter_delete(&a);
  }

  return 0;
}
