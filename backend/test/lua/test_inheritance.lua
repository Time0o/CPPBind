local test = require 'test_inheritance'

assert(test.TestBaseAbstract == nil)
assert(test.TestDerived.func_protected == nil)
assert(test.TestDerived.func_private == nil)

do
  local derived = test.TestDerived.new()

  assert(derived:func_1() == 1)
  assert(derived:func_2() == 3)
  assert(derived:func_abstract() == true)
end

do
  local base_1 = test.TestBase1.new()
  assert(base_1:func_1() == 1)

  local base_2 = test.TestBase2.new()
  assert(base_2:func_2() == 2)

  local base_protected = test.TestBaseProtected.new()
  assert(base_protected:func_protected() == true)

  local base_private = test.TestBasePrivate.new()
  assert(base_private:func_private() == true)
end
