#ifndef GUARD_WRAPPER_TYPE_H
#define GUARD_WRAPPER_TYPE_H

#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <utility>

#include "clang/AST/Decl.h"
#include "clang/AST/Type.h"

#include "FundamentalTypes.hpp"
#include "Identifier.hpp"
#include "IdentifierIndex.hpp"
#include "Print.hpp"
#include "String.hpp"

namespace cppbind
{

class WrapperType
{
public:
  WrapperType(clang::QualType const &Type)
  : Type_(Type.getDesugaredType(CompilerState()->getASTContext()))
  {}

  WrapperType(clang::Type const *Type)
  : WrapperType(clang::QualType(Type, 0))
  {}

  WrapperType()
  : WrapperType(FundamentalTypes().get("void"))
  {}

  WrapperType(clang::TypeDecl const *Decl)
  : WrapperType(Decl->getTypeForDecl())
  {}

  bool operator==(WrapperType const &Wt) const
  { return Type_ == Wt.Type_; }

  bool operator!=(WrapperType const &Wt) const
  { return !(*this == Wt); }

  clang::QualType const &operator*() const
  { return Type_; }

  clang::QualType const *operator->() const
  { return &Type_; }

  bool isQualified() const
  { return Type_.hasQualifiers(); }

  bool isFundamental(char const *Which = nullptr) const
  {
    if (!typePtr()->isFundamentalType())
      return false;

    if (!Which)
      return true;

    return FundamentalTypes().is(typePtr(), Which);
  }

  bool isWrappable(std::shared_ptr<IdentifierIndex> II) const
  { return II->has(name(), IdentifierIndex::TYPE); }

  bool isPointer() const
  { return typePtr()->isPointerType(); }

  bool isClass() const
  { return typePtr()->isClassType(); }

  bool isStruct() const
  { return typePtr()->isStructureType(); }

  bool isConst() const
  { return Type_.isConstQualified(); }

  bool isCType() const
  {
    if (!isFundamental())
      return false;

    return !static_cast<bool>(FundamentalTypes().toC(typePtr()));
  }

  std::optional<std::string> inCHeader() const
  {
    if (!isFundamental())
      return std::nullopt;

    return FundamentalTypes().inCHeader(typePtr());
  }

  WrapperType pointerTo() const
  { return WrapperType(CompilerState()->getASTContext().getPointerType(Type_)); }

  WrapperType pointee(bool recursive = false) const
  {
    if (!recursive)
      return Type_->getPointeeType();

    WrapperType Pointee(*this);
    while (Pointee.isPointer())
      Pointee = Pointee.pointee();

    return Pointee;
  }

  WrapperType unqualified() const
  { return WrapperType(typePtr()); }

  WrapperType withConst() const
  { return WrapperType(Type_.withConst()); }

  WrapperType base() const
  { return pointee(true); }

  Identifier name() const
  { return strBaseUnwrapped(); }

  std::string strWrapped(std::shared_ptr<IdentifierIndex> II) const
  {
    if (base().isFundamental())
      return toC(strUnwrapped());

    assert(isWrappable(II));

    auto Wrapped(strUnwrapped());

    if (base().isClass())
      replaceStr(Wrapped, "class", "struct");

    replaceStr(Wrapped, strBaseUnwrapped(), strBaseWrapped(II));

    return Wrapped;
  }

  std::string strUnwrapped(bool Compact = false) const
  {
    auto Unwrapped(printQualType(Type_, PrintingPolicy::CURRENT));

    if (Compact) {
      if (base().isClass())
        replaceStr(Unwrapped, "class ", "");
      if (base().isStruct())
        replaceStr(Unwrapped, "struct ", "");
    }

    return Unwrapped;
  }

  std::string strBaseWrapped(std::shared_ptr<IdentifierIndex> II) const
  {
    if (base().isFundamental())
      return toC(base().strUnwrapped());

    assert(isWrappable(II));

    auto Alias(II->alias(name()));

    return toC(Alias.strQualified(TYPE_CASE, true));
  }

  std::string strBaseUnwrapped() const
  { return printQualType(*base().unqualified(), PrintingPolicy::DEFAULT); }

  std::string strDeclaration(std::shared_ptr<IdentifierIndex> II) const
  { return strWrapped(II) + ";"; }

private:
  clang::QualType qualType() const
  { return Type_; }

  clang::Type const *typePtr() const
  { return Type_.getTypePtr(); }

  clang::QualType baseQualType() const
  { return *base(); }

  clang::Type const *baseTypePtr() const
  { return baseQualType().getTypePtr(); }

  std::string toC(std::string Str) const
  {
    auto CType(FundamentalTypes().toC(baseTypePtr()));

    if (CType)
      replaceStr(Str, strBaseUnwrapped(), *CType);

    return Str;
  }

  clang::QualType Type_;
};

inline std::ostream &operator<<(std::ostream &Os, WrapperType const &Wt)
{
  Os << "Type " << Wt.strBaseUnwrapped();
  return Os;
}

} // namespace cppbind

#endif // GUARD_WRAPPER_TYPE_H
