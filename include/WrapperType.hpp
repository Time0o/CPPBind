#ifndef GUARD_WRAPPER_TYPE_H
#define GUARD_WRAPPER_TYPE_H

#include <optional>
#include <string>
#include <utility>

#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Type.h"

#include "CompilerState.hpp"
#include "FundamentalTypes.hpp"
#include "Identifier.hpp"

namespace cppbind
{

class WrapperType
{
public:
  explicit WrapperType(clang::QualType const &Type)
  : Type_(Type.getDesugaredType(CompilerState()->getASTContext()))
  {}

  explicit WrapperType(clang::Type const *Type, unsigned Qualifiers = 0u)
  : WrapperType(clang::QualType(Type, Qualifiers))
  {}

  explicit WrapperType(std::string const &Which = "void")
  : WrapperType(FundamentalTypes().get(Which))
  {}

  explicit WrapperType(clang::TypeDecl const *Decl)
  : WrapperType(Decl->getTypeForDecl())
  {}

  bool operator==(WrapperType const &Wt) const
  { return type() == Wt.type(); }

  bool operator!=(WrapperType const &Wt) const
  { return !(*this == Wt); }

  bool isFundamental(char const *Which = nullptr) const;

  bool isVoid() const
  { return typePtr()->isVoidType(); }

  bool isBoolean() const
  { return isFundamental("bool"); }

  bool isEnum() const
  { return typePtr()->isEnumeralType(); }

  bool isScopedEnum() const
  { return typePtr()->isScopedEnumeralType(); }

  bool isIntegral() const;

  bool isIntegralUnderlyingEnum() const
  { return isIntegral() && EnumBaseType_; }

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

  bool isRecord() const;

  bool isStruct() const
  { return typePtr()->isStructureType(); }

  bool isConst() const
  { return type().isConstQualified(); }

  WrapperType lvalueReferenceTo() const
  { return modified(CompilerState()->getASTContext().getLValueReferenceType(type())); }

  WrapperType rvalueReferenceTo() const
  { return modified(CompilerState()->getASTContext().getRValueReferenceType(type())); }

  WrapperType referenced() const
  { return modified(type().getNonReferenceType()); }

  WrapperType pointerTo() const
  { return modified(CompilerState()->getASTContext().getPointerType(type())); }

  WrapperType pointee(bool recursive = false) const;

  WrapperType qualified(unsigned Qualifiers) const
  { return WrapperType(typePtr(), Qualifiers); }

  WrapperType unqualified() const
  { return modified(typePtr()); }

  WrapperType withConst() const
  { return modified(type().withConst()); }

  WrapperType withoutConst() const
  {
    auto TypeCopy(type());
    TypeCopy.removeLocalConst();

    return modified(TypeCopy);
  }

  WrapperType withEnum() const;
  WrapperType withoutEnum() const;

  WrapperType base() const;
  WrapperType changeBase(WrapperType const &NewBase) const;

  std::string str(bool Compact = false) const;

  std::string format(Identifier::Case Case, Identifier::Quals Quals) const;

private:
  clang::QualType const &type() const
  { return Type_; }

  clang::QualType baseType() const
  { return base().type(); }

  std::optional<clang::QualType> enumBaseType() const
  {
    auto Base(base());

    if (Base.isEnum()) {
      auto const *EnumBaseTypePtr = Base.typePtr()->getAs<clang::EnumType>();
      return clang::QualType(EnumBaseTypePtr, qualifiers());
    }

    return std::nullopt;
  }

  clang::Type const *typePtr() const
  { return type().getTypePtr(); }

  unsigned qualifiers() const
  { return type().getQualifiers().getAsOpaqueValue(); }

  template<typename ...ARGS>
  WrapperType modified(ARGS &&...args) const
  {
    WrapperType Modified(std::forward<ARGS>(args)...);
    Modified.EnumBaseType_ = EnumBaseType_;

    return Modified;
  }

  clang::QualType Type_;
  std::optional<clang::QualType> EnumBaseType_;
};

} // namespace cppbind

#endif // GUARD_WRAPPER_TYPE_H
