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

// A singleton class that stores a mapping to fundamental type definitions from
// their string representations so they can be conveniently accessed later.
// See generate/cppbind/fundamental_types.h for a list of all fundamental types.
class FundamentalTypeRegistry : private mixin::NotCopyOrMovable
{
  friend FundamentalTypeRegistry &FundamentalTypes();

public:
  void add(clang::Type const *Type)
  {
    assert(Type->isFundamentalType());

    auto TypeName(print::type(Type));

    FundamentalTypes_[TypeName] = Type;
  }

  // Get a type object for the fundamental type with string representation
  // 'TypeName'.
  clang::Type const *get(std::string const &TypeName) const
  {
    auto IT(FundamentalTypes_.find(TypeName));
    assert(IT != FundamentalTypes_.end());

    return IT->second;
  }

  // Check whether 'Type' is a (particular) fundamental type.
  bool is(clang::Type const *Type, std::string const &TypeName = "")
  {
    if (!Type->isFundamentalType())
      return false;

    if (TypeName.empty())
      return true;

    return clang::QualType(Type, 0).getCanonicalType() == clang::QualType(get(TypeName), 0);
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
