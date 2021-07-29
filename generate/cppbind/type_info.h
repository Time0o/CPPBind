#ifndef GUARD_CPPBIND_TYPE_INFO_H
#define GUARD_CPPBIND_TYPE_INFO_H

#include <cassert>
#include <cstdint>
#include <cstring>
#include <memory>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <utility>

namespace cppbind
{

namespace type_info
{

class type_id
{
public:
  using id_t = uint64_t;

  template<typename T>
  static id_t id()
  { return id_no_cv<typename std::remove_cv<T>::type>(); }

private:
  template<typename T>
  static id_t id_no_cv()
  {
    std::size_t len;
    char const *str = str_type<T>(&len);

    return str_hash_fnv1a(str, len);
  }

  static uint64_t str_hash_fnv1a(char const *str, std::size_t len)
  {
    uint64_t h = 0xcbf29ce484222325;

    for (std::size_t i = 0; i < len; ++i) {
      h ^= str[i];
      h *= 0x100000001b3;
    }

    return h;
  }

  template<typename T>
  static char const *str_type(std::size_t *len)
  {
    static char const *str_ref = str_pretty_function<int>();
    static std::size_t len_ref = std::strlen(str_ref);
    static std::size_t len_prefix_ref = std::strstr(str_ref, "int") - str_ref;
    static std::size_t len_postfix_ref = len_ref - len_prefix_ref - std::strlen("int");

    char const *str = str_pretty_function<T>() + len_prefix_ref;
    *len = std::strlen(str) - len_postfix_ref;

    return str;
  }

  template<typename T>
  static char const *str_pretty_function()
  { return __PRETTY_FUNCTION__; }
};

template<typename T>
class type_map
{
public:
  class const_iterator
  {
  public:
    using pointer = std::pair<type_id::id_t, T> const *;
    using reference = std::pair<type_id::id_t, T> const &;

    const_iterator(pointer data = nullptr)
    : _data(data)
    {}

    bool operator==(const_iterator const &rhs) const
    {
      if (!_data && !rhs._data)
        return true;

      if (!!_data != !!rhs._data)
        return false;

      return *_data == *rhs._data;
    }

    bool operator!=(const_iterator const &rhs) const
    { return !operator==(rhs); }

    reference operator*() const
    { return *_data; }

    pointer operator->() const
    { return _data; }

  private:
    pointer _data;
  };

  type_map()
  {
    for (type_id::id_t i = 0; i < HASH_TABLE_SZ; ++i)
      _hash_table[i] = nullptr;
  }

  ~type_map()
  {
    for (type_id::id_t i = 0; i < HASH_TABLE_SZ; ++i) {
      auto head = _hash_table[i];
      while (head) {
        auto next = head->next;
        delete head;
        head = next;
      }
    }
  }

  std::pair<const_iterator, bool> insert(type_id::id_t key, T const &val)
  {
    auto key_hashed = hash(key);

    auto head = _hash_table[key_hashed];

    _hash_table[key_hashed] = new hash_node(key, val, head);

    return std::make_pair(const_iterator(&_hash_table[key_hashed]->data), !head);
  }

  const_iterator find(type_id::id_t key) const
  {
    auto key_hashed = hash(key);

    auto head = _hash_table[key_hashed];

    while (head) {
      if (head->data.first == key)
        return const_iterator(&head->data);

      head = head->next;
    }

    return end();
  }

  const_iterator end() const
  { return const_iterator(); }

private:
  enum : type_id::id_t { HASH_TABLE_SZ = 149 };

  struct hash_node
  {
    hash_node(type_id::id_t key, T const &val, hash_node *next = nullptr)
    : data(key, val),
      next(next)
    {}

    std::pair<type_id::id_t, T> data;

    hash_node *next;
  };

  static type_id::id_t hash(type_id::id_t key)
  { return key % HASH_TABLE_SZ; }

  hash_node * _hash_table[HASH_TABLE_SZ];
};

class type
{
public:
  static void add(type_id::id_t identifier, type const *type)
  { lookup().insert(identifier, type); }

  template<typename T>
  static type const *get()
  {
    auto it(lookup().find(type_id::id<T>()));
    assert(it != lookup().end());

    return it->second;
  }

  type_id::id_t identifier() const { return _identifier; }

  virtual void *copy(void const *obj) const = 0;
  virtual void *move(void *obj) const = 0;
  virtual void destroy(void const *obj) const = 0;

  virtual void const *cast(type const *to, void const *obj) const = 0;
  virtual void *cast(type const *to, void *obj) const = 0;

protected:
  type(type_id::id_t identifier)
  : _identifier(std::move(identifier))
  { add(_identifier, this); }

private:
  static type_map<type const*> &lookup()
  {
    static type_map<type const*> _lookup;

    return _lookup;
  }

  type_id::id_t _identifier;
};

template<typename T, typename ...T_BASES>
class type_instance : public type
{
public:
  type_instance()
  : type(type_id::id<T>())
  { add_base_casts(); }

  void *copy(void const *obj) const override
  { return _copy(obj); }

  void *move(void *obj) const override
  { return _move(obj); }

  void destroy(void const *obj) const override
  { _destroy(obj); }

  void const *cast(type const *to, void const *obj) const override
  {
    auto it(_casts.find(to->identifier()));
    if (it == _casts.end())
      throw std::bad_cast();

    return (it->second)(obj);
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

  template<typename U>
  void add_cast()
  { _casts.insert(type_id::id<U>(), cast<U>); }

  void add_base_casts()
  {
    add_cast<T>();

    [](...){}((add_cast<T_BASES>(), 0)...);
  }

  type_map<void const *(*)(void const *)> _casts;
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
  : typed_ptr(type::get<T>(),
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
      return static_cast<T *>(_type->cast(type::get<T>(), _obj));

    return static_cast<T *>(_type->cast(type::get<T>(), const_cast<void *>(_obj)));
  }

  template<typename T>
  typename std::enable_if<!std::is_const<T>::value, T *>::type
  cast() const
  {
    if (_is_const)
      throw std::bad_cast();

    return static_cast<T *>(_type->cast(type::get<T>(), const_cast<void *>(_obj)));
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
