#include "Identifier.hpp"
#include "Logging.hpp"
#include "WrapperFunction.hpp"
#include "WrapperRecord.hpp"
#include "WrapperVariable.hpp"

namespace cppbind
{

WrapperFunction
WrapperVariable::getGetter()
{
  auto T(getType());

  T = T.unqualified();

  return WrapperFunctionBuilder(prefixedName("get"))
         .setNamespace(getNamespace())
         .setReturnType(T)
         .setPropertyFor(this)
         .setIsGetter()
         .setIsNoexcept()
         .build();
}

WrapperFunction
WrapperVariable::getSetter()
{
  if (!isAssignable())
    throw log::exception("tried to create setter for constant variable {0}", getName());

  auto T(getType());

  if (T.isReference())
    T = T.referenced();

  T = T.unqualified();

  return WrapperFunctionBuilder(prefixedName("set"))
         .setNamespace(getNamespace())
         .pushParameter(Identifier("val"), T)
         .setPropertyFor(this)
         .setIsSetter()
         .setIsNoexcept()
         .build();
}

bool
WrapperVariable::isConst() const
{ return Type_.isConst(); }

bool
WrapperVariable::isConstexpr() const
{ return IsConstexpr_; }

bool
WrapperVariable::isAssignable() const
{
  auto T = Type_.isReference() ? Type_.referenced() : Type_;

  if (T.isConst())
    return false;

  if (!T.isRecord())
    return true;

  auto Record = T.asRecord();

  return Record && (*Record)->getCopyAssignmentOperator();
}

Identifier
WrapperVariable::prefixedName(std::string const &Prefix)
{
  auto Name(getName());
  auto Namespace(getNamespace());

  if (!Namespace)
    return Identifier(Prefix + "_" + Name.str());

  Name = Name.unqualified(Namespace->components().size());

  return Identifier(Prefix + "_" + Name.str()).qualified(*Namespace);
}

} // namespace cppbind
