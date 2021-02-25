#ifndef GUARD_UTIL_H
#define GUARD_UTIL_H

#include <deque>

namespace cppbind
{

namespace util
{

class NotCopyOrMoveable
{
public:
  NotCopyOrMoveable(NotCopyOrMoveable const &)            = delete;
  NotCopyOrMoveable &operator=(NotCopyOrMoveable const &) = delete;
  NotCopyOrMoveable(NotCopyOrMoveable &&)                 = delete;
  void operator=(NotCopyOrMoveable &&)                    = delete;

protected:
  NotCopyOrMoveable()  = default;
  ~NotCopyOrMoveable() = default;
};

template<typename T>
std::vector<T const *> vectorOfPointers(std::deque<T> D)
{
  std::vector<T const *> V;
  V.reserve(D.size());

  for (auto const &Elem : D)
    V.push_back(&Elem);

  return V;
}

template<typename IT>
std::vector<typename IT::value_type const *>
vectorOfPointers(IT first, IT last)
{
  std::vector<typename IT::value_type const *> V;

  for (auto It = first; It != last; ++It)
    V.push_back(&(*It));

  return V;
}

} // namespace util

} // namespace cppbind

#endif // GUARD_UTIL_H
