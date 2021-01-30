#include <assert.h>

#include "test_overload_c.h"

int main()
{
  assert(test_foo_1() == 3);
  assert(test_foo_1_() == 4);
  assert(test_foo_1__(0) == 1);
  assert(test_foo_2(0.0) == 2);

  return 0;
}
