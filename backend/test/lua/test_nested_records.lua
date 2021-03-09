local test = require 'test_nested_records'

assert(test.TestToplevelNestedPrivate == nil)
assert(test.TestToplevelNestedPublicNestedNestedPrivate == nil)
assert(test.TestFuncLocal == nil)

test.TestToplevel.new()
test.TestToplevelNestedPublic.new()
test.TestToplevelNestedPublicNestedNestedPublic.new()
