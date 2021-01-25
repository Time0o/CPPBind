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
    return WrapperType(type()->getPointeeType());

  WrapperType Pointee(*this);
  while (Pointee.isPointer())
    Pointee = Pointee.pointee();

  return Pointee;
}

WrapperType
WrapperType::withoutEnum() const
{
  auto OldBase(base());

  if (!OldBase.isEnum())
    return *this;

  auto const *EnumType(OldBase.typePtr()->getAs<clang::EnumType>());

  WrapperType UnderlyingType(EnumType->getDecl()->getIntegerType());

  return changeBase(UnderlyingType);
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
