#ifndef GUARD_WRAPPER_TYPE_H
#define GUARD_WRAPPER_TYPE_H

#include <memory>
#include <string>

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
  WrapperType()
  : WrapperType(FundamentalTypes().get("void"))
  {}

  explicit WrapperType(clang::QualType const &Type)
  : Type_(Type.getDesugaredType(CompilerState()->getASTContext()))
  {}

  explicit WrapperType(clang::Type const *Type)
  : WrapperType(clang::QualType(Type, 0))
  {}

  explicit WrapperType(clang::TypeDecl const *Decl)
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

  clang::Type const *typePtr() const
  { return Type_.getTypePtr(); }

  bool isFundamental(char const *Which = nullptr) const
  {
    if (!typePtr()->isFundamentalType())
      return false;

    if (!Which)
      return true;

    return FundamentalTypes().is(typePtr(), Which);
  }

  bool isVoid() const
  { return typePtr()->isVoidType(); }

  bool isIntegral() const
  { return typePtr()->isIntegralType(CompilerState()->getASTContext()); }

  bool isFloating() const
  { return typePtr()->isFloatingType(); }

  bool isReference() const
  { return typePtr()->isReferenceType(); }

  bool isLValueReference() const
  { return typePtr()->isLValueReferenceType(); }

  bool isRValueReference() const
  { return typePtr()->isRValueReferenceType(); }

  bool isPointer() const
  { return typePtr()->isPointerType(); }

  bool isClass() const
  { return typePtr()->isClassType(); }

  bool isStruct() const
  { return typePtr()->isStructureType(); }

  bool isQualified() const
  { return Type_.hasQualifiers(); }

  bool isConst() const
  { return Type_.isConstQualified(); }

  WrapperType referenceTo() const
  { return WrapperType(CompilerState()->getASTContext().getLValueReferenceType(Type_)); }

  WrapperType referenced() const
  { return WrapperType(Type_.getNonReferenceType()); }

  WrapperType pointerTo() const
  { return WrapperType(CompilerState()->getASTContext().getPointerType(Type_)); }

  WrapperType pointee(bool recursive = false) const
  {
    if (!recursive)
      return WrapperType(Type_->getPointeeType());

    WrapperType Pointee(*this);
    while (Pointee.isPointer())
      Pointee = Pointee.pointee();

    return Pointee;
  }

  WrapperType unqualified() const
  { return WrapperType(typePtr()); }

  WrapperType withConst() const
  { return WrapperType(Type_.withConst()); }

  WrapperType withoutConst() const
  {
    auto TypeCopy(Type_);
    TypeCopy.removeLocalConst();

    return WrapperType(TypeCopy);
  }

  WrapperType base() const
  { return referenced().pointee(true).unqualified(); }

  std::string str() const
  { return printQualType(Type_, PrintingPolicy::CURRENT); }

private:
  clang::QualType Type_;
};

} // namespace cppbind

#endif // GUARD_WRAPPER_TYPE_H
