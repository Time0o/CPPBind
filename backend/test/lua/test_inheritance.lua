local test = require 'test_inheritance'

assert(test.test.BaseAbstract == nil)
assert(test.test.Derived.func_protected == nil)
assert(test.test.Derived.func_private == nil)

do
  local derived = test.test.Derived.new()

  assert(derived:func_1() == 1)
  assert(derived:func_2() == 3)
  assert(derived:func_abstract() == true)
end

do
  local base_1 = test.test.Base1.new()
  assert(base_1:func_1() == 1)

  local base_2 = test.test.Base2.new()
  assert(base_2:func_2() == 2)

  local base_protected = test.test.BaseProtected.new()
  assert(base_protected:func_protected() == true)

  local base_private = test.test.BasePrivate.new()
  assert(base_private:func_private() == true)
end
