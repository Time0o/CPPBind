local test = require 'test_inheritance'

local derived = test.TestDerived:new()

assert(derived:func_1() == 1)
assert(derived:func_1_pure_virtual() == 1)
assert(derived:func_2() == 3)
assert(derived:func_2_pure_virtual() == 2)
