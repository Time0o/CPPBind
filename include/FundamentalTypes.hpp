#ifndef GUARD_FUNDAMENTAL_TYPES_H
#define GUARD_FUNDAMENTAL_TYPES_H

#include <cassert>
#include <memory>
#include <string>
#include <unordered_map>

#include "clang/AST/PrettyPrinter.h"
#include "clang/AST/Type.h"
#include "clang/Tooling/Tooling.h"

#include "CompilerState.hpp"
#include "Print.hpp"

namespace cppbind
{

class FundamentalTypeRegistry
{
  friend FundamentalTypeRegistry &FundamentalTypes();

public:
  void add(clang::Type const *Type)
  {
    assert(Type->isFundamentalType());

    auto TypeName(printQualType(asQualType(Type), PrintingPolicy::DEFAULT));

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

#endif // GUARD_FUNDAMENTAL_TYPES_H
