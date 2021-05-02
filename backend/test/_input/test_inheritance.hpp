namespace test
{

struct Base1
{
  Base1() noexcept = default;

  virtual ~Base1() noexcept = default;

  virtual int func1() const noexcept
  { return 1; }
};

struct Base2 : public Base1
{
  Base2() noexcept = default;

  virtual ~Base2() noexcept = default;

  virtual int func2() const noexcept
  { return 2; }
};

struct BaseAbstract
{
  BaseAbstract() noexcept = default;

  virtual ~BaseAbstract() noexcept = default;

  virtual bool func_abstract() const noexcept = 0;
};

struct BaseProtected
{
  BaseProtected() noexcept = default;

  bool func_protected() const noexcept
  { return true; }
};

struct BasePrivate
{
  BasePrivate() noexcept = default;

  bool func_private() const noexcept
  { return true; }
};

struct Derived : public Base2,
                 public BaseAbstract,
                 protected BaseProtected,
                 private BasePrivate
{
  Derived() noexcept = default;

  int func2() const noexcept override
  { return 3; }

  bool func_abstract() const noexcept override
  { return true; }
};

} // namespace test
