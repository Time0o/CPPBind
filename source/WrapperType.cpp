#include <cassert>
#include <stack>
#include <string>
#include <vector>

#include "llvm/ADT/StringRef.h"

#include "ClangUtils.hpp"
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
: Type_(Type.getDesugaredType(ASTContext())),
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
WrapperType::isMatchedBy(char const *Matcher) const
{ return decl::matchDecl(typePtr(), llvm::StringRef(Matcher)); }

bool
WrapperType::isFundamental(char const *Which) const
{
  if (!type()->isFundamentalType())
    return false;

  if (!Which)
    return true;

  return FundamentalTypes().is(typePtr(), Which);
}

bool
WrapperType::isVoid() const
{ return type()->isVoidType(); }

bool
WrapperType::isBoolean() const
{ return isFundamental("bool"); }

bool
WrapperType::isEnum() const
{ return type()->isEnumeralType(); }

bool
WrapperType::isScopedEnum() const
{ return type()->isScopedEnumeralType(); }

bool
WrapperType::isIntegral() const
{
  if (!type()->isIntegralType(ASTContext()))
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
WrapperType::isFloating() const
{ return type()->isFloatingType(); }

bool
WrapperType::isReference() const
{ return type()->isReferenceType(); }

bool
WrapperType::isLValueReference() const
{ return type()->isLValueReferenceType(); }

bool
WrapperType::isRValueReference() const
{ return type()->isRValueReferenceType(); }

bool
WrapperType::isPointer() const
{ return type()->isPointerType(); }

bool
WrapperType::isRecord() const
{
  if (isStruct() || isClass())
    return true;

  assert(!type()->isRecordType()); // XXX

  return false;
}

bool
WrapperType::isStruct() const
{ return type()->isStructureType(); }

bool
WrapperType::isClass() const
{ return type()->isClassType(); }

bool
WrapperType::isConst() const
{ return type().isConstQualified(); }

WrapperType
WrapperType::lvalueReferenceTo() const
{ return WrapperType(ASTContext().getLValueReferenceType(type()), Tags_); }

WrapperType
WrapperType::rvalueReferenceTo() const
{ return WrapperType(ASTContext().getRValueReferenceType(type()), Tags_); }

WrapperType
WrapperType::referenced() const
{ return WrapperType(type().getNonReferenceType(), Tags_); }

WrapperType
WrapperType::pointerTo(unsigned Repeat) const
{
  WrapperType PointerTo(ASTContext().getPointerType(type()), Tags_);

  return Repeat == 0u ? PointerTo : PointerTo.pointerTo(Repeat - 1u);
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

unsigned
WrapperType::qualifiers() const
{ return type().getQualifiers().getAsOpaqueValue(); }

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

  WrapperType NewType(*this);
  NewType.setBase(EnumBaseType);
  return NewType;
}

WrapperType
WrapperType::withoutEnum() const
{
  if (!getBase().isEnum())
    return *this;

  auto const *EnumType = baseType()->getAs<clang::EnumType>();

  WrapperType UnderlyingBaseType(
    EnumType->getDecl()->getIntegerType(),
    withTag<UnderliesEnumTag>(baseType()));

  WrapperType NewType(*this);
  NewType.setBase(UnderlyingBaseType);
  return NewType;
}

WrapperType
WrapperType::getBase() const
{ return referenced().pointee(true); }

void
WrapperType::setBase(WrapperType const &NewBase)
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

  *this = NewType;
}

std::vector<std::string>
WrapperType::getAnnotations() const
{
  if (!hasTag<AnnotatedTag>())
    return {};

  return getTag<AnnotatedTag>()->Annotations;
}

void
WrapperType::setAnnotations(std::vector<std::string> const &Annotations)
{ addTag<AnnotatedTag>(Annotations); }

std::string
WrapperType::str() const
{ return format(); }

std::string
WrapperType::format(bool Mangled,
                    bool Compact,
                    Identifier::Case Case,
                    Identifier::Quals Quals) const
{
  if (Mangled)
    return printMangledQualType(type());

  if (getBase().isRecord()) {
    PrintingPolicy PPCompact = PrintingPolicy::CURRENT;
    PrintingPolicy PPNonCompact = PrintingPolicy::NONE;

    auto Str(printQualType(type(), Compact ? PPCompact : PPNonCompact));
    auto StrBase(printQualType(requalifyType(baseType(), 0u), PPCompact));

    string::replace(Str, StrBase, Identifier(StrBase).format(Case, Quals));

    return Str;

  } else {
    return printQualType(type(), PrintingPolicy::CURRENT);
  }
}

clang::QualType const &
WrapperType::type() const
{ return Type_; }

clang::Type const *
WrapperType::typePtr() const
{ return type().getTypePtr(); }

clang::QualType
WrapperType::baseType() const
{ return getBase().type(); }

clang::Type const *
WrapperType::baseTypePtr() const
{ return baseType().getTypePtr(); }

clang::QualType
WrapperType::requalifyType(clang::QualType const &Type, unsigned Qualifiers)
{ return clang::QualType(Type.getTypePtr(), Qualifiers); }

} // namespace cppbind
