#ifndef GUARD_CPPBIND_TYPE_INFO_H
#define GUARD_CPPBIND_TYPE_INFO_H

#include <cassert>
#include <stdexcept>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <utility>

namespace cppbind
{

namespace type_info
{


class type
{
  using map = std::unordered_map<std::type_index, type const *>;

public:
  static map &lookup()
  {
    static map lookup_instance;
    return lookup_instance;
  }

  template<typename T>
  static type const *get()
  {
    auto it(lookup().find(typeid(typename std::remove_const<T>::type)));
    assert(it != lookup().end());

    return it->second;
  }

  virtual void *copy(void const *obj) const = 0;
  virtual void *move(void *obj) const = 0;
  virtual void destroy(void const *obj) const = 0;

  virtual void const *cast(type const *to, void const *obj) const = 0;
  virtual void *cast(type const *to, void *obj) const = 0;

protected:
};

template<typename T, typename ...BS>
class type_instance : public type
{
public:
  type_instance()
  {
    add_type();
    add_cast<T>();
    add_base_casts();
  }

  void *copy(void const *obj) const override
  { return _copy(obj); }

  void *move(void *obj) const override
  { return _move(obj); }

  void destroy(void const *obj) const override
  { delete static_cast<T const *>(obj); }

  void const *cast(type const *to, void const *obj) const override
  {
    auto it(_casts.find(to));
    if (it == _casts.end())
        throw std::bad_cast();

    return it->second(obj);
  }

  void *cast(type const *to, void *obj) const override
  { return const_cast<void *>(cast(to, const_cast<void const *>(obj))); }

private:
  template<typename U = T>
  typename std::enable_if<std::is_copy_constructible<U>::value, void *>::type
  _copy(void const *obj) const
  {
    auto obj_copy = new T(*static_cast<T const *>(obj));
    return static_cast<void *>(obj_copy);
  }

  template<typename U = T>
  typename std::enable_if<!std::is_copy_constructible<U>::value, void *>::type
  _copy(void const *obj) const
  { throw std::runtime_error("not copy constructible"); }

  template<typename U = T>
  typename std::enable_if<std::is_move_constructible<U>::value, void *>::type
  _move(void *obj) const
  {
    auto obj_moved = new T(std::move(*static_cast<T *>(obj)));
    return static_cast<void *>(obj_moved);
  }

  template<typename U = T>
  typename std::enable_if<!std::is_move_constructible<U>::value, void *>::type
  _move(void *obj) const
  { throw std::runtime_error("not move constructible"); }

  void add_type() const
  { type::lookup().insert(std::make_pair(std::type_index(typeid(T)), this)); }

  template<typename U>
  static void const *cast(void const *obj)
  {
    auto const *from = static_cast<T const *>(obj);
    auto const *to = static_cast<U const *>(from);

    return static_cast<void const *>(to);
  }

  template<typename B>
  void add_cast()
  { _casts.insert(std::make_pair(type::get<B>(), cast<B>)); }

  void add_base_casts()
  { [](...){}((add_cast<BS>(), 0)...); }

  std::unordered_map<type const *, void const *(*)(void const *)> _casts;
};

class typed_ptr
{
public:
  typed_ptr(type const *type_,
            bool const_,
            void const *obj,
            bool owning = false)
  : _type(type_),
    _const(const_),
    _obj(obj),
    _owning(owning)
  {}

  template<typename T>
  typed_ptr(T *obj, bool owning = false)
  : typed_ptr(type::get<T>(),
              std::is_const<T>::value,
              static_cast<void const *>(obj),
              owning)
  {}

  ~typed_ptr()
  {
    if (_owning)
      _type->destroy(_obj);
  }

  void *copy() const
  {
    auto obj_copied = _type->copy(_obj);
    return static_cast<void *>(new typed_ptr(_type, false, obj_copied, true));
  }

  void *move() const
  {
    if (_const)
      return copy();

    auto obj_moved = _type->move(const_cast<void *>(_obj));
    return static_cast<void *>(new typed_ptr(_type, false, obj_moved, true));
  }

  void own()
  { _owning = true; }

  void disown()
  { _owning = false; }

  template<typename T>
  typename std::enable_if<std::is_const<T>::value, T *>::type
  cast() const
  {
    if (_const)
      return static_cast<T *>(_type->cast(type::get<T>(), _obj));

    return static_cast<T *>(_type->cast(type::get<T>(), const_cast<void *>(_obj)));
  }

  template<typename T>
  typename std::enable_if<!std::is_const<T>::value, T *>::type
  cast() const
  {
    if (_const)
      throw std::bad_cast();

    return static_cast<T *>(_type->cast(type::get<T>(), const_cast<void *>(_obj)));
  }

private:
  type const *_type;
  bool _const;
  void const *_obj;
  bool _owning;
};

template<typename T, typename ...ARGS>
void *make_typed(T *obj, ARGS &&...args)
{
  auto ptr = new typed_ptr(obj, std::forward<ARGS>(args)...);
  return static_cast<void *>(ptr);
}

inline typed_ptr *get_typed(void *ptr)
{ return static_cast<typed_ptr *>(ptr); }

template<typename T>
T *typed_pointer_cast(void *ptr)
{ return get_typed(ptr)->cast<T>(); }

inline void *bind_own(void *obj)
{
  get_typed(obj)->own();
  return obj;
}

inline void *bind_disown(void *obj)
{
  get_typed(obj)->disown();
  return obj;
}

inline void *bind_copy(void *obj)
{ return get_typed(obj)->copy(); }

// XXX copy assignment

inline void *bind_move(void *obj)
{ return get_typed(obj)->move(); }

// XXX move assignment

inline void bind_delete(void *obj)
{
  if (obj)
    delete get_typed(obj);
}

} // namespace type_info

} // namespace cppbind

#endif // GUARD_CPPBIND_TYPE_INFO_H
