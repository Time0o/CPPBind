#include <cassert>
#include <utility>

namespace test
{

class Integer
{
public:
  Integer(int value)
  : _value(value)
  {}

  int operator*() const
  { return _value; }

  Integer operator++()
  {
    ++_value;
    return *this;
  }

  Integer operator++(int)
  {
    auto tmp(*this);
    ++_value;
    return tmp;
  }

  explicit operator bool() const
  { return _value != 0; }

private:
  int _value;
};

inline Integer operator*(Integer const &i1, Integer const &i2)
{ return *i1 * *i2; }

class Pointee
{
public:
  Pointee(int state)
  : _state(state)
  {}

  void set_state(int state)
  { _state = state; }

  int get_state() const
  { return _state; }

private:
  int _state;
};

class Pointer
{
public:
  Pointer(int state)
  : _pointee(state)
  {}

  Pointee &operator*()
  { return _pointee; }

  Pointee *operator->()
  { return &_pointee; }

private:
  Pointee _pointee;
};

struct ClassWithCallableMember
{
  struct Sum
  {
    int operator()(int a, int b) const
    { return a + b; }
  } sum;
};

} // namespace test
