#include <assert.h>

#include "test_inheritance_c.h"

void test_base_abstract_new() {}

void test_derived_func_protected() {}
void test_derived_func_private() {}

int main()
{
  {
    struct test_derived *derived = test_derived_new();
    struct test_base_1 *base_1 = (struct test_base_1 *)derived;
    struct test_base_2 *base_2 = (struct test_base_2 *)derived;
    struct test_base_abstract *base_abstract = (struct test_base_abstract *)derived;

    assert(test_derived_func_1(derived) == 1);
    assert(test_base_1_func_1(base_1) == 1);

    assert(test_derived_func_2(derived) == 3);
    assert(test_base_2_func_2(base_2) == 3);

    assert(test_derived_func_abstract(derived) == true);
    assert(test_base_abstract_func_abstract(base_abstract) == true);

    _delete(derived);
  }

  {
    struct test_base_1 *base_1 = test_base_1_new();
    assert(test_base_1_func_1(base_1) == 1);
    _delete(base_1);

    struct test_base_2 *base_2 = test_base_2_new();
    assert(test_base_2_func_2(base_2) == 2);
    _delete(base_2);

    struct test_base_protected *base_protected = test_base_protected_new();
    assert(test_base_protected_func_protected(base_protected) == true);
    _delete(base_protected);

    struct test_base_private *base_private = test_base_private_new();
    assert(test_base_private_func_private(base_private) == true);
    _delete(base_private);
  }

  return 0;
}
