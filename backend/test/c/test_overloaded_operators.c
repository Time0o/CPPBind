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

    _delete(pointer);
    _delete(pointee1);
    _delete(pointee2);
  }

  return 0;
}
