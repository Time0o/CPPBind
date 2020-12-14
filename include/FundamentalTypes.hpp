#ifndef GUARD_FUNDAMENTAL_TYPES_H
#define GUARD_FUNDAMENTAL_TYPES_H

#include <cassert>
#include <memory>
#include <optional>
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
  FundamentalTypeRegistry(FundamentalTypeRegistry const &) = delete;
  FundamentalTypeRegistry(FundamentalTypeRegistry &&)      = delete;
  void operator=(FundamentalTypeRegistry const &)           = delete;
  void operator=(FundamentalTypeRegistry &&)                = delete;

  void clear()
  {
    FundamentalTypes_.clear();
    CTypeEquivalents_.clear();
  }

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

  std::optional<std::string> toC(clang::Type const *Type)
  {
    auto Equiv(getCTypeEquivalent(Type));
    if (!Equiv || Equiv->CType == Equiv->CXXType)
      return std::nullopt;

    return Equiv->CType;
  }

  std::optional<std::string> inCHeader(clang::Type const *Type)
  {
    auto Equiv(getCTypeEquivalent(Type));
    if (!Equiv)
      return std::nullopt;

    return Equiv->CHeader;
  }

private:
  FundamentalTypeRegistry() = default;

  static FundamentalTypeRegistry &instance()
  {
    static FundamentalTypeRegistry Ftr;
    return Ftr;
  }

  static clang::QualType asQualType(clang::Type const *Type)
  { return clang::QualType(Type, 0); }

  struct CTypeEquivalent
  {
    CTypeEquivalent(std::string CXXType, std::string CType, std::string CHeader)
    : CXXType(CXXType),
      CType(CType),
      CHeader(CHeader)
    {}

    std::string CXXType;
    std::string CType;
    std::string CHeader;
  };

  std::optional<CTypeEquivalent> getCTypeEquivalent(clang::Type const *Type)
  {
    if (CTypeEquivalents_.empty())
      initCTypeEquivalents();

    auto IT(CTypeEquivalents_.find(Type));

    if (IT == CTypeEquivalents_.end())
      return std::nullopt;

    return IT->second;
  }

  void initCTypeEquivalents()
  {
    auto insert = [&](std::string const &CXXType,
                      std::string const &CType,
                      std::string const &CHeader)
    {
      CTypeEquivalents_.emplace(get(CXXType),
                                CTypeEquivalent(CXXType, CType, CHeader));
    };

    // XXX nullptr_t

    insert("char16_t", "uint16_t", "stdint.h");
    insert("char32_t", "uint32_t", "stdint.h");
    insert("bool", "bool", "bool.h"),
    insert("wchar_t", "wchar_t", "wchar.h");
  }

  std::unordered_map<std::string, clang::Type const *> FundamentalTypes_;
  std::unordered_map<clang::Type const *, CTypeEquivalent> CTypeEquivalents_;
};

inline FundamentalTypeRegistry &FundamentalTypes()
{ return FundamentalTypeRegistry::instance(); }

} // namespace cppbind

#endif // GUARD_FUNDAMENTAL_TYPES_H
