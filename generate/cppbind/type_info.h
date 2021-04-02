#ifndef GUARD_CPPBIND_TYPE_INFO_H
#define GUARD_CPPBIND_TYPE_INFO_H

#include <cassert>
#include <cstdint>
#include <type_traits>
#include <typeinfo>
#include <utility>

namespace cppbind
{

namespace type_info
{

template<typename T_KEY, typename T_VAL>
class simple_map
{
  enum : std::size_t { TABLE_SIZE = 100 };

  struct node
  {
    node(T_KEY key, T_VAL val)
    : key(key),
      val(val),
      next(nullptr)
    {}

    T_KEY key;
    T_VAL val;

    node *next;
  };

public:
  simple_map()
  : table()
  {}

  ~simple_map()
  {
    for (std::size_t offs = 0; offs < TABLE_SIZE; ++offs) {
      node *head = table[offs];
      while (head) {
        node *next = head->next;
        delete head;
        head = next;
      }
    }
  }

  void insert(T_KEY const &key, T_VAL const &val)
  {
    std::size_t offs = table_offs(key);

    node *next = table[offs];
    node *head = new node(key, val);
    head->next = next;
    table[offs] = head;
  }

  T_VAL const *find(T_KEY const &key) const
  {
    std::size_t offs = table_offs(key);

    node *head = table[offs];
    while (head) {
      if (head->key == key)
        return &head->val;
    }

    return nullptr;
  }

private:
  static std::size_t table_offs(T_KEY const &key)
  { return reinterpret_cast<std::size_t>(key) % TABLE_SIZE; }

  node *table[TABLE_SIZE];
};

class type
{
public:
  template<typename T>
  static void add(type const *obj)
  { lookup().insert(id<T>(), obj); }

  template<typename T>
  static type const *get()
  { return *lookup().find(id<T>()); }

  virtual void *copy(void const *obj) const = 0;
  virtual void *move(void *obj) const = 0;
  virtual void destroy(void const *obj) const = 0;

  virtual void const *cast(type const *to, void const *obj) const = 0;
  virtual void *cast(type const *to, void *obj) const = 0;

private:
  template<typename T>
  static std::uintptr_t id()
  { return id_cv<typename std::remove_cv<T>::type>(); }

  template<typename T>
  static std::uintptr_t id_cv()
  {
    static int id;
    return reinterpret_cast<std::uintptr_t>(&id);
  }

  using map = simple_map<std::uintptr_t, type const *>;

  static map &lookup()
  {
    static map lookup_instance;
    return lookup_instance;
  }
};

template<typename T, typename ...BS>
class type_instance : public type
{
public:
  type_instance()
  {
    type::add<T>(this);

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
    auto const *cast = _casts.find(to);
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
  { _casts.insert(type::get<B>(), cast<B>); }

  void add_base_casts()
  { [](...){}((add_cast<BS>(), 0)...); }

  simple_map<type const *, void const *(*)(void const *)> _casts;
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
