local test = require 'test_nested_records'

assert(test.test.ToplevelNestedPrivate == nil)
assert(test.test.ToplevelNestedPublicNestedNestedPrivate == nil)
assert(test.test.FuncLocal == nil)

test.test.Toplevel.new()

local nested_public = test.test.ToplevelNestedPublic.new()
test.test.Toplevel.func(nested_public, 0, test.test.TOPLEVEL_NESTED_PUBLIC_V)

test.test.ToplevelNestedPublicNestedNestedPublic.new()
