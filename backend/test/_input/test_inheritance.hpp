namespace test
{

// XXX larger class hierarchie (including non-abstract base classes)
// XXX private and protected inheritance

struct Base1
{
  virtual ~Base1() = default;

  virtual int func1() const
  { return 1; }

  virtual int func1_pure_virtual() const = 0;
};

struct Base2
{
  virtual ~Base2() = default;

  virtual int func2() const
  { return 2; }

  virtual int func2_pure_virtual() const = 0;
};

struct Derived : public Base1, Base2
{
  int func1_pure_virtual() const override
  { return 1; }

  int func2() const override
  { return 3; }

  int func2_pure_virtual() const override
  { return 2; }
};

} // namespace test
