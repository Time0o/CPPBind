#ifndef GUARD_WRAPPER_TYPE_H
#define GUARD_WRAPPER_TYPE_H

#include <memory>
#include <ostream>
#include <string>
#include <utility>

#include "clang/AST/Decl.h"
#include "clang/AST/Type.h"

#include "FundamentalTypes.hpp"
#include "Identifier.hpp"
#include "IdentifierIndex.hpp"
#include "String.hpp"

namespace cppbind
{

class WrapperType
{
public:
  WrapperType(clang::QualType const &Type)
  : Type_(Type)
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

  WrapperType unqualified() const
  { return WrapperType(typePtr()); }

  WrapperType desugared() const
  { return WrapperType(Type_.getDesugaredType(CompilerState()->getASTContext())); }

  WrapperType unqualifiedAndDesugared() const
  { return WrapperType(typePtr()->getUnqualifiedDesugaredType()); }

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
  { return II->has(name(true), IdentifierIndex::TYPE); }

  bool isPointer() const
  { return typePtr()->isPointerType(); }

  bool isClass() const
  { return typePtr()->isClassType(); }

  bool isStruct() const
  { return typePtr()->isStructureType(); }

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

  WrapperType base() const
  { return pointee(true); }

  Identifier name(bool desugar = false) const
  { return strBaseUnwrapped(desugar); }

  std::string strWrapped(std::shared_ptr<IdentifierIndex> II) const
  {
    if (base().isFundamental())
      return strUnwrapped();

    assert(isWrappable(II));

    auto Wrapped(strUnwrapped());

    if (base().isClass())
      replaceStr(Wrapped, "class", "struct");

    replaceStr(Wrapped, strBaseUnwrapped(), strBaseWrapped(II));

    return Wrapped;
  }

  std::string strUnwrapped(bool compact = false) const
  {
    auto Unwrapped(Type_.getAsString());

    if (compact) {
      if (isClass())
        replaceStr(Unwrapped, "class ", "");
      if (isStruct())
        replaceStr(Unwrapped, "struct ", "");
    }

    return Unwrapped;
  }

  std::string strBaseWrapped(std::shared_ptr<IdentifierIndex> II) const
  { return II->alias(name()).strQualified(TYPE_CASE, true); }

  std::string strBaseUnwrapped(bool desugar = false) const
  {
    clang::PrintingPolicy PP(CompilerState()->getLangOpts());

    auto T(desugar ? base().unqualifiedAndDesugared()
                   : base().unqualified());

    return T->getAsString(PP);
  }

  std::string strDeclaration(std::shared_ptr<IdentifierIndex> II) const
  { return strWrapped(II) + ";"; }

private:
  clang::Type const *typePtr() const
  { return Type_.getTypePtr(); }

  clang::QualType baseQualType() const
  { return *base(); }

  clang::Type const *baseTypePtr() const
  { return baseQualType().getTypePtr(); }

  clang::QualType Type_;
};

inline std::ostream &operator<<(std::ostream &Os, WrapperType const &Wt)
{
  auto Sugared(Wt.strBaseUnwrapped());
  auto Desugared(Wt.strBaseUnwrapped(true));

  Os << "Type " << Sugared;

  if (Desugared != Sugared)
    Os << " (alias " + Desugared + ")";

  return Os;
}

} // namespace cppbind

#endif // GUARD_WRAPPER_TYPE_H
