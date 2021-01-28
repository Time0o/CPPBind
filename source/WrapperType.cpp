#include <cassert>
#include <stack>
#include <string>

#include "CompilerState.hpp"
#include "FundamentalTypes.hpp"
#include "Identifier.hpp"
#include "Print.hpp"
#include "String.hpp"
#include "WrapperType.hpp"

namespace cppbind
{

WrapperType::WrapperType(clang::QualType const &Type,
                         WrapperType::TagSet const &Tags)
: Type_(Type.getDesugaredType(CompilerState()->getASTContext())),
  Tags_(Tags)
{}

WrapperType::WrapperType(clang::Type const *Type,
                         WrapperType::TagSet const &Tags)
: WrapperType(clang::QualType(Type, 0u))
{}

WrapperType::WrapperType(std::string const &TypeName,
                         WrapperType::TagSet const &Tags)
: WrapperType(FundamentalTypes().get(TypeName))
{}

WrapperType::WrapperType(clang::TypeDecl const *Decl,
                         WrapperType::TagSet const &Tags)
: WrapperType(Decl->getTypeForDecl())
{}

bool
WrapperType::isFundamental(char const *Which) const
{
  if (!typePtr()->isFundamentalType())
    return false;

  if (!Which)
    return true;

  return FundamentalTypes().is(typePtr(), Which);
}

bool
WrapperType::isIntegral() const
{
  if (!typePtr()->isIntegralType(CompilerState()->getASTContext()))
    return false;

  return !isBoolean() && !isScopedEnum();
}

bool
WrapperType::isIntegralUnderlyingEnum() const
{
  return isIntegral() &&
         hasTag<UnderliesEnumTag>() &&
         getTag<UnderliesEnumTag>()->EnumBaseType->isEnumeralType();
}

bool
WrapperType::isIntegralUnderlyingScopedEnum() const
{
  return isIntegral() &&
         hasTag<UnderliesEnumTag>() &&
         getTag<UnderliesEnumTag>()->EnumBaseType->isScopedEnumeralType();
}

bool
WrapperType::isRecord() const
{
  if (isStruct() || isClass())
    return true;

  assert(!typePtr()->isRecordType()); // XXX

  return false;
}

WrapperType
WrapperType::lvalueReferenceTo() const
{
  return WrapperType(
    CompilerState()->getASTContext().getLValueReferenceType(type()), Tags_);
}

WrapperType
WrapperType::rvalueReferenceTo() const
{
  return WrapperType(
    CompilerState()->getASTContext().getRValueReferenceType(type()), Tags_);
}

WrapperType
WrapperType::referenced() const
{ return WrapperType(type().getNonReferenceType(), Tags_); }

WrapperType
WrapperType::pointerTo() const
{
  return WrapperType(
    CompilerState()->getASTContext().getPointerType(type()), Tags_);
}

WrapperType
WrapperType::pointee(bool Recursive) const
{
  if (!Recursive)
    return WrapperType(type()->getPointeeType(), Tags_);

  WrapperType Pointee(*this);
  while (Pointee.isPointer())
    Pointee = Pointee.pointee();

  return Pointee;
}

WrapperType
WrapperType::qualified(unsigned Qualifiers) const
{ return WrapperType(requalifyType(type(), Qualifiers), Tags_); }

WrapperType
WrapperType::unqualified() const
{ return WrapperType(typePtr(), Tags_); }

WrapperType
WrapperType::withConst() const
{ return WrapperType(type().withConst(), Tags_); }

WrapperType
WrapperType::withoutConst() const
{
  auto TypeCopy(type());
  TypeCopy.removeLocalConst();

  return WrapperType(TypeCopy, Tags_);
}

WrapperType
WrapperType::withEnum() const
{
  if (!hasTag<UnderliesEnumTag>())
    return *this;

  WrapperType EnumBaseType(
    requalifyType(getTag<UnderliesEnumTag>()->EnumBaseType, qualifiers()),
    withoutTag<UnderliesEnumTag>());

  return changeBase(EnumBaseType);
}

WrapperType
WrapperType::withoutEnum() const
{
  if (!base().isEnum())
    return *this;

  auto const *EnumType = baseTypePtr()->getAs<clang::EnumType>();

  WrapperType UnderlyingBaseType(
    EnumType->getDecl()->getIntegerType(),
    withTag<UnderliesEnumTag>(baseType()));

  return changeBase(UnderlyingBaseType);
}

WrapperType
WrapperType::base() const
{ return referenced().pointee(true); }

WrapperType
WrapperType::changeBase(WrapperType const &NewBase) const
{
  enum Indirection
  {
    POINTER,
    LVALUE_REFERENCE,
    RVALUE_REFERENCE
  };

  std::stack<Indirection> Indirections;

  WrapperType NewType(*this);

  if (NewType.isLValueReference()) {
    Indirections.push(LVALUE_REFERENCE);
    NewType = NewType.referenced();
  } else if (NewType.isRValueReference()) {
    Indirections.push(RVALUE_REFERENCE);
    NewType = NewType.referenced();
  }

  while (NewType.isPointer()) {
    Indirections.push(POINTER);
    NewType = NewType.pointee();
  }

  NewType = NewBase;

  while (!Indirections.empty()) {
    switch (Indirections.top()) {
    case POINTER:
      NewType = NewType.pointerTo();
      break;
    case LVALUE_REFERENCE:
      NewType = NewType.lvalueReferenceTo();
      break;
    case RVALUE_REFERENCE:
      NewType = NewType.rvalueReferenceTo();
      break;
    }

    Indirections.pop();
  }

  return NewType;
}

std::string
WrapperType::str() const
{ return format(); }

std::string
WrapperType::format(bool Compact,
                    Identifier::Case Case,
                    Identifier::Quals Quals) const
{
  static PrintingPolicy PPCompact = PrintingPolicy::DEFAULT,
                        PPNonCompact = PrintingPolicy::CURRENT;

  auto Str(printQualType(type(), Compact ? PPCompact : PPNonCompact));
  auto StrBase(printQualType(requalifyType(baseType(), 0u), PPCompact));

  string::replace(Str, StrBase, Identifier(StrBase).format(Case, Quals));

  return Str;
}

} // namespace cppbind
