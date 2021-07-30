#ifndef GUARD_MIXIN_H
#define GUARD_MIXIN_H

namespace mixin
{

// Records inheriting from this class are not copyable.
class NotCopyable
{
public:
  NotCopyable(NotCopyable const &)            = delete;
  NotCopyable &operator=(NotCopyable const &) = delete;

protected:
  NotCopyable()  = default;
  ~NotCopyable() = default;
};

// Records inheriting from this class are not copy- or movable.
class NotCopyOrMovable
{
public:
  NotCopyOrMovable(NotCopyOrMovable const &)            = delete;
  NotCopyOrMovable &operator=(NotCopyOrMovable const &) = delete;
  NotCopyOrMovable(NotCopyOrMovable &&)                 = delete;
  void operator=(NotCopyOrMovable &&)                   = delete;

protected:
  NotCopyOrMovable()  = default;
  ~NotCopyOrMovable() = default;
};

} // namespace mixin

#endif // GUARD_MIXIN_H
