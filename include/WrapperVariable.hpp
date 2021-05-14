#ifndef GUARD_WRAPPER_VARIABLE_H
#define GUARD_WRAPPER_VARIABLE_H

#include "Identifier.hpp"
#include "LLVMFormat.hpp"
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

  explicit WrapperVariable(clang::ValueDecl const *Decl)
  : WrapperObject<clang::ValueDecl>(Decl),
    Name_(Decl),
    Type_(Decl->getType())
  {}

  Identifier getName() const
  { return Name_; }

  WrapperType getType() const
  { return Type_; }

private:
  Identifier Name_;
  WrapperType Type_;
};

} // namespace cppbind

namespace llvm { LLVM_FORMAT_PROVIDER(cppbind::WrapperVariable); }

#endif // GUARD_WRAPPER_VARIABLE_H
