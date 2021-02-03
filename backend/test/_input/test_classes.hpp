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

} // namespace test
