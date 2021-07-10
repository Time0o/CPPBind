require 'test_nested_records'

assert(test.ToplevelNestedPrivate == nil)
assert(test.ToplevelNestedPublicNestedNestedPrivate == nil)
assert(test.FuncLocal == nil)

test.Toplevel.new()

local nested_public = test.ToplevelNestedPublic.new()
test.Toplevel.func(nested_public, 0, test.TOPLEVEL_NESTED_PUBLIC_V)

test.ToplevelNestedPublicNestedNestedPublic.new()
