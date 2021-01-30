local test = require 'test_overload'

assert(test.test_foo_1() == 3);
assert(test.test_foo_1_() == 4);
assert(test.test_foo_1__(0) == 1);
assert(test.test_foo_2(0.0) == 2);
