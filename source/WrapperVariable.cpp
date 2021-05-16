#include "Identifier.hpp"
#include "Logging.hpp"
#include "WrapperFunction.hpp"
#include "WrapperVariable.hpp"

namespace cppbind
{

WrapperFunction
WrapperVariable::getGetter() const
{
  auto T(getType());

  T = T.unqualified();

  return WrapperFunctionBuilder(prefixedName("get"))
         .setReturnType(T)
         .setPropertyFor(this)
         .setIsGetter()
         .setIsNoexcept()
         .build();
}

WrapperFunction
WrapperVariable::getSetter() const
{
  if (!isAssignable())
    throw log::exception("tried to create setter for constant variable {0}", getName());

  auto T(getType());

  if (T.isReference())
    T = T.referenced();

  T = T.unqualified();

  return WrapperFunctionBuilder(prefixedName("set"))
         .pushParameter(Identifier("val"), T)
         .setPropertyFor(this)
         .setIsSetter()
         .setIsNoexcept()
         .build();
}

bool
WrapperVariable::isAssignable() const
{ return !Type_.isConst() && !(Type_.isReference() && Type_.referenced().isConst()); }

Identifier
WrapperVariable::prefixedName(std::string const &Prefix) const
{
  Identifier PrefixedName(Prefix + "_" + getNonNamespacedName().str());

  auto Namespace(getNamespace());

  if (Namespace)
    PrefixedName = PrefixedName.qualified(*Namespace);

  return PrefixedName;
}

} // namespace cppbind
