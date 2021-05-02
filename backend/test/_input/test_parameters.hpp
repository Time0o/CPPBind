#include <cstddef>
#include <cstring>
#include <limits>

namespace test
{

enum : int
{
  MIN_SIGNED_INT = std::numeric_limits<int>::min(),
  MAX_SIGNED_INT = std::numeric_limits<int>::max()
};

enum : unsigned
{
  MAX_UNSIGNED_INT = std::numeric_limits<unsigned>::max()
};

enum : long long
{
  MIN_LARGE_SIGNED_INT = std::numeric_limits<long long>::min(),
  MAX_LARGE_SIGNED_INT = std::numeric_limits<long long>::max()
};

constexpr double EPSILON_DOUBLE = std::numeric_limits<double>::epsilon();

enum Boolean : bool
{
  FALSE,
  TRUE
};

enum class BooleanScoped : bool
{
  FALSE,
  TRUE
};

namespace util
{

int *int_new(int i) noexcept
{
  int *iptr = new int;
  *iptr = i;

  return iptr;
}

int **int_ptr_new(int *iptr) noexcept
{
  int **iptr_ptr = new int *;
  *iptr_ptr = iptr;

  return iptr_ptr;
}

int int_deref(int const *iptr) noexcept
{ return *iptr; }

int *int_ptr_deref(int **iptr_ptr) noexcept
{ return *iptr_ptr; }

} // namespace util

// integer parameters
inline int add_signed_int(int a, int b) noexcept
{ return a + b; }

inline unsigned add_unsigned_int(unsigned a, unsigned b) noexcept
{ return a + b; }

inline long long add_large_signed_int(long long a, long long b) noexcept
{ return a + b; }

// floating point parameters
inline double add_double(double a, double b) noexcept
{ return a + b; }

// boolean parameters
inline bool not_bool(bool a) noexcept
{ return !a; }

// enum parameters
inline Boolean not_bool_enum(Boolean a) noexcept
{ return a == Boolean::TRUE ? Boolean::FALSE : Boolean::TRUE; }

inline BooleanScoped not_bool_scoped_enum(BooleanScoped a) noexcept
{ return a == BooleanScoped::TRUE ? BooleanScoped::FALSE : BooleanScoped::TRUE; }

// pointer parameters
inline int *add_pointer(int *a, int const *b) noexcept
{
  *a += *b;
  return a;
}

inline int **add_pointer_to_pointer(int **a, int **b) noexcept
{
  **a += **b;
  return a;
}

// string parameters
char const *min_string_parameters(char const *str1, char const *str2) noexcept
{ return std::strcmp(str1, str2) <= 0 ? str1 : str2; }

// lvalue reference parameters
inline int &add_lvalue_ref(int &a, int const &b) noexcept
{
  a += b;
  return a;
}

// rvalue reference parameters
inline int add_rvalue_ref(int &&a, int const &&b) noexcept
{ return a + b; }

// no parameters
inline int one_no_parameters() noexcept
{ return 1; }

// unused parameters
inline int add_unused_parameters(int a, int, int b, int) noexcept
{ return a + b; }

} // namespace test
