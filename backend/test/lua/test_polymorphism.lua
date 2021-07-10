require 'test_polymorphism'

local derived_1 = test.Derived1.new()
local derived_2 = test.Derived2.new()

do
  local v = {derived_1, derived_2}

  for i = 1,#v do
    assert(test.func_1(v[1]) == 1)
    assert(test.func_1(v[2]) == 2)
  end
end

do
  local v = {derived_1, derived_2}

  for i = 1,#v do
    assert(test.func_2(v[1]) == -1)
    assert(test.func_2(v[2]) == -2)
  end
end
