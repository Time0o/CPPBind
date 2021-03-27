#include <assert.h>

#include "test_nested_records_c.h"

void test_toplevel_nested_private_new()
{}

void test_toplevel_nested_public_nested_nested_private_new()
{}

void test_func_local()
{}

int main()
{
  {
    struct test_toplevel obj = test_toplevel_new();
    test_toplevel_delete(&obj);
  }

  {
    struct test_toplevel_nested_public obj = test_toplevel_nested_public_new();
    test_toplevel_nested_public_delete(&obj);
  }

  {
    struct test_toplevel_nested_public_nested_nested_public obj =
      test_toplevel_nested_public_nested_nested_public_new();
    test_toplevel_nested_public_nested_nested_public_delete(&obj);
  }

  return 0;
}
