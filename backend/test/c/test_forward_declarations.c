#include <assert.h>

#include "test_forward_declarations_c.h"

int main()
{
  assert(test_add(1, 2) == 3);

  {
    void *adder = test_adder_new();

    assert(test_adder_add(adder, 1, 2) == 3);

    bind_delete(adder);
  }

  return 0;
}
