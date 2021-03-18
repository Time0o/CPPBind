#include <assert.h>
#include <errno.h>

#include "test_exceptions_c.h"

int main()
{
  assert(test_add_nothrow(1, 2) == 3);

  errno = 0;
  test_add_throw_runtime(1, 2);
  assert(errno == EBIND);

  errno = 0;
  test_add_throw_bogus(1, 2);
  assert(errno == EBIND);

  return 0;
}
