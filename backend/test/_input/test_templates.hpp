#pragma clang diagnostic ignored "-Wc++17-extensions"

#include <any>
#include <cmath>
#include <stack>
#include <type_traits>

namespace test
{

// function templates
template<typename T>
T add(T a, T b)
{ return a + b; }

template int add<int>(int, int);
template long add<long>(long, long);

// multiple template parameters
template<typename T, typename U>
typename std::common_type<T, U>::type mult(T a, U b)
{ return a * b; }

template long mult<int, long>(int, long);
template long mult<long, int>(long, int);

// non-type template parameters
template<typename T, int AMOUNT>
T inc(T a)
{ return a + AMOUNT; }

template int inc<int, 1>(int);
template int inc<int, 2>(int);
template int inc<int, 3>(int);

// default template arguments
constexpr double PI = 3.14;

template<typename T = int>
T pi()
{ return static_cast<T>(PI); }

template int pi<int>();
template double pi<double>();

// template parameter packs
template<typename ...ARGS>
typename std::common_type<ARGS...>::type sum(ARGS ...args)
{ return (args + ...); }

template int sum<int, int>(int, int);
template long sum<int, int, long, long>(int, int, long, long);

// record templates
template<typename T_DEFAULT>
class AnyStack
{
public:
  template<typename T = T_DEFAULT>
  void push(T val)
  { _stack.push(val); }

  template<typename T = T_DEFAULT>
  T pop()
  {
    std::any ret(_stack.top());
    _stack.pop();
    return std::any_cast<T>(ret);
  }

  bool empty() const
  { return _stack.empty(); }

private:
  std::stack<std::any> _stack;
};

template class AnyStack<int>;
template void AnyStack<int>::push<>(int val);
template int AnyStack<int>::pop<>();
template void AnyStack<int>::push<double>(double val);
template double AnyStack<int>::pop<double>();

} // namespace test
