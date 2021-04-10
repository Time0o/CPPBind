#include <type_traits>

typedef bool bool_type;
typedef int int_type1;
typedef long int_type2;

namespace test
{

// typedef
inline bool_type logical_not(bool_type b)
{ return !b; }

// typedef + using
using IntTypeCommon = std::common_type<int_type1, int_type2>::type;

inline IntTypeCommon add(int_type1 a, int_type2 b)
{ return a + b; }

} // namespace test
