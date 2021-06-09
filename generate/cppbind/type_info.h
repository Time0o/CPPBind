#ifndef GUARD_CPPBIND_TYPE_INFO_H
#define GUARD_CPPBIND_TYPE_INFO_H

#include <cassert>
#include <cstdint>
#include <memory>
#include <type_traits>
#include <typeinfo>
#include <utility>

#include "cppbind/util.h"

namespace cppbind
{

namespace type_info
{

class type
{
public:
  using id = std::size_t;
  using map = util::simple_map<id, type const *>;

  type(id id)
  : _id(id)
  {
    if (!by_id().find(id))
      by_id().insert(id, this);
  }

  template<typename T>
  static id opaque()
  { return opaque_cv<typename std::remove_cv<T>::type>(); }

  template<typename T>
  static constexpr id opaque_cv()
  { return util::string_hash(__PRETTY_FUNCTION__); } // XXX guarantee uniqueness

  template<typename T>
  static type const *lookup()
  {
    auto t = by_id().find(opaque<T>());
    assert(t);

    return *t;
  }

  id identifier() const { return _id; }

  virtual void *copy(void const *obj) const = 0;
  virtual void *move(void *obj) const = 0;
  virtual void destroy(void const *obj) const = 0;

  virtual void const *cast(type const *to, void const *obj) const = 0;
  virtual void *cast(type const *to, void *obj) const = 0;

private:
  static map &by_id()
  {
    static map _by_id;
    return _by_id;
  }

  id _id;
};

template<typename T, typename ...BS>
class type_instance : public type
{
public:
  type_instance() : type(type::opaque<T>())
  {
    add_cast<T>();
    add_base_casts();
  }

  void *copy(void const *obj) const override
  { return _copy(obj); }

  void *move(void *obj) const override
  { return _move(obj); }

  void destroy(void const *obj) const override
  { _destroy(obj); }

  void const *cast(type const *to, void const *obj) const override
  {
    auto const *cast = _casts.find(to->identifier());
    if (!cast)
      throw std::bad_cast();

    return (*cast)(obj);
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
  _copy(void const *) const
  { throw std::bad_cast(); }

  template<typename U = T>
  typename std::enable_if<std::is_move_constructible<U>::value, void *>::type
  _move(void *obj) const
  {
    auto obj_moved = new T(std::move(*static_cast<T *>(obj)));
    return static_cast<void *>(obj_moved);
  }

  template<typename U = T>
  typename std::enable_if<!std::is_move_constructible<U>::value, void *>::type
  _move(void *) const
  { throw std::bad_cast(); }

  template<typename U = T>
  typename std::enable_if<std::is_destructible<U>::value>::type
  _destroy(void const *obj) const
  { delete static_cast<T const *>(obj); }

  template<typename U = T>
  typename std::enable_if<!std::is_destructible<U>::value>::type
  _destroy(void const *) const
  {}

  template<typename U>
  static void const *cast(void const *obj)
  {
    auto const *from = static_cast<T const *>(obj);
    auto const *to = static_cast<U const *>(from);

    return static_cast<void const *>(to);
  }

  template<typename B>
  void add_cast()
  { _casts.insert(type::opaque<B>(), cast<B>); }

  void add_base_casts()
  { [](...){}((add_cast<BS>(), 0)...); }

  util::simple_map<type::id, void const *(*)(void const *)> _casts;
};

class typed_ptr
{
public:
  typed_ptr(type const *type,
            void const *obj,
            bool is_const,
            bool is_owning = false)
  : _type(type),
    _obj(obj),
    _is_const(is_const),
    _is_owning(is_owning),
    _is_deleted(false)
  {}

  template<typename T>
  typed_ptr(T *obj, bool is_owning = false)
  : typed_ptr(type::lookup<T>(),
              static_cast<void const *>(obj),
              std::is_const<T>::value,
              is_owning)
  {}

  ~typed_ptr()
  {
    if (_is_owning && !_is_deleted) {
      _type->destroy(_obj);
      _is_deleted = true;
    }
  }

  void *copy(void *mem = nullptr) const
  {
    auto obj_copied = _type->copy(_obj);

    auto ptr_copied = mem ? new (mem) typed_ptr(_type, obj_copied, false, true)
                          : new typed_ptr(_type, obj_copied, false, true);

    return static_cast<void *>(ptr_copied);
  }

  void *move(void *mem = nullptr) const
  {
    if (_is_const)
      return copy();

    auto obj_moved = _type->move(const_cast<void *>(_obj));

    auto ptr_moved = mem ? new (mem) typed_ptr(_type, obj_moved, false, true)
                         : new typed_ptr(_type, obj_moved, false, true);

    return static_cast<void *>(ptr_moved);
  }

  void own()
  { _is_owning = true; }

  void disown()
  { _is_owning = false; }

  template<typename T>
  typename std::enable_if<std::is_const<T>::value, T *>::type
  cast() const
  {
    if (_is_const)
      return static_cast<T *>(_type->cast(type::lookup<T>(), _obj));

    return static_cast<T *>(_type->cast(type::lookup<T>(), const_cast<void *>(_obj)));
  }

  template<typename T>
  typename std::enable_if<!std::is_const<T>::value, T *>::type
  cast() const
  {
    if (_is_const)
      throw std::bad_cast();

    return static_cast<T *>(_type->cast(type::lookup<T>(), const_cast<void *>(_obj)));
  }

private:
  type const *_type;
  void const *_obj;
  bool _is_const;
  bool _is_owning;
  bool _is_deleted;
};

template<typename T>
void *make_typed(T *obj, void *mem = nullptr, bool owning = false)
{
  auto ptr = mem ? new (mem) typed_ptr(obj, owning)
                 : new typed_ptr(obj, owning);

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
