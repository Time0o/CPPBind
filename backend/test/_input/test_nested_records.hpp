namespace test
{

struct Toplevel
{
public:
  struct NestedPublic
  {
  public:
    struct NestedNestedPublic
    {};
  private:
    struct NestedNestedPrivate
    {};
  };
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
