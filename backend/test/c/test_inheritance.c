#include <assert.h>

#include "test_inheritance_c.h"

void test_base_abstract_new() {}

void test_derived_func_protected() {}
void test_derived_func_private() {}

int main()
{
  {
    void *derived = test_derived_new();

    assert(test_derived_func_1(derived) == 1);
    assert(test_base_1_func_1(derived) == 1);

    assert(test_derived_func_2(derived) == 3);
    assert(test_base_2_func_2(derived) == 3);

    assert(test_derived_func_abstract(derived) == true);
    assert(test_base_abstract_func_abstract(derived) == true);

    bind_delete(derived);
  }

  {
    void *base_1 = test_base_1_new();
    assert(test_base_1_func_1(base_1) == 1);
    bind_delete(base_1);

    void *base_2 = test_base_2_new();
    assert(test_base_2_func_2(base_2) == 2);
    bind_delete(base_2);

    void *base_protected = test_base_protected_new();
    assert(test_base_protected_func_protected(base_protected) == true);
    bind_delete(base_protected);

    void *base_private = test_base_private_new();
    assert(test_base_private_func_private(base_private) == true);
    bind_delete(base_private);
  }

  return 0;
}
