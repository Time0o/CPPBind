#include <assert.h>

#include "test_typedefs_c.h"

int main()
{
  {
    bool_type_t b = 1;
    bool_type_t not_b = test_logical_not(b);
    assert(not_b == 0);
  }

  {
    int_type_1_t a = 1;
    int_type_2_t b = 2;
    test_int_type_common_t c = test_add(a, b);
    assert(c == 3);
  }

  {
    test_s_t s = test_s_new();
    test_s_t s_copy = test_copy(&s);

    test_s_delete(&s);
    test_s_delete(&s_copy);
  }

  return 0;
}
