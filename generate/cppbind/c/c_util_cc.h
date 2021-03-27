namespace cppbind
{

namespace c
{

template<typename T, typename S, typename ...ARGS>
S *init_owning_struct(S *s, ARGS &&...args)
{
  new (s->obj.mem) T(std::forward<ARGS>(args)...);
  s->is_initialized = 1;
  s->is_const = 0;
  s->is_owning = 1;
  return s;
}

template<typename T, typename S>
S *init_non_owning_struct(S *s, T *obj)
{
  s->obj.ptr = static_cast<void *>(obj);
  s->is_initialized = 1;
  s->is_const = 0;
  s->is_owning = 0;
  return s;
}

template<typename T, typename S>
S *init_non_owning_struct(S *s, T const *obj)
{
  s->obj.ptr = const_cast<void *>(static_cast<void const *>(obj));
  s->is_initialized = 1;
  s->is_const = 1;
  s->is_owning = 0;
  return s;
}

template<typename T, typename S>
T *struct_cast(S *s)
{
  if (s->is_owning)
    return reinterpret_cast<T *>(&s->obj.mem);
  else
    return static_cast<T *>(s->obj.ptr);
}

template<typename S, typename T>
T const *struct_cast(S const *s)
{
  if (s->is_owning)
    return reinterpret_cast<T const *>(&s->obj.mem);
  else
    return static_cast<T const *>(s->obj.ptr);
}

} // namespace c

} // namespace cppbind
