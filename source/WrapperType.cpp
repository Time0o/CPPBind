#include <cassert>
#include <stack>
#include <string>
#include <vector>

#include "llvm/ADT/StringRef.h"

#include "CompilerState.hpp"
#include "FundamentalTypes.hpp"
#include "Identifier.hpp"
#include "Print.hpp"
#include "String.hpp"
#include "WrapperType.hpp"

namespace cppbind
{

WrapperType::WrapperType()
: WrapperType("void")
{}

WrapperType::WrapperType(clang::QualType const &Type)
: Type_(Type.getDesugaredType(ASTContext()))
{}

WrapperType::WrapperType(clang::Type const *Type)
: WrapperType(clang::QualType(Type, 0u))
{}

WrapperType::WrapperType(std::string const &TypeName)
: WrapperType(FundamentalTypes().get(TypeName))
{}

WrapperType::WrapperType(clang::TypeDecl const *Decl)
: WrapperType(Decl->getTypeForDecl())
{}

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
{ return WrapperType(ASTContext().getLValueReferenceType(type())); }

WrapperType
WrapperType::rvalueReferenceTo() const
{ return WrapperType(ASTContext().getRValueReferenceType(type())); }

WrapperType
WrapperType::referenced() const
{ return WrapperType(type().getNonReferenceType()); }

WrapperType
WrapperType::pointerTo(unsigned Repeat) const
{
  WrapperType PointerTo(ASTContext().getPointerType(type()));

  return Repeat == 0u ? PointerTo : PointerTo.pointerTo(Repeat - 1u);
}

WrapperType
WrapperType::pointee(bool Recursive) const
{
  if (!Recursive)
    return WrapperType(type()->getPointeeType());

  WrapperType Pointee(*this);
  while (Pointee.isPointer())
    Pointee = Pointee.pointee();

  return Pointee;
}

WrapperType
WrapperType::underlyingIntegerType() const
{
  assert(isEnum());

  auto const *EnumType = type()->getAs<clang::EnumType>();

  return WrapperType(EnumType->getDecl()->getIntegerType());
}

unsigned
WrapperType::qualifiers() const
{ return type().getQualifiers().getAsOpaqueValue(); }

WrapperType
WrapperType::qualified(unsigned Qualifiers) const
{ return WrapperType(requalifyType(type(), Qualifiers)); }

WrapperType
WrapperType::unqualified() const
{ return WrapperType(typePtr()); }

WrapperType
WrapperType::withConst() const
{ return WrapperType(type().withConst()); }

WrapperType
WrapperType::withoutConst() const
{
  auto TypeCopy(type());
  TypeCopy.removeLocalConst();

  return WrapperType(TypeCopy);
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
