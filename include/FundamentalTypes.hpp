#ifndef GUARD_BUILTIN_TYPES_H
#define GUARD_BUILTIN_TYPES_H

#include <cassert>
#include <memory>
#include <string>
#include <unordered_map>

#include "clang/AST/PrettyPrinter.h"
#include "clang/AST/Type.h"
#include "clang/Tooling/Tooling.h"

#include "ClangIncludes.hpp"
#include "CompilerState.hpp"

namespace cppbind
{

constexpr char const *FundamentalTypesHeader = R"(
  #include <cstddef>

  namespace __fundamental_types
  {

  extern void *_void;

  extern std::nullptr_t _nullptr_t;

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

  } // namespace __fundamental_types
)";

inline void parseFundamentalTypes(
  std::unique_ptr<clang::tooling::FrontendActionFactory> const &Factory)
{
  clang::tooling::runToolOnCodeWithArgs(Factory->create(),
                                        FundamentalTypesHeader,
                                        clangIncludes());
}

class FundamentalTypeRegistry
{
  friend FundamentalTypeRegistry &FundamentalTypes();

public:
  void add(clang::Type const *Type)
  {
    assert(Type->isFundamentalType());

    clang::PrintingPolicy PP(CompilerState()->getLangOpts());
    auto TypeName(asQualType(Type).getAsString(PP));

    FundamentalTypes_[TypeName] = Type;
  }

  clang::Type const *get(std::string const &TypeName) const
  {
    auto IT(FundamentalTypes_.find(TypeName));
    assert(IT != FundamentalTypes_.end());

    return IT->second;
  }

  bool is(clang::Type const *Type, std::string const &TypeName = "")
  {
    if (!Type->isFundamentalType())
      return false;

    if (TypeName.empty())
      return true;

    return asQualType(Type) == asQualType(get(TypeName));
  }

private:
  static FundamentalTypeRegistry &instance()
  {
    static FundamentalTypeRegistry Ftr;
    return Ftr;
  }

  static clang::QualType asQualType(clang::Type const *Type)
  { return clang::QualType(Type, 0); }

  std::unordered_map<std::string, clang::Type const *> FundamentalTypes_;
};

inline FundamentalTypeRegistry &FundamentalTypes()
{ return FundamentalTypeRegistry::instance(); }

} // namespace cppbind

#endif // GUARD_BUILTIN_TYPES_H
