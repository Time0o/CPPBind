#ifndef GUARD_WRAPPER_TYPE_H
#define GUARD_WRAPPER_TYPE_H

#include <string>

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
  WrapperType()
  : WrapperType(FundamentalTypes().get("void"))
  {}

  explicit WrapperType(clang::QualType const &Type)
  : Type_(Type.getDesugaredType(CompilerState()->getASTContext()))
  {}

  explicit WrapperType(clang::Type const *Type, unsigned Qualifiers = 0u)
  : WrapperType(clang::QualType(Type, Qualifiers))
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
  { return WrapperType(CompilerState()->getASTContext().getLValueReferenceType(type())); }

  WrapperType rvalueReferenceTo() const
  { return WrapperType(CompilerState()->getASTContext().getRValueReferenceType(type())); }

  WrapperType referenced() const
  { return WrapperType(type().getNonReferenceType()); }

  WrapperType pointerTo() const
  { return WrapperType(CompilerState()->getASTContext().getPointerType(type())); }

  WrapperType pointee(bool recursive = false) const;

  WrapperType qualified(unsigned Qualifiers) const
  { return WrapperType(typePtr(), Qualifiers); }

  WrapperType unqualified() const
  { return WrapperType(typePtr()); }

  WrapperType withConst() const
  { return WrapperType(type().withConst()); }

  WrapperType withoutConst() const
  {
    auto TypeCopy(type());
    TypeCopy.removeLocalConst();

    return WrapperType(TypeCopy);
  }

  WrapperType withoutEnum() const;

  WrapperType base() const;

  WrapperType changeBase(WrapperType const &NewBase) const;

  std::string str(bool Compact = false) const;

  std::string format(Identifier::Case Case, Identifier::Quals Quals) const;

private:
  clang::QualType const &type() const
  { return Type_; }

  clang::Type const *typePtr() const
  { return Type_.getTypePtr(); }

  clang::QualType Type_;
};

} // namespace cppbind

#endif // GUARD_WRAPPER_TYPE_H
