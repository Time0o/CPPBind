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

  explicit WrapperType(clang::Type const *Type)
  : WrapperType(clang::QualType(Type, 0u))
  {}

  explicit WrapperType(clang::TypeDecl const *Decl)
  : WrapperType(Decl->getTypeForDecl())
  {}

  explicit WrapperType(std::string const &Which = "void")
  : WrapperType(FundamentalTypes().get(Which))
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
  { return isIntegral() && base().isEnum(); }

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

  clang::Type const *typePtr() const
  { return type().getTypePtr(); }

  clang::QualType baseType() const
  {
    if (BaseType_.isNull())
      return referenced().pointee(true).type();

    return BaseType_;
  }

  clang::Type const *baseTypePtr() const
  { return baseType().getTypePtr(); }

  unsigned qualifiers() const
  { return type().getQualifiers().getAsOpaqueValue(); }

  template<typename ...ARGS>
  WrapperType modified(ARGS &&...args) const
  {
    WrapperType Modified(std::forward<ARGS>(args)...);

    if (!BaseType_.isNull()) {
      Modified.BaseType_ = clang::QualType(BaseType_.getTypePtr(),
                                           Modified.qualifiers());
    }

    return Modified;
  }

  clang::QualType Type_, BaseType_;
};

} // namespace cppbind

#endif // GUARD_WRAPPER_TYPE_H
