#include <cassert>
#include <utility>

namespace test
{

class Integer
{
public:
  Integer(int value) noexcept
  : _value(value)
  {}

  int operator*() const noexcept
  { return _value; }

  Integer operator++() noexcept
  {
    ++_value;
    return *this;
  }

  Integer operator++(int) noexcept
  {
    auto tmp(*this);
    ++_value;
    return tmp;
  }

  explicit operator bool() const noexcept
  { return _value != 0; }

private:
  int _value;
};

inline Integer operator*(Integer const &i1, Integer const &i2) noexcept
{ return *i1 * *i2; }

class Pointee
{
public:
  Pointee(int state) noexcept
  : _state(state)
  {}

  void set_state(int state) noexcept
  { _state = state; }

  int get_state() const noexcept
  { return _state; }

private:
  int _state;
};

class Pointer
{
public:
  Pointer(int state) noexcept
  : _pointee(state)
  {}

  Pointee &operator*() noexcept
  { return _pointee; }

  Pointee *operator->() noexcept
  { return &_pointee; }

private:
  Pointee _pointee;
};

struct ClassWithCallableMember
{
  ClassWithCallableMember() noexcept = default;

  struct Sum
  {
    int operator()(int a, int b) const noexcept
    { return a + b; }
  } sum;
};

} // namespace test
