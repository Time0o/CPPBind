#include <cmath>

namespace test
{

constexpr double e = 2.71828;

enum : int { EXP_DEFAULT = 1 };

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

} // namespace test
