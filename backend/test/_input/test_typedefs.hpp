#include <type_traits>

typedef bool bool_type;

typedef int int_type1;

namespace scope1 {
  struct scope2 {
    using int_type2 = long;
  };
}


namespace test
{

inline bool_type logical_not(bool_type b) noexcept
{ return !b; }

using int_type_common = std::common_type<int_type1, scope1::scope2::int_type2>::type;

inline int_type_common add(int_type1 a, scope1::scope2::int_type2 b) noexcept
{ return a + b; }

typedef struct S {
  S() noexcept = default;
} s_t;

typedef s_t const *s_ptr_t;

inline s_t copy_s_ptr(s_ptr_t s) noexcept
{ return *s; }

} // namespace test
