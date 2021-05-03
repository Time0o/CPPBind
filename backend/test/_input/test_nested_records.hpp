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

    NestedPublic() noexcept = default;

    struct NestedNestedPublic
    {
      NestedNestedPublic() noexcept = default;
    };

  private:
    struct NestedNestedPrivate
    {};
  };

  Toplevel() noexcept = default;

  static void func(NestedPublic const *, NestedPublic::T, NestedPublic::E) noexcept {}

private:
  struct NestedPrivate
  {};
};

inline void func() noexcept
{
  struct Local
  {};
}

} // namespace test
