#ifndef GUARD_FUNDAMENTAL_TYPES_H
#define GUARD_FUNDAMENTAL_TYPES_H

#include <cassert>
#include <string>
#include <unordered_map>

#include "clang/AST/Type.h"

#include "Mixin.hpp"
#include "Print.hpp"

namespace cppbind
{

class FundamentalTypeRegistry : private mixin::NotCopyOrMoveable
{
  friend FundamentalTypeRegistry &FundamentalTypes();

public:
  void clear()
  { FundamentalTypes_.clear(); }

  void add(clang::Type const *Type)
  {
    assert(Type->isFundamentalType());

    auto TypeName(printType(Type, PrintingPolicy::DEFAULT));

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

    return clang::QualType(Type, 0) == clang::QualType(get(TypeName), 0);
  }

private:
  FundamentalTypeRegistry() = default;

  static FundamentalTypeRegistry &instance()
  {
    static FundamentalTypeRegistry Ftr;
    return Ftr;
  }

  std::unordered_map<std::string, clang::Type const *> FundamentalTypes_;
};

inline FundamentalTypeRegistry &FundamentalTypes()
{ return FundamentalTypeRegistry::instance(); }

} // namespace cppbind

#endif // GUARD_FUNDAMENTAL_TYPES_H
