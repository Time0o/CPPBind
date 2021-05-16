#ifndef GUARD_WRAPPER_VARIABLE_H
#define GUARD_WRAPPER_VARIABLE_H

#include "Identifier.hpp"
#include "LLVMFormat.hpp"
#include "WrapperFunction.hpp"
#include "WrapperObject.hpp"
#include "WrapperType.hpp"

#include "clang/AST/Decl.h"

namespace cppbind
{

class WrapperVariable : public WrapperObject<clang::ValueDecl>
{
public:
  WrapperVariable(Identifier const &Name, WrapperType const &Type)
  : Name_(Name),
    Type_(Type)
  {}

  explicit WrapperVariable(clang::VarDecl const *Decl)
  : WrapperObject<clang::ValueDecl>(Decl),
    Name_(Decl),
    Type_(Decl->getType()),
    IsConstexpr_(Decl->isConstexpr())
  {}

  Identifier getName() const
  { return Name_; }

  WrapperType getType() const
  { return Type_; }

  WrapperFunction getGetter() const;
  WrapperFunction getSetter() const;

  bool isConst() const
  { return Type_.isConst(); }

  bool isConstexpr() const
  { return IsConstexpr_; }

  bool isAssignable() const;

private:
  Identifier Name_;
  Identifier prefixedName(std::string const &Prefix) const;

  WrapperType Type_;

  bool IsConstexpr_ = false;
};

} // namespace cppbind

namespace llvm { LLVM_FORMAT_PROVIDER(cppbind::WrapperVariable); }

#endif // GUARD_WRAPPER_VARIABLE_H
