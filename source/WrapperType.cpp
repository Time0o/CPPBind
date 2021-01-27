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

WrapperType::WrapperType(clang::QualType const &Type)
: Type_(Type.getDesugaredType(CompilerState()->getASTContext()))
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
WrapperType::isRecord() const
{
  if (isStruct() || isClass())
    return true;

  assert(!typePtr()->isRecordType()); // XXX

  return false;
}

WrapperType
WrapperType::lvalueReferenceTo() const
{ return modified(CompilerState()->getASTContext().getLValueReferenceType(type())); }

WrapperType
WrapperType::rvalueReferenceTo() const
{ return modified(CompilerState()->getASTContext().getRValueReferenceType(type())); }

WrapperType
WrapperType::referenced() const
{ return modified(type().getNonReferenceType()); }

WrapperType
WrapperType::pointerTo() const
{ return modified(CompilerState()->getASTContext().getPointerType(type())); }

WrapperType
WrapperType::pointee(bool Recursive) const
{
  if (!Recursive)
    return modified(type()->getPointeeType());

  WrapperType Pointee(*this);
  while (Pointee.isPointer())
    Pointee = Pointee.pointee();

  return modified(Pointee);
}

WrapperType
WrapperType::qualified(unsigned Qualifiers) const
{ return modified(requalifyType(type(), Qualifiers)); }

WrapperType
WrapperType::unqualified() const
{ return modified(typePtr()); }

WrapperType
WrapperType::withConst() const
{ return modified(type().withConst()); }

WrapperType
WrapperType::withoutConst() const
{
  auto TypeCopy(type());
  TypeCopy.removeLocalConst();

  return modified(TypeCopy);
}

WrapperType
WrapperType::withEnum() const
{
  assert(base().isEnum());

  return modified(changeBase(base()));
}

WrapperType
WrapperType::withoutEnum() const
{
  assert(base().isEnum());

  auto const *EnumType = baseTypePtr()->getAs<clang::EnumType>();

  WrapperType UnderlyingType(EnumType->getDecl()->getIntegerType());

  WrapperType NewType(changeBase(UnderlyingType));
  NewType.BaseType_ = baseType();

  return NewType;
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
WrapperType::str(bool Compact) const
{
  PrintingPolicy PP = Compact ? PrintingPolicy::DEFAULT : PrintingPolicy::CURRENT;

  return printQualType(type(), PP);
}

std::string
WrapperType::format(Identifier::Case Case, Identifier::Quals Quals) const
{
  auto Str(str());
  auto StrBase(base().unqualified().str(true));

  string::replace(Str, StrBase, Identifier(StrBase).format(Case, Quals));

  return Str;
}

} // namespace cppbind
