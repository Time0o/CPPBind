#include <assert.h>

#include "test_typedefs_c.h"

int main()
{
  int_type_1 a = 1;
  int_type_2 b = 2;
  test_int_type_common c = test_add(a, b);
  assert(c == 3);

  return 0;
}
