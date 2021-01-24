#ifndef GUARD_WRAPPER_TYPE_H
#define GUARD_WRAPPER_TYPE_H

#include <cassert>
#include <stack>
#include <string>

#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Type.h"

#include "CompilerState.hpp"
#include "FundamentalTypes.hpp"
#include "Print.hpp"

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

  bool isBoolean() const
  { return isFundamental("bool"); }

  bool isEnum() const
  { return typePtr()->isEnumeralType(); }

  bool isScopedEnum() const
  { return typePtr()->isScopedEnumeralType(); }

  bool isIntegral() const
  {
    if (!typePtr()->isIntegralType(CompilerState()->getASTContext()))
      return false;

    return !isBoolean() && !isScopedEnum();
  }

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

  bool isRecord() const
  {
    if (isStruct() || isClass())
      return true;

    assert(!typePtr()->isRecordType()); // XXX

    return false;
  }

  bool isStruct() const
  { return typePtr()->isStructureType(); }

  bool isConst() const
  { return Type_.isConstQualified(); }

  WrapperType lvalueReferenceTo() const
  { return WrapperType(CompilerState()->getASTContext().getLValueReferenceType(Type_)); }

  WrapperType rvalueReferenceTo() const
  { return WrapperType(CompilerState()->getASTContext().getRValueReferenceType(Type_)); }

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

  WrapperType withConst() const
  { return WrapperType(Type_.withConst()); }

  WrapperType withoutConst() const
  {
    auto TypeCopy(Type_);
    TypeCopy.removeLocalConst();

    return WrapperType(TypeCopy);
  }

  WrapperType withoutEnum() const
  {
    auto OldBase(base());

    if (!OldBase.isEnum())
      return *this;

    auto const *EnumType((*OldBase)->getAs<clang::EnumType>());

    WrapperType UnderlyingType(EnumType->getDecl()->getIntegerType());

    return changeBase(UnderlyingType);
  }

  WrapperType base() const
  { return WrapperType(referenced().pointee(true)->getUnqualifiedType()); }

  WrapperType changeBase(WrapperType const &NewBase) const
  {
    enum Indirection
    {
      POINTER,
      LVALUE_REFERENCE,
      RVALUE_REFERENCE
    };

    std::stack<Indirection> Indirections;

    WrapperType Base(*this);

    if (Base.isLValueReference()) {
      Indirections.push(LVALUE_REFERENCE);
      Base = Base.referenced();
    } else if (Base.isRValueReference()) {
      Indirections.push(RVALUE_REFERENCE);
      Base = Base.referenced();
    }

    while (Base.isPointer()) {
      Indirections.push(POINTER);
      Base = Base.pointee();
    }

    Base = NewBase;

    while (!Indirections.empty()) {
      switch (Indirections.top()) {
      case POINTER:
        Base = Base.pointerTo();
        break;
      case LVALUE_REFERENCE:
        Base = Base.lvalueReferenceTo();
        break;
      case RVALUE_REFERENCE:
        Base = Base.rvalueReferenceTo();
        break;
      }

      Indirections.pop();
    }

    return Base;
  }

  std::string str() const
  { return printQualType(Type_, PrintingPolicy::CURRENT); }

private:
  clang::Type const *typePtr() const
  { return Type_.getTypePtr(); }

  clang::QualType Type_;
};

} // namespace cppbind

#endif // GUARD_WRAPPER_TYPE_H
