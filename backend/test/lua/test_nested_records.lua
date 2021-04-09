local test = require 'test_nested_records'

assert(test.TEST_TOPLEVEL_V3 == nil)
assert(test.TEST_TOPLEVEL_V4 == nil)

assert(test.TestToplevelNestedPrivate == nil)

assert(test.TestToplevelNestedPublicNestedNestedPrivate == nil)

assert(test.TestFuncLocal == nil)

test.TestToplevel.new()

local nested_public = test.TestToplevelNestedPublic.new()
test.TestToplevel.func_nested_record(nested_public)

test.TestToplevelNestedPublicNestedNestedPublic.new()

test.TestToplevel.func_nested_enum(test.TEST_TOPLEVEL_V1)
test.TestToplevel.func_nested_enum(test.TEST_TOPLEVEL_V2)
