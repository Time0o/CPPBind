#ifndef GUARD_WRAPPER_VARIABLE_H
#define GUARD_WRAPPER_VARIABLE_H

#include "Identifier.hpp"
#include "WrapperType.hpp"

#include "clang/AST/Decl.h"

namespace cppbind
{

class WrapperVariable
{
public:
  WrapperVariable(Identifier const &Name, WrapperType const &Type)
  : Name_(Name),
    Type_(Type)
  {}

  explicit WrapperVariable(clang::VarDecl const *Decl)
  : WrapperVariable(Identifier(Decl), WrapperType(Decl->getType()))
  {}

  Identifier getName() const
  { return Name_; }

  void setName(Identifier const &Name)
  { Name_ = Name; }

  WrapperType getType() const
  { return Type_; }

  void setType(WrapperType const &Type)
  { Type_ = Type; }

private:
  Identifier Name_;
  WrapperType Type_;
};

} // namespace cppbind

#endif // GUARD_WRAPPER_VARIABLE_H
