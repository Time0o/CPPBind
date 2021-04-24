namespace test
{

enum SimpleEnum : unsigned
{
  ENUM_1 = 1u,
  ENUM_2 = 2u
};

enum struct ScopedEnum : unsigned
{
  SCOPED_ENUM_1 = 1u,
  SCOPED_ENUM_2 = 2u
};


enum : unsigned
{
  ANONYMOUS_ENUM_1 = 1u,
  ANONYMOUS_ENUM_2 = 2u
};

namespace
{
  enum EnumInAnonymousNamespace
  {
    ENUM_IN_ANONYMOUS_NAMESPACE_1 = 1u,
    ENUM_IN_ANONYMOUS_NAMESPACE_2 = 2u
  };
}

constexpr unsigned unsigned_constexpr_1 = 1u;
constexpr double double_constexpr_1 = 1.0;

#define MACRO_CONST 1
#define MACRO_EXPR ~(1 << 3)

}
