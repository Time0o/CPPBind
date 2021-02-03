#pragma clang diagnostic ignored "-Wc++17-extensions"

namespace test
{

struct Trivial
{};

struct NonConstructible
{
  NonConstructible() = delete;
};

class AClass
{
public:
  AClass()
  : _state(0)
  {}

  AClass(int initial_state)
  : _state(initial_state)
  {}

  ~AClass()
  {}

  static void set_static_state(int state)
  { _static_state = state; }

  static int get_static_state()
  { return _static_state; }

  void set_state(int state)
  { set_state_private(state); }

  int get_state() const
  { return _state; }

private:
  void set_state_private(int state)
  { _state = state; }

  int _state;
  inline static int _static_state = 0;
};

} // namespace test
