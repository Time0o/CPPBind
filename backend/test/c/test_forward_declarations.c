#include <assert.h>

#include "test_forward_declarations_c.h"

void test_add_3() {}

int main()
{
  assert(test_add_1(1, 2) == 3);
  assert(test_add_2(1, 2, 3) == 6);

  {
    struct test_adder adder = test_adder_new();

    assert(test_adder_add(&adder, 1, 2) == 3);

    test_adder_delete(&adder);
  }

  return 0;
}
