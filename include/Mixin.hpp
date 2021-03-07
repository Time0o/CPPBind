#ifndef GUARD_MIXIN_H
#define GUARD_MIXIN_H

namespace mixin
{

class NotCopyable
{
public:
  NotCopyable(NotCopyable const &)            = delete;
  NotCopyable &operator=(NotCopyable const &) = delete;

protected:
  NotCopyable()  = default;
  ~NotCopyable() = default;
};

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

} // namespace mixin

#endif // GUARD_MIXIN_H
