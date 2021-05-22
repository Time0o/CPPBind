#ifndef GUARD_WRAPPER_DEFINITION_H
#define GUARD_WRAPPER_DEFINITION_H

#include <string>

#include "Identifier.hpp"
#include "WrapperType.hpp"
#include "WrapperVariable.hpp"

namespace cppbind
{

class WrapperDefinition
{
public:
  explicit WrapperDefinition(Identifier const &Name, std::string const &Arg)
  : Name_(Name),
    Arg_(Arg)
  {}

  Identifier getName() const
  { return Name_; }

  std::string getArg() const
  { return Arg_; }

  WrapperVariable getAsVariable(
    WrapperType const &Type = WrapperType("int").withConst()) const
  { return WrapperVariable(Name_, Type); }

  std::string str() const
  { return "#define " + Name_.str() + " " + Arg_; }

private:
  Identifier Name_;
  std::string Arg_;
};

} // namespace cppbind

#endif // GUARD_WRAPPER_DEFINITION_H
