namespace test
{

struct Base1
{
  virtual ~Base1() = default;

  virtual int func1() const
  { return 1; }
};

struct Base2 : public Base1
{
  virtual ~Base2() = default;

  virtual int func2() const
  { return 2; }
};

struct BaseAbstract
{
  virtual ~BaseAbstract() = default;

  virtual bool func_abstract() const = 0;
};

struct BaseProtected
{
  bool func_protected() const
  { return true; }
};

struct BasePrivate
{
  bool func_private() const
  { return true; }
};

struct Derived : public Base2,
                 public BaseAbstract,
                 protected BaseProtected,
                 private BasePrivate
{
  int func2() const override
  { return 3; }

  bool func_abstract() const override
  { return true; }
};

} // namespace test
