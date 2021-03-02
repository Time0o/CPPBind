#ifndef GUARD_WRAPPER_CONSTANT_H
#define GUARD_WRAPPER_CONSTANT_H

#include "Identifier.hpp"
#include "WrapperObject.hpp"
#include "WrapperType.hpp"

#include "clang/AST/Decl.h"

namespace cppbind
{

class WrapperConstant : public WrapperObject<clang::ValueDecl>
{
public:
  explicit WrapperConstant(clang::ValueDecl const *Decl)
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

namespace llvm
{

LLVM_WRAPPER_OBJECT_FORMAT_PROVIDER(cppbind::WrapperConstant, clang::ValueDecl)

} // namespace llvm

#endif // GUARD_WRAPPER_CONSTANT_H
