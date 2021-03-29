#include <type_traits>

typedef int int_type1;
typedef long int_type2;

namespace test
{

using IntTypeCommon = std::common_type<int_type1, int_type2>::type;

inline IntTypeCommon add(int_type1 a, int_type2 b)
{ return a + b; }

} // namespace test
