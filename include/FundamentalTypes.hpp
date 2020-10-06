#ifndef GUARD_BUILTIN_TYPES_H
#define GUARD_BUILTIN_TYPES_H

#include <cassert>
#include <string>
#include <unordered_map>

#include "clang/AST/PrettyPrinter.h"
#include "clang/AST/Type.h"

#include "CompilerState.hpp"

namespace cppbind
{

class FundamentalTypeRegistry
{
  friend FundamentalTypeRegistry &FundamentalTypes();

public:
  void add(clang::Type const *Type)
  {
    assert(Type->isFundamentalType());

    clang::PrintingPolicy PP(CompilerState()->getLangOpts());
    auto TypeName(asQualType(Type).getAsString(PP));

    _FundamentalTypes[TypeName] = Type;
  }

  clang::Type const *get(std::string const &TypeName) const
  {
    auto IT(_FundamentalTypes.find(TypeName));
    assert(IT != _FundamentalTypes.end());

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

  std::unordered_map<std::string, clang::Type const *> _FundamentalTypes;
};

inline FundamentalTypeRegistry &FundamentalTypes()
{ return FundamentalTypeRegistry::instance(); }

} // namespace cppbind

#endif // GUARD_BUILTIN_TYPES_H
