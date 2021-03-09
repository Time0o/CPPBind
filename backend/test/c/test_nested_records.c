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
  _delete(test_toplevel_new());
  _delete(test_toplevel_nested_public_new());
  _delete(test_toplevel_nested_public_nested_nested_public_new());

  return 0;
}
