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

enum class Boolean
{
  FALSE,
  TRUE
};

namespace util
{

int *int_new(int i)
{
  int *iptr = new int;
  *iptr = i;

  return iptr;
}

int **int_ptr_new(int *iptr)
{
  int **iptr_ptr = new int *;
  *iptr_ptr = iptr;

  return iptr_ptr;
}

int int_deref(int const *iptr)
{ return *iptr; }

int *int_ptr_deref(int **iptr_ptr)
{ return *iptr_ptr; }

Boolean *bool_enum_new(Boolean b)
{
  Boolean *bptr = new Boolean(b);

  return bptr;
}

Boolean **bool_enum_ptr_new(Boolean *bptr)
{
  Boolean **bptr_ptr = new Boolean *(bptr);

  return bptr_ptr;
}

Boolean bool_enum_deref(Boolean const *bptr)
{ return *bptr; }

Boolean *bool_enum_ptr_deref(Boolean **bptr_ptr)
{ return *bptr_ptr; }

} // namespace util

// integer parameters
inline int add_signed_int(int a, int b)
{ return a + b; }

inline unsigned add_unsigned_int(unsigned a, unsigned b)
{ return a + b; }

inline long long add_large_signed_int(long long a, long long b)
{ return a + b; }

// floating point parameters
inline double add_double(double a, double b)
{ return a + b; }

// boolean parameters
inline bool not_bool(bool a)
{ return !a; }

// enum parameters
inline Boolean not_bool_enum(Boolean a)
{ return a == Boolean::TRUE ? Boolean::FALSE : Boolean::TRUE; }

// pointer parameters
inline int *add_pointer(int *a, int const *b)
{
  *a += *b;
  return a;
}

inline int **add_pointer_to_pointer(int **a, int * const *b)
{
  **a += **b;
  return a;
}

inline Boolean *not_bool_enum_pointer(Boolean *a)
{
  *a = not_bool_enum(*a);
  return a;
}

inline Boolean const *not_bool_enum_pointer_to_const(Boolean const *a, Boolean *res)
{
  *res = not_bool_enum(*a);
  return a;
}

inline Boolean **not_bool_enum_pointer_to_pointer(Boolean **a)
{
  **a = not_bool_enum(**a);
  return a;
}

// lvalue reference parameters
inline int &add_lvalue_ref(int &a, int const &b)
{
  a += b;
  return a;
}

inline Boolean &not_bool_enum_lvalue_ref(Boolean &a)
{
  a = not_bool_enum(a);
  return a;
}

inline Boolean not_bool_enum_lvalue_ref_to_const(Boolean const &a)
{ return not_bool_enum(a); }

inline Boolean *not_bool_enum_lvalue_ref_to_pointer(Boolean * &a)
{
  Boolean b = *a;
  delete a;
  a = new Boolean(not_bool_enum(b));
  return a;
}

// rvalue reference parameters
inline int add_rvalue_ref(int &&a, int const &&b)
{ return a + b; }

inline Boolean not_bool_enum_rvalue_ref(Boolean &&a)
{ return not_bool_enum(a); }

// no parameters
inline int one_no_parameters()
{ return 1; }

// unused parameters
inline int add_unused_parameters(int a, int, int b, int)
{ return a + b; }

// string parameters
char const *min_string_parameters(char const *str1, char const *str2)
{ return std::strcmp(str1, str2) <= 0 ? str1 : str2; }

} // namespace test
