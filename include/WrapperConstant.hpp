#ifndef GUARD_WRAPPER_CONSTANT_H
#define GUARD_WRAPPER_CONSTANT_H

#include "Identifier.hpp"
#include "LLVMFormat.hpp"
#include "WrapperObject.hpp"
#include "WrapperType.hpp"

#include "clang/AST/Decl.h"

namespace cppbind
{

class WrapperConstant : public WrapperObject<clang::ValueDecl>
{
public:
  explicit WrapperConstant(Identifier const &Name,
                           WrapperType const &Type = WrapperType("int"))
  : Name_(Name),
    Type_(Type),
    IsMacro_(true)
  {}

  explicit WrapperConstant(clang::ValueDecl const *Decl)
  : WrapperObject<clang::ValueDecl>(Decl),
    Name_(Decl),
    Type_(Decl->getType()),
    IsMacro_(false)
  {}

  Identifier getName() const
  { return Name_; }

  WrapperType getType() const
  { return Type_; }

  bool isMacro() const
  { return IsMacro_; }

private:
  Identifier Name_;
  WrapperType Type_;

  bool IsMacro_;
};

} // namespace cppbind

namespace llvm { LLVM_FORMAT_PROVIDER(cppbind::WrapperConstant); }

#endif // GUARD_WRAPPER_CONSTANT_H
