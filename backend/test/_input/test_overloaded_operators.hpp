#include <cassert>
#include <utility>

namespace test
{

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

  Pointee *operator->()
  { return &_pointee; }

private:
  Pointee _pointee;
};

// XXX test more operators (e.g. class Number)

} // namespace test
