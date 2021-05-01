namespace test
{

struct Toplevel
{
public:
  struct NestedPublic
  {
  public:
    using T = int;

    enum E {
      V
    };

    struct NestedNestedPublic
    {};
  private:
    struct NestedNestedPrivate
    {};
  };

  static void func(NestedPublic const *, NestedPublic::T, NestedPublic::E) {}

private:
  struct NestedPrivate
  {};
};

inline void func()
{
  struct Local
  {};
}

} // namespace test
