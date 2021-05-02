namespace test
{

int add(int, int) noexcept;

int add(int a, int b) noexcept
{ return a + b; }

int add(int, int, int) noexcept;

int add(int a, int b, int c) noexcept
{ return a + b + c; }

class Adder;

class Adder
{
public:
  Adder() noexcept = default;

  int add(int a, int b) const noexcept;

private:
  int _add(int a, int b) const noexcept;
};

int Adder::add(int a, int b) const noexcept
{ return _add(a, b); }

int Adder::_add(int a, int b) const noexcept
{ return a + b; }

} // namespace test
