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

  static void func_nested_record(NestedPublic *) {}

  enum NestedEnumPublic {
    V1,
    V2
  };

  static void func_nested_enum(NestedEnumPublic) {}

private:
  enum NestedEnumPrivate {
    V3,
    V4
  };

  struct NestedPrivate
  {};
};

inline void func()
{
  struct Local
  {};
}

} // namespace test
