#ifndef GUARD_MIXIN_H
#define GUARD_MIXIN_H

namespace cppbind
{

namespace mixin
{

class NotCopyOrMoveable
{
public:
  NotCopyOrMoveable(const NotCopyOrMoveable &)            = delete;
  NotCopyOrMoveable& operator=(const NotCopyOrMoveable &) = delete;
  NotCopyOrMoveable(NotCopyOrMoveable &&)                 = delete;
  void operator=(NotCopyOrMoveable &&)                    = delete;

protected:
  NotCopyOrMoveable()  = default;
  ~NotCopyOrMoveable() = default;
};

} // namespace mixin

} // namespace cppbind

#endif // GUARD_MIXIN_H
