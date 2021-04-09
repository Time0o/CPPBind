#include <assert.h>

#include "test_nested_records_c.h"

unsigned TEST_TOPLEVEL_V3;
unsigned TEST_TOPLEVEL_V4;

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
    test_toplevel_func_nested_record(&obj);
    test_toplevel_nested_public_delete(&obj);
  }

  {
    struct test_toplevel_nested_public_nested_nested_public obj =
      test_toplevel_nested_public_nested_nested_public_new();
    test_toplevel_nested_public_nested_nested_public_delete(&obj);
  }

  test_toplevel_func_nested_enum(TEST_TOPLEVEL_V1);
  test_toplevel_func_nested_enum(TEST_TOPLEVEL_V2);

  return 0;
}
