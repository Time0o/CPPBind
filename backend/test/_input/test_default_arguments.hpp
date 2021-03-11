#include <cmath>
#include <limits>

namespace test
{

constexpr double e = 2.71828;

enum : int
{ EXP_DEFAULT = 1 };

constexpr bool round_default()
{ return false; }

enum class Round
{
  NEAREST,
  DOWNWARD,
  UPWARD
};

inline double pow_default_arguments(
  double base = e,
  int exp = EXP_DEFAULT,
  bool round = round_default(),
  Round rounding_mode = Round::NEAREST)
{
  double p = std::pow(base, exp);

  if (round) {
    switch (rounding_mode) {
    case Round::DOWNWARD:
      p = std::floor(p);
      break;
    case Round::UPWARD:
      p = std::ceil(p);
      break;
    case Round::NEAREST:
      p = std::round(p);
      break;
    }
  }

  return p;
}

enum : long long
{ LARGE_SIGNED = std::numeric_limits<long long>::max() };

long long default_large_signed(long long i = LARGE_SIGNED)
{ return i; }

enum : unsigned
{ LARGE_UNSIGNED = std::numeric_limits<unsigned>::max() };

unsigned default_large_unsigned(unsigned u = LARGE_UNSIGNED)
{ return u; }

constexpr int func_constexpr()
{ return 1; }

int default_constexpr_function_call(int i = func_constexpr())
{ return i; }

inline int func()
{ return 1; }

int default_function_call(int i = func())
{ return i; }

template<typename T>
T default_template(T n = 1)
{ return n; }

template unsigned default_template<unsigned>(unsigned);

template<typename T, typename ...ARGS>
T default_template_after_parameter_pack(ARGS ..., T n = 1)
{ return n; }

template unsigned default_template_after_parameter_pack<unsigned, int, int>(int, int, unsigned);

} // namespace test
