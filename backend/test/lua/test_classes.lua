local test = require 'test_classes'

do
  local trivial = test.TestTrivial.new()
  assert(test.TestTrivial.delete == nil)
end

--do
--  assert(test.TestNonConstructible.new == nil)
--  assert(test.TestNonConstructible.delete == nil)
--end
--
--do
--  local a_class = test.TestAClass.new_1()
--  assert(a_class:get_state() == 0)
--end
--
--do
--  local a_class = test.TestAClass.new_2(1)
--  assert(a_class:get_state() == 1)
--
--  a_class:set_state(2)
--  assert(a_class:get_state() == 2)
--end
--
--test.TestAClass.set_static_state(1)
--assert(test.TestAClass.get_static_state() == 1)
--
--collectgarbage()
--collectgarbage()
--
--assert(test.TestAClass.get_num_destroyed() == 2)
