#include <stdexcept>

namespace test
{

int add_nothrow(int a, int b) noexcept
{ return a + b; }

int add_throw_runtime(int, int)
{ throw std::runtime_error("runtime error"); }

int add_throw_bogus(int, int)
{ throw 0; }

} // namespace test
