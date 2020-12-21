#ifndef GUARD_FUNDAMENTAL_TYPES_HEADER_H
#define GUARD_FUNDAMENTAL_TYPES_HEADER_H

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>

#include "Logging.hpp"

namespace cppbind
{

class FundamentalTypesHeader
{
public:
  FundamentalTypesHeader()
  : Path_(createTmpPath())
  {
    std::ofstream Stream(path());
    if (!Stream)
      error() << "failed to generate temporary file";

    Stream << Header_;
    Stream.flush();
  }

  ~FundamentalTypesHeader()
  { std::remove(Path_.c_str()); }

  static std::string prepend(std::string &Code)
  { return Header_ + ("\n" + Code); }

  std::string path() const
  { return Path_; }

private:
  static std::string createTmpPath()
  {
    char Tmpnam[6] = {'X', 'X', 'X', 'X', 'X', 'X'};
    if (mkstemp(Tmpnam) == -1)
      throw std::runtime_error("failed to create temporary path");

    return Tmpnam;
  }

  static constexpr char const *Header_ = &R"(
#include <cstddef>

namespace __fundamental_types
{

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
extern long double _long_double;

} // namespace __fundamental_types)"[1];

  std::string Path_;
};

} // namespace cppbind

#endif // GUARD_FUNDAMENTAL_TYPES_HEADER_H