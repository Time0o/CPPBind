#include <assert.h>

#include "test_overloaded_operators_c.h"

int main()
{
  {
    void * pointer = test_pointer_new(1);

    void * pointee1 = test_pointer_op_arrow(pointer);
    assert(test_pointee_get_state(pointee1) == 1);

    test_pointee_set_state(pointee1, 2);
    void * pointee2 = test_pointer_op_arrow(pointer);
    assert(test_pointee_get_state(pointee2) == 2);

    bind_delete(pointer);
    bind_delete(pointee1);
    bind_delete(pointee2);
  }

  {
    void * class = test_class_with_callable_member_new();

    assert(test_class_with_callable_member_sum(class, 1, 2) == 3);

    bind_delete(class);
  }

  return 0;
}
