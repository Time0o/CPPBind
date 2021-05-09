local test = require 'test.exceptions'

assert(test.test.add_nothrow(1, 2) == 3)

res, e = pcall(test.test.add_throw_runtime, 1, 2)
assert(not res)
assert(e == 'runtime error')

res, e = pcall(test.test.add_throw_bogus, 1, 2)
assert(not res)
assert(e == 'exception')
