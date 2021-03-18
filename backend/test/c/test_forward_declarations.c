#include <assert.h>

#include "test_forward_declarations_c.h"

int main()
{
  assert(test_add(1, 2) == 3);

  {
    struct test_adder adder;
    test_adder_new(&adder);

    assert(test_adder_add(&adder, 1, 2) == 3);

    test_adder_delete(&adder);
  }

  return 0;
}
