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

  Identifier name() const
  { return Name_; }

  WrapperType type() const
  { return Type_; }

private:
  Identifier Name_;
  WrapperType Type_;
};

} // namespace cppbind

#endif // GUARD_WRAPPER_VARIABLE_H
