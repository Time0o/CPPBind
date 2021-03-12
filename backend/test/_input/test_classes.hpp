#pragma clang diagnostic ignored "-Wc++17-extensions"

#include <cassert>
#include <utility>

namespace test
{

struct Trivial
{};

struct NonConstructible
{
  NonConstructible() = delete;
};

struct ImplicitlyNonConstructible
{
  int &dummy;
};

class AClass
{
public:
  AClass()
  : _state(0)
  {}

  AClass(int initial_state)
  : _state(initial_state)
  { ++_num_created; }

  ~AClass()
  { ++_num_destroyed; }

  static void set_static_state(int state)
  { _static_state = state; }

  static int get_static_state()
  { return _static_state; }

  static int get_num_created()
  { return _num_created; }

  static int get_num_destroyed()
  { return _num_destroyed; }

  void set_state(int state)
  { set_state_private(state); }

  int get_state() const
  { return _state; }

private:
  void set_state_private(int state)
  { _state = state; }

  int _state;

  inline static int _static_state = 0;

  inline static int _num_created = 0;
  inline static int _num_destroyed = 0;
};

class CopyableClass
{
public:
  CopyableClass(int state)
  : _state(state)
  {}

  CopyableClass(CopyableClass const &other)
  : _state(other._state)
  { ++_num_copied; }

  CopyableClass &operator=(CopyableClass const &other)
  {
    _state = other._state;
    ++_num_copied;
    return *this;
  }

  CopyableClass(CopyableClass &&other) = delete;
  void operator=(CopyableClass &&other) = delete;

  static int get_num_copied()
  { return _num_copied; }

  void set_state(int state)
  { _state = state; }

  int get_state() const
  { return _state; }

private:
  int _state;

  inline static int _num_copied = 0;
};

class MoveableClass
{
public:
  MoveableClass(int state)
  : _state(state)
  {}

  MoveableClass(MoveableClass const &other) = delete;
  MoveableClass &operator=(MoveableClass const &other) = delete;

  MoveableClass(MoveableClass &&other)
  : _state(other._state)
  { ++_num_moved; }

  void operator=(MoveableClass &&other)
  {
    _state = other._state;
    ++_num_moved;
  }

  static int get_num_moved()
  { return _num_moved; }

  void set_state(int state)
  { _state = state; }

  int get_state() const
  { return _state; }

private:
  int _state;

  inline static int _num_moved = 0;
};

class ClassParameter
{
public:
  ClassParameter(int state)
  : _state(state)
  {}

  ClassParameter(ClassParameter const &other)
  { *this = other; }

  ClassParameter &operator=(ClassParameter const &other)
  {
    assert(_copyable);
    _state = other._state;
    return *this;
  }

  ClassParameter(ClassParameter &&other)
  { *this = std::move(other); }

  void operator=(ClassParameter &&other)
  {
    if (_moveable) {
      _state = other._state;
      other._moved = true;
    } else {
      assert(_copyable);
      _state = other._state;
    }
  }

  static void set_copyable(bool val)
  { _copyable = val; }

  static void set_moveable(bool val)
  { _moveable = val; }

  bool was_moved() const
  { return _moved; }

  void set_state(int state)
  { _state = state; }

  int get_state() const
  { return _state; }

private:
  int _state;
  bool _moved = false;

  inline static bool _copyable = false;
  inline static bool _moveable = false;
};

// value parameters
inline ClassParameter add_class(ClassParameter a,
                                ClassParameter const b)
{
  a.set_state(a.get_state() + b.get_state());
  return a;
}

// pointer parameters
inline ClassParameter *add_class_pointer(ClassParameter *a,
                                         ClassParameter const *b)
{
  a->set_state(a->get_state() + b->get_state());
  return a;
}

// XXX pointer to pointer

// lvalue reference parameters
inline ClassParameter &add_class_lvalue_ref(ClassParameter &a,
                                            ClassParameter const &b)
{
  a.set_state(a.get_state() + b.get_state());
  return a;
}

// XXX lvalue reference to pointer

// rvalue reference parameters
inline void noop_class_rvalue_ref(ClassParameter &&a)
{ ClassParameter b(std::move(a)); }

} // namespace test
