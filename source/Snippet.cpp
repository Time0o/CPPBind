#include <sstream>
#include <string>

#include "WrapperRecord.hpp"
#include "Snippet.hpp"

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

} // namespace cppbind
