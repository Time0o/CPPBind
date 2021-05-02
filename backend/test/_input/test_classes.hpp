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
  ~NonConstructible() = delete;
};

struct ImplicitlyNonConstructible
{
  ~ImplicitlyNonConstructible() = delete;

  int &dummy;
};

class AClass
{
public:
  AClass() noexcept
  : _state(0)
  {}

  AClass(int initial_state) noexcept
  : _state(initial_state)
  { ++_num_created; }

  ~AClass() noexcept
  { ++_num_destroyed; }

  static void set_static_state(int state) noexcept
  { _static_state = state; }

  static int get_static_state() noexcept
  { return _static_state; }

  static int get_num_created() noexcept
  { return _num_created; }

  static int get_num_destroyed() noexcept
  { return _num_destroyed; }

  void set_state(int state) noexcept
  { set_state_private(state); }

  int get_state() const noexcept
  { return _state; }

private:
  void set_state_private(int state) noexcept
  { _state = state; }

  int _state;

  inline static int _static_state = 0;

  inline static int _num_created = 0;
  inline static int _num_destroyed = 0;
};

class CopyableClass
{
public:
  CopyableClass(int state) noexcept
  : _state(state)
  {}

  CopyableClass(CopyableClass const &other) noexcept
  : _state(other._state)
  { ++_num_copied; }

  CopyableClass &operator=(CopyableClass const &other) noexcept
  {
    _state = other._state;
    ++_num_copied;
    return *this;
  }

  CopyableClass(CopyableClass &&other) = delete;
  void operator=(CopyableClass &&other) = delete;

  static int get_num_copied() noexcept
  { return _num_copied; }

  void set_state(int state) noexcept
  { _state = state; }

  int get_state() const noexcept
  { return _state; }

private:
  int _state;

  inline static int _num_copied = 0;
};

class MoveableClass
{
public:
  MoveableClass(int state) noexcept
  : _state(state)
  {}

  MoveableClass(MoveableClass const &other) = delete;
  MoveableClass &operator=(MoveableClass const &other) = delete;

  MoveableClass(MoveableClass &&other) noexcept
  : _state(other._state)
  { ++_num_moved; }

  void operator=(MoveableClass &&other) noexcept
  {
    _state = other._state;
    ++_num_moved;
  }

  static int get_num_moved() noexcept
  { return _num_moved; }

  void set_state(int state) noexcept
  { _state = state; }

  int get_state() const noexcept
  { return _state; }

private:
  int _state;

  inline static int _num_moved = 0;
};

class ClassParameter
{
public:
  ClassParameter(int state) noexcept
  : _state(state)
  {}

  ClassParameter(ClassParameter const &other) noexcept
  { *this = other; }

  ClassParameter &operator=(ClassParameter const &other) noexcept
  {
    assert(_copyable);
    _state = other._state;
    return *this;
  }

  ClassParameter(ClassParameter &&other) noexcept
  { *this = std::move(other); }

  void operator=(ClassParameter &&other) noexcept
  {
    if (_moveable) {
      _state = other._state;
      other._moved = true;
    } else {
      assert(_copyable);
      _state = other._state;
    }
  }

  static void set_copyable(bool val) noexcept
  { _copyable = val; }

  static void set_moveable(bool val) noexcept
  { _moveable = val; }

  bool was_moved() const noexcept
  { return _moved; }

  void set_state(int state) noexcept
  { _state = state; }

  int get_state() const noexcept
  { return _state; }

private:
  int _state;
  bool _moved = false;

  inline static bool _copyable = false;
  inline static bool _moveable = false;
};

// value parameters
inline ClassParameter add_class(ClassParameter a,
                                ClassParameter const b) noexcept
{
  a.set_state(a.get_state() + b.get_state());
  return a;
}

// pointer parameters
inline ClassParameter const *add_class_pointer(ClassParameter *a,
                                               ClassParameter const *b) noexcept
{
  a->set_state(a->get_state() + b->get_state());
  return a;
}

// lvalue reference parameters
inline ClassParameter const &add_class_lvalue_ref(ClassParameter &a,
                                                  ClassParameter const &b) noexcept
{
  a.set_state(a.get_state() + b.get_state());
  return a;
}

// rvalue reference parameters
inline void noop_class_rvalue_ref(ClassParameter &&a) noexcept
{ ClassParameter b(std::move(a)); }

} // namespace test
