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
WrapperType::pointee(bool recursive) const
{
  if (!recursive)
    return modified(type()->getPointeeType());

  WrapperType Pointee(*this);
  while (Pointee.isPointer())
    Pointee = Pointee.pointee();

  return modified(Pointee);
}

WrapperType
WrapperType::withEnum() const
{
  auto Base(base());

  assert(Base.isEnum());

  return modified(changeBase(Base));
}

WrapperType
WrapperType::withoutEnum() const
{
  auto Base(base());

  if (!Base.isEnum())
    return *this;

  auto const *EnumType = Base.typePtr()->getAs<clang::EnumType>();

  WrapperType UnderlyingType(EnumType->getDecl()->getIntegerType());

  WrapperType NewType(changeBase(UnderlyingType));
  NewType.BaseType_ = baseType();

  return NewType;
}

WrapperType
WrapperType::base() const
{ return WrapperType(baseType()); }

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
  auto StrBase(WrapperType(baseTypePtr()).str(true));

  string::replace(Str, StrBase, Identifier(StrBase).format(Case, Quals));

  return Str;
}

} // namespace cppbind
