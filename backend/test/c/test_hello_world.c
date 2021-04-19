#include <assert.h>
#include <string.h>

#include "test_hello_world_c.h"

int main()
{
  assert(strcmp(test_hello_world(), "hello world") == 0);
}
