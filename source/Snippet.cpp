#include <sstream>
#include <string>

#include "WrapperRecord.hpp"
#include "Snippet.hpp"
#include "Typeinfo.hpp"

namespace cppbind
{

std::string
FundamentalTypesSnippet::include_()
{ return "#include <cstddef>"; }

std::string
FundamentalTypesSnippet::namespace_()
{ return "__fundamental_types"; }

std::string
FundamentalTypesSnippet::code()
{
  return &R"(
extern void *_void;

extern bool _bool;

extern short int _short_int;
extern int _int;
extern long _long;
extern long long _long_long;

extern unsigned short int _unsigned_short_int;
extern unsigned int _unsigned_int;
extern unsigned long long _unsigned_long_long;
extern unsigned long _unsigned_long;

extern std::size_t _size_t;

extern signed char _signed_char;
extern unsigned char _unsigned_char;
extern char _char;
extern wchar_t _wchar_t;
extern char16_t _char16_t;
extern char32_t _char32_t;

extern float _float;
extern double _double;
extern long double _long_double;)"[1];
}

std::string
TypeinfoSnippet::include_()
{
  return &R"(
#include <cassert>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>)"[1];
}

std::string
TypeinfoSnippet::namespace_()
{ return Typeinfo::Namespace; }

std::string
TypeinfoSnippet::code()
{
  return &R"(
class type
{
public:
  template<typename T>
  static type const *get()
  {
    auto it(_types.find(typeid(T)));
    assert(it != _types.end());

    return it->second;
  }

  virtual void const *cast(type const *to, void const *obj) const = 0;
  virtual void *cast(type const *to, void *obj) const = 0;

protected:
  static std::unordered_map<std::type_index, type const *> _types;
};

std::unordered_map<std::type_index, type const *> type::_types;

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
  template<typename B>
  static void const *cast(void const *obj)
  {
    auto const *base = static_cast<B const*>(static_cast<T const *>(obj));

    return static_cast<void const *>(base);
  }

  void add_type() const
  { _types.insert(std::make_pair(std::type_index(typeid(T)), this)); }

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
  typed_ptr(type const *type, void const *obj)
  : _type(type),
    _obj(obj)
  {}

  void const *cast(type const *to) const
  { return _type->cast(to, _obj); }

  void *cast(type const *to)
  { return const_cast<void *>(const_cast<typed_ptr const *>(this)->cast(to)); }

private:
  type const *_type;
  void const *_obj;
};

template<typename T>
void const *make_typed_ptr(T const *obj)
{
  auto const *ptr = new typed_ptr(type::get<T>(), static_cast<void const *>(obj));

  return static_cast<void const *>(ptr);
}

template<typename T>
void *make_typed_ptr(T *obj)
{ return const_cast<void *>(make_typed_ptr(const_cast<T const *>(obj))); }

template<typename T>
T const *typed_ptr_cast(void const *ptr)
{
  void const *obj = static_cast<typed_ptr const *>(ptr)->cast(type::get<T>());

  return static_cast<T const *>(obj);
}

template<typename T>
T *typed_ptr_cast(void *ptr)
{ return const_cast<T *>(typed_ptr_cast<T>(const_cast<void const *>(ptr))); }

)"[1] + Typeinfo::typeInstances();
}

} // namespace cppbind
