#ifndef GUARD_CPPBIND_C_UTIL_CC_H
#define GUARD_CPPBIND_C_UTIL_CC_H

#include <cassert>
#include <cstring>
#include <type_traits>

namespace cppbind
{

namespace c
{

template<typename T, typename S, typename ...ARGS>
S make_owning_struct(ARGS &&...args)
{
  S s;

  new (s.obj.mem) T(std::forward<ARGS>(args)...);

  s.is_initialized = 1;
  s.is_const = 0;
  s.is_owning = 1;

  return s;
}

template<typename T, typename S>
typename std::enable_if<std::is_const<T>::value, S>::type
make_non_owning_struct(T *obj)
{
  S s;

  s.obj.ptr = const_cast<void *>(static_cast<void const *>(obj));
  s.is_initialized = 1;
  s.is_const = 1;
  s.is_owning = 0;

  return s;
}

template<typename T, typename S>
typename std::enable_if<!std::is_const<T>::value, S>::type
make_non_owning_struct(T *obj)
{
  S s;

  s.obj.ptr = static_cast<void *>(obj);
  s.is_initialized = 1;
  s.is_const = 0;
  s.is_owning = 0;

  return s;
}

template<typename T, typename S>
typename std::enable_if<std::is_const<T>::value, T *>::type
struct_cast(S const *s)
{
  assert(s->is_initialized);

  if (s->is_owning)
    return reinterpret_cast<T *>(&s->obj.mem);
  else
    return static_cast<T *>(s->obj.ptr);
}

template<typename T, typename S>
typename std::enable_if<!std::is_const<T>::value, T *>::type
struct_cast(S const *s)
{
  assert(!s->is_const);

  return const_cast<T *>(struct_cast<T const>(s));
}

} // namespace c

} // namespace cppbind

#endif // GUARD_CPPBIND_C_UTIL_CC_H
