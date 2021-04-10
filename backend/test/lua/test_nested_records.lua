local test = require 'test_nested_records'

assert(test.TestToplevelNestedPrivate == nil)
assert(test.TestToplevelNestedPublicNestedNestedPrivate == nil)
assert(test.TestFuncLocal == nil)

test.TestToplevel.new()

local nested_public = test.TestToplevelNestedPublic.new()
test.TestToplevel.func(nested_public, 0, test.TEST_TOPLEVEL_NESTED_PUBLIC_V)

test.TestToplevelNestedPublicNestedNestedPublic.new()
