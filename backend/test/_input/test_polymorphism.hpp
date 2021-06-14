namespace test
{

struct Base1
{
  Base1() noexcept = default;

  virtual ~Base1() noexcept = default;

  virtual int func1() const noexcept = 0;
};

struct Base2 : public Base1
{
  Base2() noexcept = default;

  virtual ~Base2() noexcept = default;

  virtual int func2() const noexcept = 0;
};

struct Derived1 : public Base2
{
  Derived1() noexcept = default;

  int func1() const noexcept override
  { return 1; }

  int func2() const noexcept override
  { return -1; }
};

struct Derived2 : public Base2
{
  Derived2() noexcept = default;

  int func1() const noexcept override
  { return 2; }

  int func2() const noexcept override
  { return -2; }
};

inline int func1(Base1 const &obj) noexcept
{ return obj.func1(); }

inline int func2(Base2 const &obj) noexcept
{ return obj.func2(); }

} // namespace test
