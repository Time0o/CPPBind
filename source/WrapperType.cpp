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
  assert(EnumBaseType_);

  WrapperType EnumBaseType(*EnumBaseType_);

  return changeBase(EnumBaseType);
}

WrapperType
WrapperType::withoutEnum() const
{
  auto EnumBaseType(enumBaseType());

  if (!EnumBaseType)
    return *this;

  auto const *EnumBaseTypePtr = (*EnumBaseType)->getAs<clang::EnumType>();

  WrapperType UnderlyingType(EnumBaseTypePtr->getDecl()->getIntegerType());
  UnderlyingType.EnumBaseType_ = EnumBaseType;

  return changeBase(UnderlyingType);
}

WrapperType
WrapperType::base() const
{ return modified(referenced().pointee(true)); }

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

  Base.EnumBaseType_ = NewBase.EnumBaseType_;

  return Base;
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
