#ifndef GUARD_WRAPPER_TYPE_H
#define GUARD_WRAPPER_TYPE_H

#include <optional>
#include <string>
#include <utility>

#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Type.h"

#include "FundamentalTypes.hpp"
#include "Identifier.hpp"

namespace cppbind
{

class WrapperType
{
public:
  explicit WrapperType(clang::QualType const &Type);

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

  WrapperType lvalueReferenceTo() const;
  WrapperType rvalueReferenceTo() const;
  WrapperType referenced() const;

  WrapperType pointerTo() const;
  WrapperType pointee(bool Recursive = false) const;

  WrapperType qualified(unsigned Qualifiers) const;
  WrapperType unqualified() const;

  WrapperType withConst() const;
  WrapperType withoutConst() const;

  WrapperType withEnum() const;
  WrapperType withoutEnum() const;

  WrapperType base() const;
  WrapperType changeBase(WrapperType const &NewBase) const;

  std::string str() const;

  std::string format(bool Compact = false,
                     Identifier::Case Case = Identifier::ORIG_CASE,
                     Identifier::Quals Quals = Identifier::KEEP_QUALS) const;

private:
  clang::QualType const &type() const
  { return Type_; }

  clang::Type const *typePtr() const
  { return type().getTypePtr(); }

  bool hasSpecialBaseType() const
  { return !BaseType_.isNull(); }

  clang::QualType baseType() const
  {
    if (!hasSpecialBaseType())
      return base().type();

    return BaseType_;
  }

  clang::Type const *baseTypePtr() const
  { return baseType().getTypePtr(); }

  unsigned qualifiers() const
  { return type().getQualifiers().getAsOpaqueValue(); }

  clang::QualType requalifyType(clang::QualType const &Type,
                                unsigned Qualifiers) const
  { return clang::QualType(Type.getTypePtr(), Qualifiers); }

  template<typename ...ARGS>
  WrapperType modified(ARGS &&...args) const
  {
    WrapperType Modified(std::forward<ARGS>(args)...);

    if (hasSpecialBaseType())
      Modified.BaseType_ = requalifyType(baseType(), Modified.qualifiers());

    return Modified;
  }

  clang::QualType Type_, BaseType_;
};

} // namespace cppbind

#endif // GUARD_WRAPPER_TYPE_H
