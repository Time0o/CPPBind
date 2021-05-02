#include <assert.h>
#include <string.h>

#include "test_exceptions_c.h"

int main()
{
  assert(test_add_nothrow(1, 2) == 3);

  bind_error_reset();
  test_add_throw_runtime(1, 2);
  assert(bind_error_what() && strcmp(bind_error_what(), "runtime error") == 0);

  bind_error_reset();
  test_add_throw_bogus(1, 2);
  assert(bind_error_what() && strcmp(bind_error_what(), "exception") == 0);

  return 0;
}
