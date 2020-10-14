#ifndef GUARD_WRAPPER_TYPE_H
#define GUARD_WRAPPER_TYPE_H

#include <cassert>
#include <memory>
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
  { assert(isFundamental() || isWrappable()); }

  WrapperType(clang::Type const *Type)
  : WrapperType(clang::QualType(Type, 0))
  {}

  WrapperType()
  : WrapperType(FundamentalTypes().get("void"))
  {}

  WrapperType(clang::TypeDecl const *Decl)
  : WrapperType(Decl->getTypeForDecl())
  {}

  clang::QualType const &operator*() const
  { return Type_; }

  clang::QualType const *operator->() const
  { return &Type_; }

  WrapperType unqualified() const
  { return WrapperType(Type_.getTypePtr()); }

  bool isQualified() const
  { return Type_.hasQualifiers(); }

  bool isVoid() const
  { return FundamentalTypes().is(Type_.getTypePtr(), "void"); }

  bool isPointer() const
  { return Type_->isPointerType(); }

  bool isClass() const
  { return (*pointee(true))->isClassType(); }

  bool isStruct() const
  { return (*pointee(true))->isStructureType(); }

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

  Identifier name() const
  { return strBaseUnwrapped(); }

  std::string strWrapped(std::shared_ptr<IdentifierIndex> II) const
  {
    if (isFundamental() || pointee(true).isFundamental())
      return strUnwrapped();

    auto Wrapped(strUnwrapped());

    if (isClass())
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

  std::string strBaseUnwrapped() const
  {
    clang::PrintingPolicy PP(CompilerState()->getLangOpts());
    return pointee(true).unqualified()->getAsString(PP);
  }

private:
  bool isFundamental() const
  { return Type_->isFundamentalType(); }

  bool isWrappable() const
  { return true; } // XXX

  clang::QualType Type_;
};

} // namespace cppbind

#endif // GUARD_WRAPPER_TYPE_H
