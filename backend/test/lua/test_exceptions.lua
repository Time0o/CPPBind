local test = require 'test_exceptions'

assert(test.test_add_nothrow(1, 2) == 3)

-- TODO: this will always leak unless Lua was built in C++ mode
--res, e = pcall(test.test_add_throw_runtime, 1, 2)
--assert(not res)
--assert(e == 'runtime error')
--
--res, e = pcall(test.test_add_throw_bogus, 1, 2)
--assert(not res)
--assert(e == 'exception')